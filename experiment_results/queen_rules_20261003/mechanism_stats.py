#!/usr/bin/env python3
"""Read-only mechanism checks for the completed queen-rule experiments."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
sys.path.insert(0,str(ROOT / "experiment_results/protect_local_20260927"))
from analyze_replays import Message, parse_map, point

def scan(path, side, feed_only=False):
    root=Message(path.read_bytes()).structure(0,0)
    _,_,dragons,teams=parse_map(root.text(0))
    qid=0 if side=="A" else 1
    round_index=-1
    feeds=[]; splits=[]; death=None
    for event in root.structs(3):
        kind=event.number(0,"H"); data=event.child(0)
        if kind==0:
            round_index=data.number(0)
        elif kind==6:
            text=data.text(0)
            if "QUEEN_FEED" in text.split():
                feeds.append({"round_index":round_index,"donor":data.number(0),"log":text,
                              "queen_alive_at_log":qid in dragons,
                              "queen_length_at_log":len(dragons.get(qid,[]))})
        elif kind==9:
            did=data.number(0); body=dragons[did]
            body.insert(0,point(data.child(0))); tail=point(data.child(1))
            while len(body)>1 and body[-1]!=tail:body.pop()
        elif kind==10:
            parent,child=data.number(0),data.number(4)
            before=len(dragons[parent])
            parent_body=[point(p) for p in data.structs(0)]
            child_body=[point(p) for p in data.structs(1)]
            dragons[parent],dragons[child]=parent_body,child_body
            teams[child]="AB"[data.number(8,"H")]
            if parent==qid:
                splits.append({"round_index":round_index,"length_before":before,
                               "parent_length_after":len(parent_body),"child_length":len(child_body)})
        elif kind==11:
            did=data.number(0)
            if did==qid:
                reason=("wall","self","body","head","invalid_action")[data.number(4,"H")]
                death={"round_index":round_index,"reason":reason,"last_split":splits[-1] if splits else None}
            dragons.pop(did,None)
    return {"feeds":feeds,"queen_splits":splits,"queen_death":death}

def collection_scan(path, side):
    root=Message(path.read_bytes()).structure(0,0)
    _,_,dragons,teams=parse_map(root.text(0)); qid=0 if side=="A" else 1
    entries=[]; pending={}; sources={}; round_index=-1; turn=None; donor=None; pending_collections=[]
    for event in root.structs(3):
        kind=event.number(0,"H");data=event.child(0)
        if kind==0:round_index=data.number(0)
        elif kind==1:turn=data.number(0);donor=None;pending_collections=[]
        elif kind==6 and "QUEEN_FEED" in data.text(0).split():
            did=data.number(0)
            item={"round_index":round_index,"donor":did,"log":data.text(0),
                  "queen_alive_at_log":qid in dragons,"queen_length_at_log":len(dragons.get(qid,[])),
                  "donor_body":list(dragons.get(did,[])),"donor_died":False,"pearl_cells_created":[],
                  "queen_collections":[]}
            entries.append(item);pending[did]=item
        elif kind==9:
            did=data.number(0);body=dragons[did];head=point(data.child(0))
            body.insert(0,head);tail=point(data.child(1))
            while len(body)>1 and body[-1]!=tail:body.pop()
            if did==qid:
                for tile,source in pending_collections:
                    if tile==head:
                        source["queen_collections"].append({"round_index":round_index,"tile":tile,
                                                           "queen_length_after_update":len(body)})
                pending_collections=[]
        elif kind==10:
            parent,child=data.number(0),data.number(4)
            dragons[parent]=[point(p) for p in data.structs(0)]
            dragons[child]=[point(p) for p in data.structs(1)]
            teams[child]="AB"[data.number(8,"H")]
        elif kind==11:
            did=data.number(0)
            if did in pending:
                donor=pending[did];donor["donor_died"]=True
                donor["death_reason"]=("wall","self","body","head","invalid_action")[data.number(4,"H")]
                donor["queen_alive_at_donor_death"]=qid in dragons
            dragons.pop(did,None)
        elif kind==3:
            tile=point(data.child(0));present=bool(data.number(0,"B")&1)
            if present and donor is not None and tile in donor["donor_body"]:
                donor["pearl_cells_created"].append(tile);sources[tile]=donor
            elif not present and tile in sources:
                source=sources.pop(tile)
                if turn==qid:
                    pending_collections.append((tile,source))
    return entries

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--output",type=Path,default=HERE/"feed_mechanism.json")
    args=ap.parse_args()
    if args.output.exists():ap.error("Output exists; preserve it and select a new --output")
    feed=json.loads((HERE/"queen_feed_v127/verified_audit.json").read_text())["games"]
    queen=json.loads((HERE/"queen_v127/verified_audit.json").read_text())["games"]
    feed_entries=[]
    scan_only=HERE/"feed_mechanism_scan_only.json"
    previous=json.loads(scan_only.read_text()) if scan_only.exists() else None
    selected={e["game"] for e in previous["feed"]["entries"]} if previous else None
    for game in feed:
        if selected is not None and game["game"] not in selected:continue
        for entry in collection_scan(Path(game["replay"]),game["bot1_side"]):
            feed_entries.append({"game":game["game"],"opponent":game["opponent"],"map":game["map"],
                                 "replay":game["replay"],"queen_alive_final":game["standings"][game["bot1_side"]]["queen"]>0,
                                 **entry})
    deaths=[g["queen_deaths"][g["bot1_side"]] for g in queen if g["queen_deaths"][g["bot1_side"]] is not None]
    details=[]
    for game in queen:
        if game["map"] not in ("default","portals"):continue
        result=scan(Path(game["replay"]),game["bot1_side"])
        d=result["queen_death"]
        if d and d["last_split"]:
            d["rounds_since_last_split"]=d["round_index"]-d["last_split"]["round_index"]
        details.append({"game":game["game"],"opponent":game["opponent"],"map":game["map"],
                        "side":game["bot1_side"],"outcome":game["outcome"],"replay":game["replay"],**result})
    output={"feed":{"games_checked":len(feed),"queen_feed_logs":len(feed_entries),
                    "unique_games_with_feed":len({e["game"] for e in feed_entries}),
                    "entries":feed_entries,
                    "donors_that_died":sum(e["donor_died"] for e in feed_entries),
                    "logged_feeds_with_observed_queen_collection":sum(bool(e["queen_collections"]) for e in feed_entries),
                    "directly_observed_queen_pearl_collections":sum(len(e["queen_collections"]) for e in feed_entries),
                    "games_with_observed_queen_collection":len({e["game"] for e in feed_entries if e["queen_collections"]}),
                    "feed_games_with_queen_alive_final":len({e["game"] for e in feed_entries if e["queen_alive_final"]}),
                    "collection_evidence":"A donated pearl is a tile-change creation on the dead donor's old body during its death turn; collection requires a later queen update to that tile and same-turn pearl removal. This is a conservative directly observed subset, not whole-body collection."},
            "queen_v127":{"games":len(queen),"queen_survived":len(queen)-len(deaths),
                          "queen_deaths_before_round_index_50":sum(d["round_index"]<50 for d in deaths),
                          "queen_deaths_before_round_index_100":sum(d["round_index"]<100 for d in deaths),
                          "queen_deaths_before_round_index_200":sum(d["round_index"]<200 for d in deaths),
                          "queen_death_reasons":dict(Counter(d["reason"] for d in deaths)),
                          "default_portals_details":details},
            "method":"QUEEN_FEED is counted in decoded official replay dragon-log events, not unverbose CLI logs. Emergency-split antecedents are temporal observations, not proven causal effects.",
            "script_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with args.output.open("x") as handle:json.dump(output,handle,indent=2);handle.write("\n")
    print(json.dumps({"output":str(args.output),"feed":{k:v for k,v in output["feed"].items() if k!="entries"},
                      "queen_v127":{k:v for k,v in output["queen_v127"].items() if k!="default_portals_details"}},indent=2))
    return 0
if __name__=="__main__":raise SystemExit(main())
