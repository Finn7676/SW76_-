#!/usr/bin/env python3
"""Summarize source-frozen audited batches, retaining side/map weaknesses."""
import argparse
import json
from collections import Counter
from pathlib import Path
from statistics import mean, stdev

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
OPPONENTS = ("V3_portal", "xxqztest")
CODE_SUFFIXES = (".cpp", ".hpp", ".cc", ".hh", ".c", ".h", ".toml")


def load(label):
    directory = RUNS / label
    audit = json.loads((directory / "verified_audit.json").read_text())
    config = json.loads((directory / "run.json").read_text())
    assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68, label
    assert config["unswbc"] == "1.2.7" and len(config["selected_maps"]) == 17
    return config, audit["games"]


def code(config, index):
    return {k: v for k, v in config["sources"][index]["files"].items() if k.endswith(CODE_SUFFIXES)}


def key(game):
    return (game["opponent"], game["map"], int(game["seed"]), game["bot1_side"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--group", action="append", nargs="+", required=True,
                        help="VERSION LABEL [LABEL...] ; first group is reference")
    parser.add_argument("--output-prefix", required=True, type=Path)
    args = parser.parse_args()
    outputs = [args.output_prefix.with_suffix(s) for s in (".json", ".md")]
    if any(p.exists() for p in outputs):
        parser.error("Choose a fresh output prefix; prior reports are retained")
    results, groups, records = {}, {}, {}
    reference_config = None
    for group in args.group:
        if len(group) < 2 or group[0] in results:
            parser.error("Each unique group needs a version name and >=1 label")
        name, labels = group[0], group[1:]
        batches = [load(label) for label in labels]
        first = batches[0][0]
        reference_config = reference_config or first
        assert len({c["base_seed"] for c, _ in batches}) == len(batches)
        for config, _ in batches:
            assert code(config, 0) == code(first, 0), name
            assert config["engine_sha256"] == reference_config["engine_sha256"]
            assert {m["name"]: m["sha256"] for m in config["maps"]} == {
                m["name"]: m["sha256"] for m in reference_config["maps"]}
            assert all(code(config, i) == code(reference_config, i) for i in (1, 2))
            assert not config["sandbox"], "Do not mix native and sandbox outcomes"
        games = [g for _, batch in batches for g in batch]
        groups[name] = games
        records[name] = {key(g): g for g in games}
        assert len(records[name]) == len(games)
        row = {"labels": labels, "games": len(games), "seeds": [c["base_seed"] for c, _ in batches],
               "candidate_code_sha256": code(first, 0), "by_opponent": {},
               "old10_wins": sum(g["outcome"] == "win" for g in games if g["legacy_subset"].lower() == "true")}
        for opponent in OPPONENTS:
            local = [g for g in games if g["opponent"] == opponent]
            rates = [sum(g["outcome"] == "win" for g in batch if g["opponent"] == opponent) / 34
                     for _, batch in batches]
            row["by_opponent"][opponent] = {
                "wins": sum(g["outcome"] == "win" for g in local), "games": len(local),
                "mean_rate": mean(rates), "sample_std": stdev(rates) if len(rates) > 1 else None,
                "range": [min(rates), max(rates)], "batch_rates": rates,
                "sides": {s: {"wins": sum(g["outcome"] == "win" for g in local if g["bot1_side"] == s),
                              "games": len(local) // 2} for s in ("A", "B")},
                "maps": {m: {"wins": sum(g["outcome"] == "win" for g in local if g["map"] == m),
                             "games": 2 * len(batches)} for m in first["selected_maps"]}}
        results[name] = row
    baseline = args.group[0][0]
    comparisons = {}
    for name in results:
        if name == baseline:
            continue
        keys = set(records[name])
        assert keys <= set(records[baseline]), "Candidate seeds must have matched baseline"
        local = {o: {"gained": 0, "lost": 0, "matched": 0} for o in OPPONENTS}
        for k in keys:
            old, new = records[baseline][k], records[name][k]
            item = local[k[0]]
            item["matched"] += 1
            item["gained"] += old["outcome"] != "win" and new["outcome"] == "win"
            item["lost"] += old["outcome"] == "win" and new["outcome"] != "win"
        comparisons[name] = local
    lines = ["# Multi-seed XXQZ Results", "", "Audited native1.2.7; draws non-wins. Development comparison, no p-values.",
             "Different versions are never pooled. Variants with different batch counts are compared only on matched seeds.",
             "", "| Version | Seeds | V3 Wins | xx Wins | xx Mean +/- Sample Std | xx A Wins | xx B Wins |",
             "|---|---|---:|---:|---:|---:|---:|"]
    for name, row in results.items():
        v3, xx = (row["by_opponent"][o] for o in OPPONENTS)
        spread = "NA" if xx["sample_std"] is None else f'{100*xx["sample_std"]:.1f}%'
        lines.append(f'| {name} | {row["seeds"]} | {v3["wins"]}/{v3["games"]} | '
                     f'{xx["wins"]}/{xx["games"]} | {100*xx["mean_rate"]:.1f}% +/- {spread} | '
                     f'{xx["sides"]["A"]["wins"]}/{xx["sides"]["A"]["games"]} | '
                     f'{xx["sides"]["B"]["wins"]}/{xx["sides"]["B"]["games"]} |')
    lines += ["", "## Paired Changes", "", "| Candidate | Opponent | Matched Games | Gained Wins | Lost Wins |",
              "|---|---|---:|---:|---:|"]
    for name, local in comparisons.items():
        for o, values in local.items():
            lines.append(f'| {name} | {o} | {values["matched"]} | {values["gained"]} | {values["lost"]} |')
    for name, row in results.items():
        lines += ["", f"## {name} Map Breakdown", "", "| Map | V3 Wins | xx Wins |", "|---|---:|---:|"]
        for m in row["by_opponent"][OPPONENTS[0]]["maps"]:
            v, x = (row["by_opponent"][o]["maps"][m] for o in OPPONENTS)
            lines.append(f'| {m} | {v["wins"]}/{v["games"]} | {x["wins"]}/{x["games"]} |')
    payload = {"phase": "development", "reference": baseline, "results": results,
               "paired": comparisons, "total_audited_games": sum(r["games"] for r in results.values())}
    args.output_prefix.parent.mkdir(parents=True, exist_ok=True)
    with outputs[0].open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    with outputs[1].open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"outputs": [str(p) for p in outputs], "games": payload["total_audited_games"]}))


if __name__ == "__main__":
    main()
