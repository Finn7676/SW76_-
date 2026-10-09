#!/usr/bin/env python3
"""Audited H14/xx development queen head-death attribution, read-only."""
import hashlib
import json
from collections import Counter
from pathlib import Path
import sys

HERE=Path(__file__).resolve().parent
RUNS=HERE.parent/"queen_rules_20261003"
sys.path.insert(0,str(RUNS))
from analyze import Message,parse_map,point,sha,REASONS
from queen_immediate_witness import topology,visible_cells,project,continuation
from queen_immediate_engine import action

BASES=(82001,83001,84001)
H14_MAIN="ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
LETTERS="NESW"


def local_alternatives(snapshot,w,h,edges):
    own=snapshot["body_before"]
    visible=visible_cells(own[0],w,h)
    if len(set(own))!=len(own) or not set(own)<=visible:
        return {"eligible":False,"reason":"partial_or_nonunique_own_body"}
    for front,rear in zip(own,own[1:]):
        if not any(edges.get((rear,d),(0,-1))[0]==0 and
                   ((rear[0]+((0,1,0,-1)[d]))%w,(rear[1]+((-1,0,1,0)[d]))%h)==front
                   for d in range(4)):
            return {"eligible":False,"reason":"own_body_portal_or_nonordinary"}
    actual=snapshot["directions"]
    if not actual:
        return {"eligible":False,"reason":"queen_previous_action_not_move"}
    occupancy=set(map(tuple,snapshot["visible_other_cells"]))
    pearls=set(map(tuple,snapshot["visible_pearls"]))
    original=project(own,actual,occupancy,pearls,visible,edges,w,h)
    if not original["known"] or not original["survives"]:
        return {"eligible":False,"reason":"queen_previous_action_not_ordinary_known_living"}
    head=original["body"][0]
    incident=[{"dir":LETTERS[d],"portal_id":edges.get((head,d),(0,-1))[1]}
              for d in range(4) if edges.get((head,d),(0,-1))[0]==2]
    if not incident:
        return {"eligible":False,"reason":"queen_final_head_not_portal_incident"}
    maxsteps=min(4,(len(own)+3)//4)
    rows=[]
    stack=[()]
    while stack:
        path=stack.pop()
        if len(path)>=maxsteps:continue
        for d in range(4):
            trial=path+(d,)
            post=project(own,trial,occupancy,pearls,visible,edges,w,h)
            if not post["known"] or not post["survives"]:continue
            stack.append(trial)
            q=post["body"][0]
            if len(post["body"])!=len(original["body"]):continue
            if any(edges.get((q,e),(0,-1))[0]==2 for e in range(4)):continue
            # The endpoint itself must be in current view. All its edges are
            # shown there; hypothetical remote terrain is never used.
            if q not in visible:continue
            onward=continuation(post["body"],occupancy,visible,edges,w,h)
            if onward is None:continue
            rows.append({"action":"".join(LETTERS[e] for e in trial),"projection":post,
                         "ordinary_continuation":onward,"nonportal_endpoint":True})
    rows.sort(key=lambda row:(len(row["action"]),row["action"]))
    return {"eligible":True,"original_projection":original,"visible_portal_incident_edges":incident,
            "same_length_ordinary_alternatives":rows[:3],"alternative_count":len(rows),
            "limitations":["Current-view ordinary survival and continuation only, no enemy/friend future action prediction.",
                           "No hard/soft friendly-protection policy proof or global safety claim is made by offline geometry."]}


def scan(game):
    path=Path(game["replay"])
    assert sha(path)==game["replay_sha256"]
    root=Message(path.read_bytes()).structure(0,0)
    w,h,bodies,teams=parse_map(root.text(0))
    _,_,edges=topology(root.text(0))
    queen=int(game["bot1_side"]=="B")
    pearls=set();r=-1;turn=-1;turnaction=None;moves=0
    queenprevious=None;findings=[];turn_head_deaths=[]
    def snapshot(did,event_index,move):
        own=list(bodies[did]);visible=visible_cells(own[0],w,h)
        return {"round":r,"event":event_index,"action":move,"directions":[],
                "body_before":own,"body_after":list(own),
                "visible_other_cells":[cell for other,body in bodies.items() if other!=did for cell in body if cell in visible],
                "visible_other_heads":[{"id":other,"team":teams[other],"cell":body[0]}
                                       for other,body in bodies.items() if other!=did and body[0] in visible],
                "visible_pearls":sorted(pearls&visible)}
    def finish_turn():
        for record in findings:
            if record["death_turn_event"]==turnaction.get("turn_event") if turnaction else False:
                record["paired_head_deaths_same_turn"]=list(turn_head_deaths)
    turn_event=-1
    for index,event in enumerate(root.structs(3)):
        kind,data=event.number(0,"H"),event.child(0)
        if kind==0:
            r=data.number(0)
        elif kind==1:
            finish_turn()
            turn,turn_event=data.number(0),index
            turnaction=None;moves=0;turn_head_deaths=[]
        elif kind==3:
            cell=point(data.child(0))
            if data.number(0,"B")&1:pearls.add(cell)
            else:pearls.discard(cell)
        elif kind==4:
            did=data.number(0);item=data.child(0)
            turnaction={"id":did,"team":teams[did],"round":r,"action":action(data),
                        "event":index,"turn_event":turn_event,
                        "directions":item.uint16s(0) if item.number(0,"H")==0 else [],
                        "body_before":list(bodies[did])}
            if did==queen:
                queenprevious=snapshot(did,index,turnaction["action"])
                queenprevious["directions"]=list(turnaction["directions"])
        elif kind==9:
            did,head,tail=data.number(0),point(data.child(0)),point(data.child(1))
            before=bodies[did]
            # Before first turn, official position events repeat map state.
            if turn<0 and head==before[0] and tail==before[-1]:continue
            body=[head]+before
            while len(body)>1 and body[-1]!=tail:body.pop()
            assert body[-1]==tail
            bodies[did]=body
            if did==turn:moves+=1
            if did==queen and queenprevious is not None:
                queenprevious["body_after"]=list(body)
        elif kind==10:
            parent,child=data.number(0),data.number(4)
            bodies[parent]=[point(p) for p in data.structs(0)]
            bodies[child]=[point(p) for p in data.structs(1)]
            teams[child]="AB"[data.number(8,"H")]
            if parent==queen and queenprevious is not None:
                queenprevious["body_after"]=list(bodies[parent])
        elif kind==11:
            did,reason=data.number(0),REASONS[data.number(4,"H")]
            if reason=="head":
                turn_head_deaths.append({"id":did,"team":teams[did],"event":index,"head":bodies[did][0]})
            if did==queen and reason=="head":
                assert turnaction is not None
                actor=turnaction["id"]
                who="queen_initiated" if actor==queen else "friendly_initiated" if teams[actor]==teams[queen] else "enemy_initiated"
                actorhead=bodies[actor][0] if actor in bodies else turnaction["body_before"][0]
                d=turnaction["directions"][moves] if moves<len(turnaction["directions"]) else None
                edge=edges.get((actorhead,d),(0,-1)) if d is not None else None
                record={"round":r,"queen":queen,"death_event":index,"death_turn_event":turn_event,
                        "queen_head":bodies[queen][0],"queen_body":list(bodies[queen]),"initiator_class":who,
                        "acting_id":actor,"acting_team":teams[actor],"acting_action":dict(turnaction),
                        "successful_steps_before_collision":moves,"collision_from":actorhead,
                        "collision_direction":LETTERS[d] if d is not None else None,
                        "collision_edge_kind":edge[0] if edge is not None else None,
                        "collision_portal_id":edge[1] if edge is not None else None,
                        "queen_previous_action":queenprevious,
                        "friendly_incoming_portal":who=="friendly_initiated" and edge is not None and edge[0]==2,
                        "paired_head_deaths_same_turn":[]}
                if record["friendly_incoming_portal"] and queenprevious is not None:
                    record["current_queen_view_endpoint_analysis"]=local_alternatives(queenprevious,w,h,edges)
                    record["incoming_origin_visible_to_previous_queen"]=actorhead in visible_cells(queenprevious["body_before"][0],w,h)
                    record["queen_previous_action_same_round"]=queenprevious["round"]==r
                    record["queen_head_equals_previous_posthead"]=queenprevious["body_after"][0]==bodies[queen][0]
                findings.append(record)
            bodies.pop(did,None)
    finish_turn()
    standing={s:{"population":sum(teams[d]==s for d in bodies),"total":sum(len(b) for d,b in bodies.items() if teams[d]==s),
                 "longest":max((len(b) for d,b in bodies.items() if teams[d]==s),default=0),
                 "queen":len(bodies.get("AB".index(s),[]))} for s in "AB"}
    assert standing==game["standings"],(game["map"],game["seed"],standing,game["standings"])
    assert len(findings)==int(game["queen_deaths"][game["bot1_side"]] is not None and game["queen_deaths"][game["bot1_side"]]["reason"]=="head")
    return {"game":game["game"],"map":game["map"],"seed":int(game["seed"]),"side":game["bot1_side"],"outcome":game["outcome"],
            "replay":str(path),"replay_sha256":game["replay_sha256"],"final_standings_checked":True,"findings":findings}


def main():
    output=HERE/"r23_portal_endpoint_diagnosis.json"
    md=HERE/"r23_portal_endpoint_diagnosis.md"
    assert not output.exists() and not md.exists()
    games=[];provenance=[]
    for base in BASES:
        run=RUNS/f"robust_h14_{base}"
        config=json.loads((run/"run.json").read_text());audit=json.loads((run/"verified_audit.json").read_text())
        assert config["unswbc"]=="1.2.7" and not config["sandbox"] and config["sources"][0]["files"]["main.cpp"]==H14_MAIN
        assert audit["summary"]["audit_passed"] and len(audit["games"])==68
        selected=[g for g in audit["games"] if g["opponent"]=="xxqztest"]
        assert len(selected)==34
        provenance.append({"base":base,"run_json_sha256":sha(run/"run.json"),"audit_json_sha256":sha(run/"verified_audit.json"),
                           "source_main_sha256":H14_MAIN,"engine_sha256":config["engine_sha256"]})
        for g in sorted(selected,key=lambda g:int(g["game"])):
            games.append({"base":base,**scan(g)})
            print(f"H14/xx head-death scan {len(games)}/102",flush=True)
    findings=[(g,f) for g in games for f in g["findings"]]
    classes=Counter(f["initiator_class"] for _,f in findings)
    portal=[(g,f) for g,f in findings if f["friendly_incoming_portal"]]
    representatives=[{"base":g["base"],"map":g["map"],"seed":g["seed"],"side":g["side"],"outcome":g["outcome"],
                       "replay":g["replay"],"replay_sha256":g["replay_sha256"],"finding":f} for g,f in portal[:3]]
    payload={"passed":True,"scope":"102 audited unchanged H14/xx native dev games, three bases, both seats; no matches or bot edits",
             "source_sha256":sha(__file__),"parser_sha256":sha(RUNS.parent/"protect_local_20260927/analyze_replays.py"),
             "bases":BASES,"provenance":provenance,"queen_head_deaths":len(findings),"initiator_classes":dict(classes),
             "friendly_incoming_portal_count":len(portal),"representatives":representatives,"games":games,
             "limitations":["Queen actor ID is reported separately; own-initiated head hits are not attributed to an enemy acting later.",
                             "Replay board truth supports actual attribution; alternatives use only radius3 queen decision view.",
                             "Incoming portal endpoint detection does not reveal a remote dragon intent or certify overall safety.",
                             "Offline alternatives do not establish H14 policy activation, competitive benefit, or hard/soft friend-risk compliance.",
                             "No reserved/holdout seed or competitive engine run was used."]}
    with output.open("x") as handle:json.dump(payload,handle,indent=2);handle.write("\n")
    lines=["# R23 H14 Portal Endpoint Diagnosis","",payload["scope"],"",f"Queen head deaths: {len(findings)}. Initiator classes: {dict(classes)}. Friendly incoming portal deaths: {len(portal)}.",""]
    for row in representatives:
        f=row["finding"];view=f.get("current_queen_view_endpoint_analysis",{})
        lines += [f"## {row['map']} Seed{row['seed']} {row['side']}","",f"Queen{f['queen']} round{f['round']} dies head at {f['queen_head']} during friendly{f['acting_id']} {f['acting_action']['action']}; origin {f['collision_from']} direction{f['collision_direction']} portal{f['collision_portal_id']}.",
                  f"Queen previous action: {f['queen_previous_action']['action'] if f['queen_previous_action'] else None}; same round {f.get('queen_previous_action_same_round')}; remote origin currently visible {f.get('incoming_origin_visible_to_previous_queen')}.",
                  f"Current-view endpoint analysis: {view}.",""]
    if not portal:
        lines += ["No H14 friendly incoming-portal head-death witness was found under the exact attribution criteria. The R20-derived islands snapshot alone cannot establish this failure mode's presence or prevalence in H14. Do not force a candidate based on that counterfactual alone.",""]
    lines += ["Initial preturn position events that repeat map head/tail are not counted as body growth; every final standing was reconstructed and matched the audited replay. Inputs were not regenerated by an engine: current vision is the official radius3 around the queen before its recorded previous action, with all tile edges in that view. Alternatives are ordinary, same projected length, living and continuation-proven, not global enemy/friend safety certificates.","", "Full events/provenance are in `r23_portal_endpoint_diagnosis.json`; no holdout seed, bot or competitive match was touched."]
    with md.open("x") as handle:handle.write("\n".join(lines)+"\n")
    print(json.dumps({"passed":True,"queen_head_deaths":len(findings),"initiator_classes":dict(classes),"friendly_incoming_portal":len(portal),"output":str(output)}))


if __name__=="__main__":main()
