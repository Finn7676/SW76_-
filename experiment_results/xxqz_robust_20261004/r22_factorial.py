#!/usr/bin/env python3
"""Matched 82001 attack-package by rear-allocation screen, descriptive only."""
from collections import Counter
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from compare import key, comparison, metrics
from analyze import sha, tree_hashes

CELLS = {
    "H14": ("robust_h14_82001", "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"),
    "R10": ("robust_queen_hunt_82001_retry1", "de27f3b2abb1440d75395c9676eb4d80967947bd7adb7e66f9570005f24ca8ba"),
    "R21": ("robust_rear_cascade_82001", "eb364e6e9ccf138a4b3b12d071e356a2ecee6300a4b0209675fe6888382f5573"),
    "R22": ("robust_queen_rear_82001", "43ed3e523628fb34f74279943f80408f149353d5a3440a6fbadf73cfe4ff8138"),
}


def load(label, digest):
    directory = RUNS / label
    config = json.loads((directory / "run.json").read_text())
    audit = json.loads((directory / "verified_audit.json").read_text())
    assert config["unswbc"] == "1.2.7" and config["base_seed"] == 82001
    assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
    assert config["sources"][0]["files"]["main.cpp"] == digest
    for source in config["sources"]:
        assert tree_hashes(Path(source["snapshot"])) == source["files"]
        assert sha(source["prepared"]) == source["prepared_sha256"]
    for record in config["maps"]:
        assert sha(record["snapshot"]) == record["sha256"]
    for game in audit["games"]:
        assert sha(game["replay"]) == game["replay_sha256"]
    return config, audit["games"], {"label": label, "main_sha256": digest,
        "run_json_sha256": sha(directory / "run.json"),
        "audit_json_sha256": sha(directory / "verified_audit.json")}


def main():
    output = HERE / "r22_factorial.json"
    markdown = output.with_suffix(".md")
    assert not output.exists() and not markdown.exists()
    runs = {name: load(*record) for name, record in CELLS.items()}
    configs = [record[0] for record in runs.values()]
    for config in configs[1:]:
        assert config["engine_sha256"] == configs[0]["engine_sha256"]
        assert {m["name"]: m["sha256"] for m in config["maps"]} == {
            m["name"]: m["sha256"] for m in configs[0]["maps"]}
        assert [s["prepared_sha256"] for s in config["sources"][1:]] == [
            s["prepared_sha256"] for s in configs[0]["sources"][1:]]
        assert set(map(key, runs["H14"][1])) == set(map(key, runs[config_name(config)][1]))
    lines = ["# R22 Allocation Interaction Screen", "",
        "Native unswbc 1.2.7, ranked17, paired A/B, development base82001 only. "
        "Four separately frozen candidates; no holdout or significance test.", "",
        "| Cell | V3 Wins | xx Wins | xx A | xx B |", "| --- | ---: | ---: | ---: | ---: |"]
    summary = {}
    for name, (_, games, _) in runs.items():
        summary[name] = {o: metrics([g for g in games if g["opponent"] == o])
            for o in ("V3_portal", "xxqztest")}
        xx = [g for g in games if g["opponent"] == "xxqztest"]
        seats = {s: sum(g["outcome"] == "win" for g in xx if g["bot1_side"] == s) for s in "AB"}
        lines.append(f'| {name} | {summary[name]["V3_portal"].get("win", 0)}/34 | '
            f'{summary[name]["xxqztest"].get("win", 0)}/34 | {seats["A"]}/17 | {seats["B"]}/17 |')
    comparisons = {}
    for baseline, candidate in (("H14", "R21"), ("R10", "R22"), ("H14", "R22")):
        label = f"{candidate}_versus_{baseline}"
        comparisons[label] = comparison(runs[baseline][1], runs[candidate][1])
    interaction = {}
    for opponent in ("V3_portal", "xxqztest"):
        wins = {n: summary[n][opponent].get("win", 0) for n in CELLS}
        interaction[opponent] = {
            "allocation_without_attack_win_delta": wins["R21"] - wins["H14"],
            "allocation_with_attack_win_delta": wins["R22"] - wins["R10"],
            "difference_in_differences_wins": wins["R22"] - wins["R10"] - wins["R21"] + wins["H14"],
        }
    lines += ["", "R10 is an attack package (exact sprint plus queen-target priority), "
        "not a pure queen-value factor. Rear allocation increases V3 wins by5 without "
        "that package but by3 with it. For xx the corresponding deltas are+1 and-2 "
        "wins; the descriptive interaction is-3/34, not an additive improvement.", "",
        "R22 versus R10 changes one xx loss to a win (slithery B), but loses three "
        "former wins (default A, schooltime A, slithery A). Against H14 it gains three "
        "xx wins and loses two. Its xx seats change from H14 10/6 to8/9: the B gain "
        "comes with A regression. These are matched outcomes, not causal explanations.", "",
        "Decision: retain the complete negative interaction screen; do not expand "
        "R22 to additional development or confirmation bases. xx total is exactly50%, "
        "A is below50%, and the package-parent comparison is negative. No source or "
        "metric will be retrofitted. Mechanism/cascade evidence is documented separately."]
    payload = {"scope": "Single development base82001; descriptive paired four-cell screen",
        "script_sha256": sha(__file__), "provenance": [r[2] for r in runs.values()],
        "by_cell": summary, "comparisons": comparisons, "interaction": interaction,
        "decision": "Do not expand, freeze as finalist, or use confirmation bases"}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    with markdown.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"summary": summary, "interaction": interaction, "output": str(output)}))


def config_name(config):
    return next(n for n, (label, _) in CELLS.items() if label == config["label"])


if __name__ == "__main__":
    main()
