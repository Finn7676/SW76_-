#!/usr/bin/env python3
"""Aggregate audited replay mechanism records without pooling bot variants."""
import argparse
from collections import Counter
import json
from pathlib import Path

from action_donation_mechanism import summarize, sprint_summary

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--group", action="append", nargs="+", required=True)
    parser.add_argument("--output-prefix", type=Path, required=True)
    args = parser.parse_args()
    outputs = [args.output_prefix.with_suffix(s) for s in (".json", ".md")]
    if any(p.exists() for p in outputs):
        parser.error("Use a fresh output prefix to preserve prior evidence")
    groups, summaries, batches = {}, {}, {}
    for group in args.group:
        name, labels = group[0], group[1:]
        if not labels or name in groups:
            parser.error("Each unique version needs one or more suite labels")
        records = [json.loads((HERE / "mechanisms" / f"{label}.json").read_text()) for label in labels]
        assert len({r["base_seed"] for r in records}) == len(labels)
        assert len({r["candidate"] for r in records}) == 1
        assert len({r["script_sha256"] for r in records}) == 1
        games = [g for r in records for g in r["games"]]
        keys = {(g["opponent"], g["map"], g["seed"], g["side"]) for g in games}
        assert len(keys) == len(games)
        groups[name] = games
        summaries[name] = {
            "all": summarize(games),
            "by_opponent": {o: summarize([g for g in games if g["opponent"] == o])
                            for o in ("V3_portal", "xxqztest")},
            "nonqueen_ge3": sprint_summary([s for g in games for s in g["sprints"]
                                              if s["requested_steps"] >= 3 and not s["queen"]]),
            "long_attack_details": [
                {"label": r["label"], "game": g["game"], "opponent": g["opponent"],
                 "map": g["map"], "seed": g["seed"], "side": g["side"],
                 "outcome": g["outcome"], "replay": g["replay"], **s}
                for r in records for g in r["games"] for s in g["sprints"]
                if s["requested_steps"] >= 3 and s["enemy_head_hit"]],
        }
        batches[name] = [{"label": r["label"], "base_seed": r["base_seed"],
                          "by_opponent": r["by_opponent"]} for r in records]
    first = groups[args.group[0][0]]
    keys = {(g["opponent"], g["map"], g["seed"], g["side"]) for g in first}
    assert all(keys == {(g["opponent"], g["map"], g["seed"], g["side"])
                        for g in games} for games in groups.values())
    lines = ["# Sprint And Donation Mechanisms", "", "Official replay events from matched, source-frozen native 1.2.7 suites; all selected games previously passed scoring and manifest audits.", "",
             "Requests with >=3 steps include free harvest and queen-survival actions. They are not counted as proactive attacks unless the official same-turn death events record an actor/enemy head collision. A body death is the engine's category; this script does not invent the other body's owner.", "",
             "| Version | Opponent | Games | >=2 Requests | >=2 Head Hits | >=3 Requests | >=3 Head Hits | >=3 Body Deaths | >=3 Self Deaths |",
             "|---|---|---:|---:|---:|---:|---:|---:|---:|"]
    for name, row in summaries.items():
        for opponent, item in row["by_opponent"].items():
            s2, s3 = item["sprints_ge2"], item["sprints_ge3"]
            lines.append(f'| {name} | {opponent} | {item["games"]} | {s2["requests"]} | {s2["enemy_head_hits"]} | {s3["requests"]} | {s3["enemy_head_hits"]} | {s3["body_deaths"]} | {s3["death_reasons"].get("self", 0)} |')
    lines += ["", "## Head-Hit Timing", "", "| Version | Steps | Rounds 0-99 | 100-199 | 200-299 | 300-399 | 400-499 | Initial Lengths |",
              "|---|---|---:|---:|---:|---:|---:|---|"]
    for name, row in summaries.items():
        for key, label in (("sprints_ge2", ">=2"), ("sprints_ge3", ">=3")):
            item = row["all"][key]
            timing = item["head_hit_round_buckets"]
            lines.append(f'| {name} | {label} | ' + " | ".join(str(timing.get(k, 0)) for k in
                ("0-99", "100-199", "200-299", "300-399", "400-499")) +
                f' | {json.dumps(item["head_hit_initial_lengths"], sort_keys=True)} |')
    lines += ["", "## Consolidation Pickup", "", "Collections require directly observed donated-pearl creation, later removal, and collector movement to that tile during the same turn. These are pearl cells, not guaranteed total length gain or complete-corpse consumption.", "",
              "| Version | Opponent | Feed Logs | Intended Collections | Intended Head Collections | Feeds With Pickup | Other-Friendly Collections | Enemy Collections | Route Logs | Route Donor-Game Pairs |",
              "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for name, row in summaries.items():
        for opponent, item in row["by_opponent"].items():
            f = item["consolidate"]
            lines.append(f'| {name} | {opponent} | {f["feed_logs"]} | {f["intended_receiver_collections"]} | {f["intended_receiver_head_collections"]} | {f["feeds_with_receiver_collection"]} | {f["other_friendly_collections"]} | {f["enemy_collections"]} | {item["route_logs"]} | {item["unique_route_donor_game_pairs"]} |')
    lines += ["", "## Route Association", "", "| Version | Prior-Route Feeds | Intended Collections | Feeds With Pickup |",
              "|---|---:|---:|---:|"]
    for name, row in summaries.items():
        f = row["all"]["route_associated_feeds"]
        lines.append(f'| {name} | {f["feed_logs"]} | {f["intended_receiver_collections"]} | {f["feeds_with_receiver_collection"]} |')
    lines += ["", "Association means the same donor logged DONOR_ROUTE toward that receiver in the previous 20 rounds. It is not a counterfactual causal estimate.", "", "## Long Attack Events", ""]
    for name, row in summaries.items():
        details = row["long_attack_details"]
        lines.append(f"### {name}")
        lines.append("")
        if not details:
            lines.append("No >=3-step request caused an observed enemy-head collision.")
        else:
            counts = Counter(d["map"] for d in details)
            lines.append("Map hit counts: " + json.dumps(dict(counts), sort_keys=True))
            for d in details[:12]:
                enemy = [e for e in d["deaths"] if e["side"] != d["side"] and e["reason"] == "head"]
                lines.append(f'- {d["label"]} game {d["game"]}, {d["opponent"]}, {d["map"]}, side {d["side"]}, round index {d["round_index"]}: actor {d["id"]} initial length {d["length_before_action"]}, requested {d["requested_steps"]}, succeeded {d["successful_steps"]}, enemy {json.dumps(enemy)}; game {d["outcome"]}.')
        lines.append("")
    output = {"summaries": summaries, "batches": batches,
              "selected_labels": {group[0]: group[1:] for group in args.group}}
    with outputs[0].open("x") as handle:
        json.dump(output, handle, indent=2)
        handle.write("\n")
    with outputs[1].open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"outputs": [str(p) for p in outputs], "versions": list(groups)}))


if __name__ == "__main__":
    main()
