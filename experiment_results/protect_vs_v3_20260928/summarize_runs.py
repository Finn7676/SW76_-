#!/usr/bin/env python3
"""Summarize completed games and paired outcomes without treating failures as losses.

Example:
  python3 summarize_runs.py economy_dev economy_champion_dev \
    --baseline ../protect_local_20260927/full_v1 --output summary.json

Keys are (opponent, seed, map, bot1_side). Win rate means wins/completed;
draws stay in the denominator. Seed standard deviation is the sample SD of
per-seed win rates, not a binomial standard error or significance test.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import statistics
import subprocess
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path

VALID = {"win", "loss", "draw"}
POINTS = {"win": 1.0, "draw": 0.5, "loss": 0.0}
KEYS = ("opponent", "seed", "map", "bot1_side")
SKIP_DIRS = {".git", ".unswbc-build", "__pycache__", ".venv", "venv"}


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def load_rows(run_dir):
    with (run_dir / "results.csv").open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    seen = set()
    for row in rows:
        key = tuple(row[k] for k in KEYS)
        if key in seen:
            raise ValueError(f"duplicate condition in {run_dir}: {key}")
        seen.add(key)
    return rows


def counts(rows):
    c = Counter(row.get("outcome", "") for row in rows)
    completed = sum(c[k] for k in VALID)
    return {
        "recorded": len(rows), "completed": completed,
        "wins": c["win"], "losses": c["loss"], "draws": c["draw"],
        "failures": len(rows) - completed,
        "win_rate": c["win"] / completed if completed else None,
    }


def group_counts(rows, fields):
    groups = defaultdict(list)
    for row in rows:
        groups[tuple(row[field] for field in fields)].append(row)
    return [dict(zip(fields, key), **counts(value)) for key, value in sorted(groups.items())]


def seed_stats(rows):
    output = []
    for opponent in sorted({r["opponent"] for r in rows}):
        selected = [r for r in rows if r["opponent"] == opponent]
        seed_rows = group_counts(selected, ["seed"])
        values = [r["win_rate"] for r in seed_rows if r["win_rate"] is not None]
        condition_sets = {
            seed: sorted((r["map"], r["bot1_side"]) for r in selected
                         if r["seed"] == seed and r["outcome"] in VALID)
            for seed in sorted({r["seed"] for r in selected})
        }
        output.append({
            "opponent": opponent, "n_seeds": len(values), "by_seed": seed_rows,
            "mean_win_rate": statistics.mean(values) if values else None,
            "sample_std_win_rate": statistics.stdev(values) if len(values) > 1 else None,
            "same_completed_conditions_each_seed": len({json.dumps(v) for v in condition_sets.values()}) <= 1,
            "test": "descriptive only; no significance test",
        })
    return output


def paired(rows, baseline_rows):
    baseline = {tuple(r[k] for k in KEYS): r for r in baseline_rows}
    pairs, unmatched, failed = [], [], []
    for row in rows:
        key = tuple(row[k] for k in KEYS)
        old = baseline.get(key)
        if old is None:
            unmatched.append(dict(zip(KEYS, key)))
        elif row["outcome"] not in VALID or old["outcome"] not in VALID:
            failed.append(dict(zip(KEYS, key), baseline=old["outcome"], candidate=row["outcome"]))
        else:
            pairs.append(dict(zip(KEYS, key), baseline=old["outcome"], candidate=row["outcome"],
                              candidate_replay=row.get("replay"), baseline_replay=old.get("replay")))
    def summarize(items):
        transitions = Counter(f'{r["baseline"]}->{r["candidate"]}' for r in items)
        before = counts([{"outcome": r["baseline"]} for r in items])
        after = counts([{"outcome": r["candidate"]} for r in items])
        return {
            "pairs": len(items), "baseline": before, "candidate": after,
            "win_delta": after["wins"] - before["wins"],
            "win_rate_delta": after["win_rate"] - before["win_rate"] if items else None,
            "loss_to_win": transitions["loss->win"], "win_to_loss": transitions["win->loss"],
            "transitions": dict(sorted(transitions.items())),
            "improved_points": sum(POINTS[r["candidate"]] > POINTS[r["baseline"]] for r in items),
            "regressed_points": sum(POINTS[r["candidate"]] < POINTS[r["baseline"]] for r in items),
        }
    output = summarize(pairs)
    output["by_opponent"] = [dict(opponent=o, **summarize([r for r in pairs if r["opponent"] == o]))
                             for o in sorted({r["opponent"] for r in pairs})]
    output.update(unmatched=unmatched, failed_pairs=failed, conditions=pairs)
    return output


def provenance(run_dir):
    config_path = run_dir / "run.json"
    config = json.loads(config_path.read_text()) if config_path.is_file() else {}
    snapshots = {}
    source_dir = run_dir / "sources"
    if source_dir.is_dir():
        for path in sorted(source_dir.rglob("*")):
            relative = path.relative_to(run_dir)
            if path.is_file() and not set(relative.parts) & SKIP_DIRS:
                snapshots[str(relative)] = digest(path)
    maps = []
    for filename, recorded in sorted(config.get("maps", {}).items()):
        path = Path(filename)
        actual = digest(path) if path.is_file() else None
        maps.append({"path": filename, "recorded_sha256": recorded,
                     "current_sha256": actual, "matches_recorded": actual == recorded})
    return {"run_config": config, "snapshot_file_sha256": snapshots, "map_hash_checks": maps}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dirs", nargs="+", type=Path)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    baseline_rows = load_rows(args.baseline) if args.baseline else None
    git = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True, text=True)
    output = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "git_commit_at_summary_time": git.stdout.strip() if git.returncode == 0 else None,
        "git_commit_note": "Current commit only; does not identify uncommitted sources at match time. Use snapshot hashes.",
        "baseline": str(args.baseline.resolve()) if args.baseline else None,
        "metric_definition": __doc__, "runs": [],
    }
    for run_dir in args.run_dirs:
        rows = load_rows(run_dir)
        item = {
            "run_dir": str(run_dir.resolve()), "overall": counts(rows),
            "by_opponent": group_counts(rows, ["opponent"]),
            "by_map": group_counts(rows, ["opponent", "map"]),
            "by_seed": group_counts(rows, ["opponent", "seed"]),
            "by_seed_map": group_counts(rows, ["opponent", "seed", "map"]),
            "seed_statistics": seed_stats(rows),
            "failures": [r for r in rows if r["outcome"] not in VALID],
            "raw_results": rows, "provenance": provenance(run_dir),
        }
        if baseline_rows is not None:
            item["paired_to_baseline"] = paired(rows, baseline_rows)
        output["runs"].append(item)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output.resolve()),
                      "runs": [{"run_dir": r["run_dir"], **r["overall"]} for r in output["runs"]]},
                     ensure_ascii=False))


if __name__ == "__main__":
    main()
