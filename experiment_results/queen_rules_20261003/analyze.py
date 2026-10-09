#!/usr/bin/env python3
"""Audit frozen-suite CSV/replays, including the updated queen standings."""
from __future__ import annotations
import argparse, csv, hashlib, json, re, sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OLD = ROOT / "experiment_results/protect_local_20260927"
sys.path.insert(0, str(OLD))
from analyze_replays import Message, point, parse_map  # Cap'n Proto reader is schema-compatible

REASONS = ("wall", "self", "body", "head", "invalid_action")

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def tree_hashes(path):
    return {p.relative_to(path).as_posix(): sha(p) for p in sorted(path.rglob("*"))
            if p.is_file() and "__pycache__" not in p.parts}

def audit(path: Path, bot1_side: str = "A"):
    root = Message(path.read_bytes()).structure(0, 0)
    fmt = root.number(0, "I")
    if fmt > 2:
        raise ValueError(f"unsupported replay format {fmt}")
    width, height, dragons, teams = parse_map(root.text(0))
    if 0 not in dragons or 1 not in dragons:
        raise ValueError("initial queen IDs 0 and 1 are missing")
    deaths = {s: {r: 0 for r in REASONS} for s in "AB"}
    queen_deaths = {side: None for side in "AB"}
    queen_splits = {side: 0 for side in "AB"}
    last_actions = {}
    explicit_suicide_deaths = {side: 0 for side in "AB"}
    rounds = -1
    completed = 0
    for event in root.structs(3):
        kind = event.number(0, "H")
        data = event.child(0)
        if kind == 0:
            if rounds >= 0:
                completed += 1
            rounds = data.number(0)
        elif kind == 4:
            dragon_id = data.number(0)
            action = data.child(0)
            last_actions[dragon_id] = action.number(0, "H")
        elif kind == 9:
            dragon_id = data.number(0)
            if dragon_id not in dragons:
                raise ValueError(f"position update for missing dragon {dragon_id}")
            head, tail = point(data.child(0)), point(data.child(1))
            body = dragons[dragon_id]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            if body[-1] != tail:
                raise ValueError(f"tail mismatch for dragon {dragon_id}")
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            parent_body = [point(p) for p in data.structs(0)]
            child_body = [point(p) for p in data.structs(1)]
            dragons[parent], dragons[child] = parent_body, child_body
            teams[child] = "AB"[data.number(8, "H")]
            if parent in (0, 1):
                queen_splits["AB"[parent]] += 1
        elif kind == 11:
            dragon_id = data.number(0)
            if dragon_id in teams:
                reason = REASONS[data.number(4, "H")]
                side = teams[dragon_id]
                deaths[side][reason] += 1
                if reason == "invalid_action" and last_actions.get(dragon_id) == 2:
                    explicit_suicide_deaths[side] += 1
                if dragon_id in (0, 1):
                    queen_deaths[side] = {"round_index": rounds, "reason": reason,
                                          "length_before_death": len(dragons.get(dragon_id, []))}
            dragons.pop(dragon_id, None)
        elif kind > 12:
            raise ValueError(f"unknown event kind {kind}")
    completed += 1 if rounds >= 0 else 0
    result = root.child(4)
    if not result.number(0, "B") & 1:
        raise ValueError("replay is not terminated")
    end_reason = result.number(2, "H")
    winner = "AB"[result.number(6, "H")] if result.number(4, "H") == 1 else "draw"
    standings = {}
    for i, side in enumerate("AB"):
        standing = result.child(i)
        values = {"population": standing.number(0), "longest": standing.number(4),
                  "total": standing.number(8), "queen": standing.number(12)}
        bodies = [len(body) for did, body in dragons.items() if teams[did] == side]
        actual = {"population": len(bodies), "longest": max(bodies, default=0),
                  "total": sum(bodies), "queen": len(dragons.get(i, []))}
        if values != actual:
            raise ValueError(f"{side} final standings mismatch: replay={values}, rebuilt={actual}")
        standings[side] = values
    if end_reason == 0:
        expected = "A" if standings["A"]["population"] else "B" if standings["B"]["population"] else "draw"
        if standings["A"]["population"] and standings["B"]["population"]:
            raise ValueError("elimination result still contains both teams")
    elif end_reason == 1:
        scores = {s: tuple(standings[s][key] for key in ("queen", "longest", "total")) for s in "AB"}
        expected = "A" if scores["A"] > scores["B"] else "B" if scores["B"] > scores["A"] else "draw"
    else:
        raise ValueError(f"unsupported end reason {end_reason}")
    if winner != expected:
        raise ValueError(f"winner mismatch: replay={winner}, reconstructed={expected}")
    return {"replay": str(path.resolve()), "format_version": fmt, "width": width,
            "height": height, "completed_rounds": completed, "winner": winner,
            "bot1_side": bot1_side, "standings": standings, "deaths": deaths,
            "queen_deaths": queen_deaths, "queen_splits": queen_splits,
            "explicit_suicide_deaths": explicit_suicide_deaths, "end_reason": end_reason,
            "seed": root.number(8, "Q") if root.number(4, "H") == 1 else None,
            "bot_a": root.text(1), "bot_b": root.text(2),
            "map_sha256": hashlib.sha256(root.text(0).encode()).hexdigest(),
            "replay_sha256": sha(path)}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("run_dir", type=Path)
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()
    output = args.output or args.run_dir / "verified_audit.csv"
    json_output = output.with_suffix(".json")
    if output.exists() or json_output.exists():
        ap.error("Audit outputs already exist; choose a new --output to preserve evidence")
    config = json.loads((args.run_dir / "run.json").read_text())
    rows=[]; errors=[]; manifest_errors=[]; reports=[]
    for source in config["sources"]:
        snapshot=Path(source["snapshot"])
        if tree_hashes(snapshot) != source["files"]:
            manifest_errors.append(f"Frozen bot tree changed: {snapshot}")
        if sha(source["prepared"]) != source["prepared_sha256"]:
            manifest_errors.append(f'Frozen binary changed: {source["prepared"]}')
    for item in config["maps"]:
        if sha(item["snapshot"]) != item["sha256"]:
            manifest_errors.append(f'Frozen map changed: {item["snapshot"]}')
    if sha(config["cli"]) != config["cli_sha256"]:
        manifest_errors.append("Local CLI wrapper changed")
    if sha(config["engine_original"]) != config["engine_sha256"]:
        manifest_errors.append("Installed engine WASM changed")
    snapshot_engine=args.run_dir / "frozen/engine"
    if tree_hashes(snapshot_engine) != config["engine_snapshot_files"]:
        manifest_errors.append("Frozen engine/module tree changed")
    package=Path(config["engine_original"]).parent
    for name, digest in config["engine_snapshot_files"].items():
        if not (package / name).is_file() or sha(package / name) != digest:
            manifest_errors.append(f"Installed engine Python module changed: {name}")
    schedule={str(t["game"]):t for t in config["schedule"]}
    maps={m["name"]:m for m in config["maps"]}
    seen=set()
    failed=[]
    with (args.run_dir / "results.csv").open(newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            game=row.get("game", "")
            if game in seen:
                errors.append({"game": game, "error": "duplicate CSV game"})
            seen.add(game)
            task=schedule.get(game)
            if not task or any(str(row.get(field)) != str(task[key]) for field,key in
                               (("opponent","opponent"),("map","map"),("seed","seed"),("bot1_side","side"))):
                errors.append({"game": game, "error": "CSV differs from registered schedule"})
            if row.get("outcome") not in ("win", "loss", "draw"):
                failed.append(row)
                continue
            try:
                report=audit(args.run_dir / row["replay"], row["bot1_side"])
                if report["replay_sha256"] != row["replay_sha256"]:
                    raise ValueError("recorded replay hash mismatch")
                log=args.run_dir / row["log"]
                if sha(log) != row["log_sha256"]:
                    raise ValueError("recorded log hash mismatch")
                if report["winner"] != row["winner"]:
                    raise ValueError("CSV winner differs from replay")
                expected="draw" if report["winner"]=="draw" else "win" if report["winner"]==row["bot1_side"] else "loss"
                if row["outcome"] != expected:
                    raise ValueError("CSV outcome differs from replay winner/side")
                if int(row["rounds"]) != report["completed_rounds"]:
                    raise ValueError("CSV rounds differ from replay")
                if int(row["seed"]) != report["seed"]:
                    raise ValueError("CSV seed differs from replay")
                if report["map_sha256"] != maps[row["map"]]["sha256"]:
                    raise ValueError("Replay map differs from frozen map")
                prepared=[s["prepared"] for s in config["sources"]]
                oi=next(i for i,s in enumerate(config["sources"]) if i and Path(s["original"]).name==row["opponent"])
                pair=[prepared[0],prepared[oi]] if row["bot1_side"]=="A" else [prepared[oi],prepared[0]]
                if [report["bot_a"],report["bot_b"]] != pair:
                    raise ValueError("Replay bot ordering differs from frozen binaries")
                for side in "AB":
                    if int(row[f"{side.lower()}_invalid_action_deaths"]) != report["deaths"][side]["invalid_action"]:
                        raise ValueError("CLI/replay invalid-action death mismatch")
                report.update({key: row[key] for key in ("game", "opponent", "map", "seed", "outcome", "legacy_subset")})
                reports.append(report)
                row.update({"audit": "ok", "queen_a": report["standings"]["A"]["queen"],
                            "queen_b": report["standings"]["B"]["queen"],
                            "longest_a": report["standings"]["A"]["longest"],
                            "longest_b": report["standings"]["B"]["longest"],
                            "total_a": report["standings"]["A"]["total"],
                            "total_b": report["standings"]["B"]["total"],
                            "replay_format": report["format_version"],
                            "replay_sha256_actual": report["replay_sha256"],
                            "audit_json": json.dumps(report, separators=(",", ":"))})
            except Exception as exc:
                row.update(audit="failed", audit_error=str(exc)); errors.append({"game": row.get("game"), "error": str(exc)})
            rows.append(row)
    if seen != set(schedule):
        errors.append({"error": "schedule coverage mismatch", "missing": sorted(set(schedule)-seen),
                       "unexpected": sorted(seen-set(schedule))})
    output.parent.mkdir(parents=True, exist_ok=True)
    fields=[]
    for row in rows:
        for key in row:
            if key not in fields: fields.append(key)
    with output.open("x", newline="", encoding="utf-8") as f:
        writer=csv.DictWriter(f, fieldnames=fields); writer.writeheader(); writer.writerows(rows)
    summary={"run_dir": str(args.run_dir.resolve()), "label": config["label"],
             "created_utc": datetime.now(timezone.utc).isoformat(),
             "games": len(rows), "planned": config["planned"], "audit_failures": errors,
             "manifest_failures": manifest_errors, "failed_matches": failed,
             "wins": sum(r.get("outcome")=="win" for r in rows), "losses": sum(r.get("outcome")=="loss" for r in rows),
             "draws": sum(r.get("outcome")=="draw" for r in rows), "audit_csv": str(output.resolve())}
    by_opponent = {}
    for opponent in sorted({r.get("opponent", "") for r in rows}):
        subset = [r for r in rows if r.get("opponent") == opponent]
        by_opponent[opponent] = {k: sum(x.get("outcome") == k for x in subset) for k in ("win", "loss", "draw")}
    summary["by_opponent"] = by_opponent
    summary["legacy_subset_games"] = sum(str(r.get("legacy_subset", "")).lower() == "true" for r in rows)
    summary["audit_tool_sha256"]=sha(__file__)
    summary["packed_parser_sha256"]=sha(OLD / "analyze_replays.py")
    summary["audit_passed"]=not errors and not manifest_errors and not failed and len(rows)==config["planned"]
    with json_output.open("x", encoding="utf-8") as handle:
        json.dump({"summary": summary, "games": reports}, handle, indent=2)
        handle.write("\n")
    print(json.dumps(summary, indent=2))
    return 0 if summary["audit_passed"] else 1
if __name__ == "__main__": raise SystemExit(main())
