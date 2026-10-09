#!/usr/bin/env python3
"""Describe audited, paired new-rule runs without significance/stability claims."""
from __future__ import annotations
import argparse
from collections import Counter
import json
from pathlib import Path
from statistics import mean, median

def metrics(games):
    result={"games": len(games), **dict(Counter(g["outcome"] for g in games))}
    result["win_rate"]=sum(g["outcome"]=="win" for g in games)/len(games) if games else None
    sides=[(g["standings"][g["bot1_side"]], g["standings"]["B" if g["bot1_side"]=="A" else "A"]) for g in games]
    result["queen_survived"]=sum(ours["queen"]>0 for ours,_ in sides)
    result["opponent_queen_survived"]=sum(theirs["queen"]>0 for _,theirs in sides)
    for key in ("queen", "longest", "total"):
        result["mean_"+key]=mean(ours[key] for ours,_ in sides) if sides else None
        result["opponent_mean_"+key]=mean(theirs[key] for _,theirs in sides) if sides else None
    result["mean_queen_splits"]=mean(g["queen_splits"][g["bot1_side"]] for g in games) if games else None
    queen_deaths=[g["queen_deaths"][g["bot1_side"]] for g in games if g["queen_deaths"][g["bot1_side"]] is not None]
    result["queen_death_reasons"]=dict(Counter(q["reason"] for q in queen_deaths))
    result["median_queen_death_round_index"]=median(q["round_index"] for q in queen_deaths) if queen_deaths else None
    result["median_queen_length_before_death"]=median(q["length_before_death"] for q in queen_deaths) if queen_deaths else None
    result["invalid_action_deaths"]=sum(g["deaths"][g["bot1_side"]]["invalid_action"] for g in games)
    result["opponent_invalid_action_deaths"]=sum(g["deaths"]["B" if g["bot1_side"]=="A" else "A"]["invalid_action"] for g in games)
    return result

def key(game):
    return (game["opponent"],game["map"],game["seed"],game["bot1_side"])

def comparison(baseline, candidate):
    a={key(g):g for g in baseline}; b={key(g):g for g in candidate}
    if set(a)!=set(b):
        raise ValueError("Runs are not matched by opponent/map/seed/side")
    promotions=[]; regressions=[]; changes=[]
    for k in sorted(a):
        left,right=a[k],b[k]
        if left["outcome"]!=right["outcome"]:
            item={"opponent": k[0], "map": k[1], "seed": k[2], "side": k[3],
                  "baseline": left["outcome"], "candidate": right["outcome"],
                  "baseline_replay": left["replay"], "candidate_replay": right["replay"]}
            changes.append(item)
            if right["outcome"]=="win": promotions.append(item)
            elif left["outcome"]=="win": regressions.append(item)
    return {"matched_games":len(a), "promotions_to_win":promotions, "regressions_from_win":regressions,
            "all_outcome_changes":changes}

def examples(games):
    losses=[g for g in games if g["outcome"]=="loss"]
    chosen=[]
    # Declared selection: requested maps first, one loss per opponent/map, A before B.
    for opponent in sorted({g["opponent"] for g in games}):
        for map_name in ("default","portals"):
            eligible=sorted((g for g in losses if g["opponent"]==opponent and g["map"]==map_name), key=lambda g:g["bot1_side"])
            if eligible: chosen.append((eligible[0],"requested-map loss; side A first"))
        if not any(g["opponent"]==opponent for g,_ in chosen):
            eligible=[g for g in losses if g["opponent"]==opponent]
            if eligible:
                def deficit(g):
                    our=g["standings"][g["bot1_side"]]["queen"]
                    other=g["standings"]["B" if g["bot1_side"]=="A" else "A"]["queen"]
                    return (other-our,g["map"],g["bot1_side"])
                chosen.append((max(eligible,key=deficit),"largest final queen deficit; lexical tie break"))
    return [{"opponent":g["opponent"],"map":g["map"],"side":g["bot1_side"],"seed":g["seed"],
             "selection":selection,"replay":g["replay"],"standings":g["standings"],
             "queen_deaths":g["queen_deaths"],"end_reason":g["end_reason"]} for g,selection in chosen]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("audits",nargs="+",type=Path,help="First audit is baseline; following audits are candidates")
    ap.add_argument("--output-prefix",required=True,type=Path)
    args=ap.parse_args()
    output_json=args.output_prefix.with_suffix(".json"); output_md=args.output_prefix.with_suffix(".md")
    if output_json.exists() or output_md.exists(): ap.error("Comparison output exists; choose a new prefix")
    packages=[json.loads(path.read_text()) for path in args.audits]
    if any(not p["summary"]["audit_passed"] for p in packages): ap.error("Every input must have audit_passed=true")
    data={"scope":"One deterministic pearl seed per ranked map, A/B paired; descriptive development comparison only",
          "runs":[], "comparisons":[]}
    text=["# Queen-rule Experiment Comparison", "", data["scope"], "",
          "| Version | Opponent | W/L/D | Queen Alive | Mean Queen | Opponent Mean Queen | Legacy W/L/D |",
          "|---|---|---:|---:|---:|---:|---:|"]
    for package,path in zip(packages,args.audits):
        label=package["summary"]["label"]; games=package["games"]
        item={"label":label,"audit":str(path.resolve()),"overall":metrics(games),"by_opponent":{},
              "by_map":{},"representative_losses":examples(games)}
        for opponent in sorted({g["opponent"] for g in games}):
            subset=[g for g in games if g["opponent"]==opponent]
            met=metrics(subset); legacy=metrics([g for g in subset if str(g["legacy_subset"]).lower()=="true"])
            item["by_opponent"][opponent]={"ranked17":met,"legacy10":legacy,
                                              "side_A":metrics([g for g in subset if g["bot1_side"]=="A"]),
                                              "side_B":metrics([g for g in subset if g["bot1_side"]=="B"])}
            text.append(f'| {label} | {opponent} | {met.get("win",0)}/{met.get("loss",0)}/{met.get("draw",0)} | '
                        f'{met["queen_survived"]}/{met["games"]} | {met["mean_queen"]:.2f} | '
                        f'{met["opponent_mean_queen"]:.2f} | {legacy.get("win",0)}/{legacy.get("loss",0)}/{legacy.get("draw",0)} |')
            for map_name in sorted({g["map"] for g in subset}):
                item["by_map"][opponent+":"+map_name]=metrics([g for g in subset if g["map"]==map_name])
        data["runs"].append(item)
        if package is not packages[0]:
            paired=comparison(packages[0]["games"],games)
            paired.update(baseline=packages[0]["summary"]["label"],candidate=label)
            data["comparisons"].append(paired)
    text += ["", "## Paired Changes", ""]
    for item in data["comparisons"]:
        text.append(f'{item["candidate"]} versus {item["baseline"]}: {len(item["promotions_to_win"])} changes to a win; '
                    f'{len(item["regressions_from_win"])} former wins became non-wins.')
    text += ["", "## Representative Losses", "", "Selection is fixed: default/portals loss per opponent, side A before B; otherwise largest final queen deficit.", ""]
    for run in data["runs"]:
        text.append("### "+run["label"]); text.append("")
        for example in run["representative_losses"]:
            side=example["side"]; other="B" if side=="A" else "A"
            ours=example["standings"][side]; theirs=example["standings"][other]
            text.append(f'- {example["opponent"]}, {example["map"]}, side {side}: '
                        f'queen {ours["queen"]}:{theirs["queen"]}, longest {ours["longest"]}:{theirs["longest"]}, '
                        f'total {ours["total"]}:{theirs["total"]}; [replay]({example["replay"]}).')
        if not run["representative_losses"]: text.append("No losses in this run.")
        text.append("")
    args.output_prefix.parent.mkdir(parents=True,exist_ok=True)
    with output_json.open("x") as handle: json.dump(data,handle,indent=2); handle.write("\n")
    with output_md.open("x") as handle: handle.write("\n".join(text)+"\n")
    print(json.dumps({"json":str(output_json),"markdown":str(output_md),"runs":len(packages)}))
    return 0
if __name__=="__main__": raise SystemExit(main())
