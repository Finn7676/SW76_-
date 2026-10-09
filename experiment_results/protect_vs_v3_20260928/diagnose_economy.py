#!/usr/bin/env python3
"""Reconstruct cumulative body growth, splits and deaths with length conservation.

The engine emits one initial dragon-position notification before round 0 for
each initial dragon. These are checked against the map and never counted as
movement or growth. Runtime position events insert the new head and trim to
the reported tail. Positive/negative net body changes are kept separately;
positive growth is not gross pearl intake when sprint costs offset eating.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

PARSER_DIR = Path(__file__).resolve().parent.parent / "protect_local_20260927"
sys.path.insert(0, str(PARSER_DIR))
import analyze_replays as ar


def analyze(path, candidate_side, checkpoints):
    raw = path.read_bytes()
    root = ar.Message(raw).structure(0, 0)
    version = root.number(0, "I")
    if version > 2:
        raise ValueError(f"unsupported replay version {version}")
    width, height, dragons, teams = ar.parse_map(root.text(0))
    initial = {side: sum(len(body) for ident, body in dragons.items() if teams[ident] == side)
               for side in "AB"}
    initial_ids = set(dragons)
    initial_notified = set()
    stats = {side: Counter() for side in "AB"}
    deaths = {side: Counter() for side in "AB"}
    losses = {side: Counter() for side in "AB"}
    round_num, rows = -1, []

    def snapshot(checkpoint):
        for side in "AB":
            lengths = [len(body) for ident, body in dragons.items() if teams[ident] == side]
            s = stats[side]
            expected = initial[side] + s["growth"] - s["movement_shrink"] - s["death_length"]
            assert expected == sum(lengths), (path, checkpoint, side, expected, sum(lengths))
            rows.append({"team": side, "owner": "candidate" if side == candidate_side else "opponent",
                         "checkpoint": checkpoint, "completed_rounds": round_num + 1,
                         "initial_total_length": initial[side], "population": len(lengths),
                         "total_length": sum(lengths), "max_length": max(lengths, default=0),
                         **{key: s[key] for key in ["growth", "movement_shrink", "splits", "death_length",
                                                    "steps", "sprint_actions", "sprint_requested_extra_steps"]},
                         "deaths_by_reason": dict(deaths[side]), "death_length_by_reason": dict(losses[side]),
                         "length_conservation_ok": True})

    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            if round_num + 1 in checkpoints:
                snapshot(round_num + 1)
            round_num = data.number(0)
        elif kind == 4:
            ident, action = data.number(0), data.child(0)
            if action.number(0, "H") == 0:
                directions = action.uint16s(0)
                if len(directions) > 1:
                    stats[teams[ident]]["sprint_actions"] += 1
                    stats[teams[ident]]["sprint_requested_extra_steps"] += len(directions) - 1
        elif kind == 9:
            ident = data.number(0)
            head, tail = ar.point(data.child(0)), ar.point(data.child(1))
            body = dragons[ident]
            if round_num < 0:
                assert ident in initial_ids and ident not in initial_notified
                assert head == body[0] and tail == body[-1], (path, ident, "initial notification mismatch")
                initial_notified.add(ident)
                continue
            before = len(body)
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail, (path, round_num, ident, "tail not found")
            delta = len(body) - before
            stats[teams[ident]]["growth"] += max(0, delta)
            stats[teams[ident]]["movement_shrink"] += max(0, -delta)
            stats[teams[ident]]["steps"] += 1
        elif kind == 10:
            parent, child, team = data.number(0), data.number(4), "AB"[data.number(8, "H")]
            parent_body = [ar.point(p) for p in data.structs(0)]
            child_body = [ar.point(p) for p in data.structs(1)]
            assert len(dragons[parent]) == len(parent_body) + len(child_body), (path, round_num, parent, "split lost length")
            dragons[parent], dragons[child], teams[child] = parent_body, child_body, team
            stats[team]["splits"] += 1
        elif kind == 11:
            ident, reason = data.number(0), ar.DEATH_REASONS[data.number(4, "H")]
            side, length = teams[ident], len(dragons[ident])
            stats[side]["death_length"] += length
            deaths[side][reason] += 1
            losses[side][reason] += length
            del dragons[ident]
        elif kind > 12:
            raise ValueError(f"unknown event kind {kind}")
    if round_num + 1 in checkpoints:
        snapshot(round_num + 1)
    snapshot("final")
    result = root.child(4)
    for i, side in enumerate("AB"):
        standing = result.child(i)
        final = next(r for r in rows if r["checkpoint"] == "final" and r["team"] == side)
        expected = {"population": standing.number(0), "max_length": standing.number(4), "total_length": standing.number(8)}
        assert expected == {key: final[key] for key in expected}, (path, side, "final standings mismatch")
    return {"replay": str(path.resolve()), "replay_sha256": hashlib.sha256(raw).hexdigest(),
            "format_version": version, "width": width, "height": height,
            "candidate_side": candidate_side, "initial_total_length": initial,
            "initial_position_notifications_skipped": len(initial_notified),
            "all_initial_dragons_notified": initial_notified == initial_ids,
            "split_conservation_ok": True, "final_standings_ok": True, "checkpoints": rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dirs", nargs="+", type=Path)
    parser.add_argument("--maps", nargs="+", default=["schooltime"])
    parser.add_argument("--opponents", nargs="+", default=["V3_portal"])
    parser.add_argument("--seeds", nargs="+", type=str)
    parser.add_argument("--checkpoints", nargs="+", type=int, default=[50, 100, 200])
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    analyses = []
    for run_dir in args.run_dirs:
        with (run_dir / "results.csv").open(newline="", encoding="utf-8") as handle:
            rows = list(csv.DictReader(handle))
        for row in rows:
            if (row["outcome"] not in {"win", "loss", "draw"} or row["map"] not in args.maps
                    or row["opponent"] not in args.opponents or (args.seeds and row["seed"] not in args.seeds)):
                continue
            analysis = analyze(run_dir / row["replay"], row["bot1_side"], set(args.checkpoints))
            analysis.update({"run_dir": str(run_dir.resolve()), **{k: row[k] for k in ["game", "seed", "map", "opponent", "outcome"]}})
            analyses.append(analysis)
            print(f"validated {run_dir.name}/{row['replay']}", flush=True)
    groups = defaultdict(list)
    for game in analyses:
        for row in game["checkpoints"]:
            groups[(game["run_dir"], game["map"], game["opponent"], str(row["checkpoint"]), row["owner"])].append(row)
    means = []
    for key, rows in sorted(groups.items()):
        means.append(dict(zip(["run_dir", "map", "opponent", "checkpoint", "owner"], key), n=len(rows),
                          **{k: statistics.mean(r[k] for r in rows) for k in
                             ["growth", "movement_shrink", "splits", "death_length", "steps", "population",
                              "total_length", "max_length", "sprint_actions"]}))
    payload = {"metric_definition": __doc__, "games": analyses, "group_means": means,
               "note": "Descriptive statistics only; seeds and both sides are reported in raw games."}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output.resolve()), "games": len(analyses)}))


if __name__ == "__main__":
    main()
