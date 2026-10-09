#!/usr/bin/env python3
"""Summarize audited development and preregistered H14 confirmation results."""
import argparse
import json
from pathlib import Path
from statistics import mean, stdev

HERE = Path(__file__).resolve().parent
OPPONENTS = ("V3_portal", "xxqztest")
DEVELOPMENT = (
    ("B0", "counter_baseline"), ("H1", "queen_v127"),
    ("H2", "queen_feed_v127"), ("H3", "queen_guard_v127"),
    ("H4", "queen_lookahead_v127"), ("H5", "queen_consolidate_v127"),
    ("H6", "queen_cap32_v127"), ("H7", "queen_margin_v127"),
    ("H8", "queen_mature350_v127"), ("H9", "queen_feed_mature350_v127"),
    ("H10", "queen_guard_mature350_v127"), ("H11", "queen_consolidate_mature350_v127"),
    ("H12", "queen_mature300_v127"), ("H13", "queen_consolidate_mature300_v127"),
    ("H14", "queen_guard_consolidate_v127"), ("H15", "queen_consolidate_scout_v127"),
)


def read(label):
    directory = HERE / label
    package = json.loads((directory / "verified_audit.json").read_text())
    manifest = json.loads((directory / "run.json").read_text())
    assert package["summary"]["audit_passed"], label
    assert package["summary"]["games"] == 68, label
    assert manifest["unswbc"] == "1.2.7" and len(manifest["selected_maps"]) == 17
    games = package["games"]
    result = {"label": label, "audit": str(directory / "verified_audit.json"),
              "engine_sha256": manifest["engine_sha256"],
              "candidate_code_sha256": {k: v for k, v in manifest["sources"][0]["files"].items()
                                         if k.endswith((".cpp", ".hpp", ".cc", ".hh", ".c", ".h", ".toml"))},
              "opponent_code_sha256": [{k: v for k, v in source["files"].items()
                                          if k.endswith((".cpp", ".hpp", ".cc", ".hh", ".c", ".h", ".toml"))}
                                        for source in manifest["sources"][1:]],
              "map_sha256": {m["name"]: m["sha256"] for m in manifest["maps"]},
              "base_seed": manifest["base_seed"], "by_opponent": {},
              "legacy_wins": sum(g["outcome"] == "win" for g in games
                                 if str(g["legacy_subset"]).lower() == "true"),
              "queen_survived": sum(g["standings"][g["bot1_side"]]["queen"] > 0 for g in games),
              "by_map": {}}
    for opponent in OPPONENTS:
        subset = [g for g in games if g["opponent"] == opponent]
        assert len(subset) == 34, (label, opponent)
        result["by_opponent"][opponent] = {
            "wins": sum(g["outcome"] == "win" for g in subset), "games": len(subset),
            "draws": sum(g["outcome"] == "draw" for g in subset),
            "side_A_wins": sum(g["outcome"] == "win" for g in subset if g["bot1_side"] == "A"),
            "side_B_wins": sum(g["outcome"] == "win" for g in subset if g["bot1_side"] == "B"),
        }
        result["by_map"][opponent] = {}
        for map_name in result["map_sha256"]:
            local = [g for g in subset if g["map"] == map_name]
            assert len(local) == 2, (label, opponent, map_name)
            result["by_map"][opponent][map_name] = {
                "wins": sum(g["outcome"] == "win" for g in local), "games": 2}
    result["joint_over_half"] = all(result["by_opponent"][o]["wins"] > 17 for o in OPPONENTS)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("development", "confirmation"), required=True)
    parser.add_argument("--output-prefix", type=Path, required=True)
    args = parser.parse_args()
    json_path = args.output_prefix.with_suffix(".json")
    md_path = args.output_prefix.with_suffix(".md")
    if json_path.exists() or md_path.exists():
        parser.error("Output exists; choose a new prefix to retain earlier evidence")
    if args.phase == "development":
        runs = []
        missing = []
        for hypothesis, label in DEVELOPMENT:
            if not (HERE / label / "verified_audit.json").exists():
                missing.append({"hypothesis": hypothesis, "label": label})
                continue
            row = read(label)
            row["hypothesis"] = hypothesis
            assert row["base_seed"] == 78001
            runs.append(row)
        report = {"phase": "development", "runs": runs, "not_yet_audited": missing,
                  "scope": "One development seed per map; no independent confirmation claim."}
        lines = ["# Development Results", "", report["scope"], "",
                 "| ID | Version | V3 Wins/34 | xx Wins/34 | Old10 Wins/40 | Queen Alive/68 | Joint Gate |",
                 "|---|---|---:|---:|---:|---:|---|"]
        for row in runs:
            v3, xx = (row["by_opponent"][o]["wins"] for o in OPPONENTS)
            lines.append(f'| {row["hypothesis"]} | {row["label"]} | {v3} | {xx} | '
                         f'{row["legacy_wins"]} | {row["queen_survived"]} | '
                         f'{"PASS development only" if row["joint_over_half"] else "not passed"} |')
        lines += ["", f'Audited main games: {len(runs) * 68}. Smoke games are separate.',
                  "Every failed candidate is retained; no wins are pooled across different bots."]
    else:
        runs = {name: [read(f"{name}_confirm{seed}") for seed in (79001, 80001, 81001)]
                for name in ("h14", "counter")}
        assert len({r["engine_sha256"] for group in runs.values() for r in group}) == 1
        for name, group in runs.items():
            assert all(r["candidate_code_sha256"] == group[0]["candidate_code_sha256"] for r in group), name
            assert [r["base_seed"] for r in group] == [79001, 80001, 81001], name
        frozen = runs["h14"][0]
        assert frozen["candidate_code_sha256"]["main.cpp"] == "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
        assert runs["counter"][0]["candidate_code_sha256"]["main.cpp"] == "ceaa0183760c064b25141cb8f7cbe2d73213050a760942db0768419572529873"
        for group in runs.values():
            for row in group:
                assert row["map_sha256"] == frozen["map_sha256"]
                assert row["opponent_code_sha256"] == frozen["opponent_code_sha256"]
        aggregates = {}
        for name, group in runs.items():
            aggregates[name] = {}
            for opponent in OPPONENTS:
                wins = [r["by_opponent"][opponent]["wins"] for r in group]
                rates = [w / 34 for w in wins]
                aggregates[name][opponent] = {
                    "wins": sum(wins), "games": 102, "win_rate": sum(wins) / 102,
                    "batch_wins": wins, "batch_rate_mean": mean(rates),
                    "batch_rate_sample_std": stdev(rates), "batch_rate_range": [min(rates), max(rates)],
                    "side_A_wins": sum(r["by_opponent"][opponent]["side_A_wins"] for r in group),
                    "side_B_wins": sum(r["by_opponent"][opponent]["side_B_wins"] for r in group),
                    "by_map": {m: {"wins": sum(r["by_map"][opponent][m]["wins"] for r in group),
                                   "games": 6} for m in frozen["map_sha256"]}}
        passed = all(aggregates["h14"][o]["wins"] > 51 for o in OPPONENTS)
        deltas = {}
        for opponent in OPPONENTS:
            delta = [h["by_opponent"][opponent]["wins"] / 34 - c["by_opponent"][opponent]["wins"] / 34
                     for h, c in zip(runs["h14"], runs["counter"])]
            deltas[opponent] = {"win_rate_difference": mean(delta), "batch_delta_sample_std": stdev(delta),
                               "relative_win_increase": (aggregates["h14"][opponent]["wins"] /
                                                         aggregates["counter"][opponent]["wins"] - 1)}
        report = {"phase": "confirmation", "runs": runs, "aggregates": aggregates,
                  "deltas_vs_reference": deltas,
                  "joint_confirmation_passed": passed,
                  "scope": "Three preregistered unused seed batches; draws are non-wins; no p-values."}
        lines = ["# H14 Confirmation", "", report["scope"], "",
                 "| Version | Seed Base | V3 Wins/34 | xx Wins/34 | Old10 Wins/40 |",
                 "|---|---:|---:|---:|---:|"]
        for name, group in runs.items():
            for row in group:
                v3, xx = (row["by_opponent"][o]["wins"] for o in OPPONENTS)
                lines.append(f'| {name} | {row["base_seed"]} | {v3} | {xx} | {row["legacy_wins"]} |')
        lines += ["", "| Version | Opponent | Total Wins/102 | Batch Mean +/- Std | Batch Range |",
                  "|---|---|---:|---:|---:|"]
        for name, results in aggregates.items():
            for opponent, row in results.items():
                lo, hi = row["batch_rate_range"]
                lines.append(f'| {name} | {opponent} | {row["wins"]} | '
                             f'{100*row["batch_rate_mean"]:.1f}% +/- {100*row["batch_rate_sample_std"]:.1f}% | '
                             f'{100*lo:.1f}% to {100*hi:.1f}% |')
        lines += ["", f'Preregistered joint confirmation gate: {"PASS" if passed else "FAIL"}.',
                  "Native confirmation does not establish all-map sandbox compliance or victory on every map.",
                  "", "## Side Breakdown", "",
                  "| Version | Opponent | Side A Wins/51 | Side B Wins/51 |",
                  "|---|---|---:|---:|"]
        for name, results in aggregates.items():
            for opponent, row in results.items():
                lines.append(f'| {name} | {opponent} | {row["side_A_wins"]} | {row["side_B_wins"]} |')
        lines += ["", "H14 remains below half on side B against both opponents. The registered gate pools sides.",
                  "", "## Map Breakdown", "",
                  "| Map | H14 V3 Wins/6 | Counter V3 Wins/6 | H14 xx Wins/6 | Counter xx Wins/6 |",
                  "|---|---:|---:|---:|---:|"]
        for m in frozen["map_sha256"]:
            values = [aggregates[name][o]["by_map"][m]["wins"]
                      for o in OPPONENTS for name in ("h14", "counter")]
            lines.append(f'| {m} | ' + " | ".join(str(v) for v in values) + " |")
        lines += ["", "## Reference Difference", "",
                  "Three paired seed batches; descriptive statistics only, no hypothesis test."]
        for opponent, row in deltas.items():
            lines.append(f'- {opponent}: win-rate difference {100*row["win_rate_difference"]:.1f} percentage points '
                         f'+/- {100*row["batch_delta_sample_std"]:.1f} across batches; '
                         f'relative wins {100*row["relative_win_increase"]:.1f}% higher in this sample.')
    with json_path.open("x") as handle:
        json.dump(report, handle, indent=2)
        handle.write("\n")
    with md_path.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"json": str(json_path), "markdown": str(md_path)}))


if __name__ == "__main__":
    main()
