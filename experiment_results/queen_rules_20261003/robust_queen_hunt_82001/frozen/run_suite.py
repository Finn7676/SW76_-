#!/usr/bin/env python3
"""Frozen, side-balanced unswbc 1.2.7 queen-rule comparisons.

The ranked pool has 17 maps: two opponents and both sides give 68 games.
The old ten-map subset is reported separately and contains 40 games.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import csv
import fcntl
import hashlib
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import time
from collections import Counter
from contextlib import contextmanager
from datetime import datetime, timezone
from importlib.metadata import version
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PYTHON = ROOT / ".venv-battlecode/bin/python"
if Path(sys.prefix) != PYTHON.parent.parent:
    os.execv(str(PYTHON), [str(PYTHON), str(Path(__file__).resolve()), *sys.argv[1:]])
sys.path.insert(0, str(ROOT))
import benchmark as bench
import unswbc

LEGACY_MAPS = ("autarky", "default", "devil", "portals", "dilemma", "queen_of_spades",
               "schooltime", "slithery_fight", "trauma", "trophy")
FIELDS = ["game", "opponent", "seed", "map", "legacy_subset", "bot1_side", "outcome", "winner",
          "rounds", "reason", "seconds", "replay", "log", "error", "replay_sha256", "log_sha256",
          "a_invalid_action_deaths", "b_invalid_action_deaths"]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def tree_hashes(path):
    return {p.relative_to(path).as_posix(): sha(p) for p in sorted(path.rglob("*"))
            if p.is_file() and "__pycache__" not in p.parts}


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


@contextmanager
def worker_slot():
    # Shared locks cap this experiment's concurrent CLI processes across suites.
    handles = [(HERE / f".worker_slot_{i}").open("a") for i in range(4)]
    chosen = None
    try:
        while chosen is None:
            for handle in handles:
                try:
                    fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
                    chosen = handle
                    break
                except BlockingIOError:
                    pass
            if chosen is None:
                time.sleep(0.1)
        yield
    finally:
        if chosen is not None:
            fcntl.flock(chosen, fcntl.LOCK_UN)
        for handle in handles:
            handle.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", required=True, help="New evidence directory; existing labels are refused")
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--maps", nargs="+", default=["all"], help="all/ranked, legacy, or explicit map names")
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--seed", type=int, default=78001)
    parser.add_argument("--timeout", type=float, default=300)
    parser.add_argument("--sandbox", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]*", args.label):
        parser.error("--label must be a simple unique directory name")
    if not 1 <= args.workers <= 4 or args.timeout <= 0:
        parser.error("--workers must be 1..4 and --timeout must be positive")
    toolkit_version = version("unswbc")
    if toolkit_version != "1.2.7":
        parser.error(f"This experiment is registered for unswbc 1.2.7, not {toolkit_version}")
    ranked = bench.resolve_maps(["all"])
    selected = bench.resolve_maps(list(LEGACY_MAPS) if args.maps == ["legacy"] else args.maps)
    if any(p.name not in {m.name for m in ranked} for p in selected):
        parser.error("This comparison supports only the frozen ranked17 pool")
    if not 0 <= args.seed <= 2**64 - len(ranked):
        parser.error("Seed range exceeds unsigned 64-bit range")
    out = HERE / args.label
    out.mkdir(exist_ok=False)
    frozen = out / "frozen"
    frozen.mkdir()
    sources = [bench.resolve_bot(args.candidate), bench.resolve_bot("tyhh_test/V3_portal"),
               bench.resolve_bot("tyhh_test/xxqztest")]
    os.environ["CXX"] = "g++-10"
    os.environ["UNSWBC_PYTHON"] = str(PYTHON)
    os.environ["UNSWBC_NO_UPDATE"] = "1"
    os.environ["UNSWBC_NO_VSCODE"] = "1"
    prepared = []
    source_records = []
    for index, source in enumerate(sources):
        destination = frozen / "bots" / f"{index}_{source.stem}"
        if source.is_file() and source.suffix != ".py":
            destination.mkdir(parents=True)
            target = destination / source.name
            shutil.copy2(source, target)
            binary = bench.prepare_bot(target, destination / "sources", args.sandbox)
        else:
            destination.parent.mkdir(parents=True, exist_ok=True)
            binary = bench.prepare_bot(source, destination, args.sandbox)
        prepared.append(binary)
        source_records.append({"original": str(source), "snapshot": str(destination),
                               "prepared": str(binary), "prepared_sha256": sha(binary),
                               "files": tree_hashes(destination)})
    map_dir = frozen / "maps"
    map_dir.mkdir()
    map_records = []
    for index, source in enumerate(ranked):
        target = map_dir / source.name
        shutil.copy2(source, target)
        map_records.append({"name": source.stem, "original": str(source), "snapshot": str(target),
                            "sha256": sha(target), "seed": args.seed + index,
                            "legacy_subset": source.stem in LEGACY_MAPS})
    engine_dir = frozen / "engine"
    engine_dir.mkdir()
    package = Path(unswbc.__file__).parent
    for source in package.glob("*.py"):
        shutil.copy2(source, engine_dir / source.name)
    shutil.copy2(package / "unswbc_engine.wasm", engine_dir / "unswbc_engine.wasm")
    shutil.copy2(ROOT / "benchmark.py", frozen / "benchmark.py")
    shutil.copy2(Path(__file__).resolve(), frozen / "run_suite.py")
    # Actual CLI still uses the explicit synchronized venv; snapshots preserve evidence.
    cli = ROOT / ".venv-battlecode/bin/unswbc"
    engine_original = package / "unswbc_engine.wasm"
    engine_hash = sha(engine_original)
    work = []
    selected_names = {p.stem for p in selected}
    for opponent_index in (1, 2):
        for item in map_records:
            if item["name"] in selected_names:
                for side in ("A", "B"):
                    work.append({"game": len(work) + 1, "opponent_index": opponent_index,
                                 "map_record": item, "side": side})
    config = {"label": args.label, "created_utc": datetime.now(timezone.utc).isoformat(),
              "unswbc": toolkit_version, "python": str(PYTHON), "cli": str(cli),
              "cli_sha256": sha(cli), "engine_original": str(engine_original),
              "engine_sha256": engine_hash, "engine_snapshot_files": tree_hashes(engine_dir),
              "candidate": str(sources[0]), "sources": source_records, "maps": map_records,
              "selected_maps": [m["name"] for m in map_records if m["name"] in selected_names],
              "base_seed": args.seed, "seed_policy": "base + ranked17 index; shared A/B, opponents and variants",
              "rules": "queen ID A=0/B=1; queen,longest,total; ceil(initial action length/4) free steps",
              "sandbox": args.sandbox, "workers": args.workers, "global_worker_cap": 4,
              "timeout_seconds": args.timeout, "planned": len(work), "legacy_maps": LEGACY_MAPS,
              "schedule": [{"game": t["game"], "opponent": sources[t["opponent_index"]].name,
                            "map": t["map_record"]["name"], "seed": t["map_record"]["seed"],
                            "side": t["side"]} for t in work],
              "native_limit_caveat": "Native runs do not enforce judge points/memory or bot random seeding"}
    write_json(out / "run.json", config)
    print(f"Frozen {len(ranked)} maps and {len(sources)} bots. Running {len(work)} games in {out}", flush=True)

    def run(task):
        oi, item, side = task["opponent_index"], task["map_record"], task["side"]
        opponent = sources[oi].name
        stem = f'{task["game"]:04d}_{opponent}_{item["name"]}_seed{item["seed"]}_{side}'
        replay, log = out / (stem + ".replay"), out / (stem + ".log")
        pair = [prepared[0], prepared[oi]] if side == "A" else [prepared[oi], prepared[0]]
        command = [str(cli), "run", item["snapshot"], *map(str, pair), "--seed", str(item["seed"]), "-o", str(replay)]
        if args.sandbox:
            command.append("--sandbox")
        if args.verbose:
            command.append("-v")
        row = {field: "" for field in FIELDS}
        row.update(game=task["game"], opponent=opponent, seed=item["seed"], map=item["name"],
                   legacy_subset=item["legacy_subset"], bot1_side=side, outcome="failed",
                   replay=replay.name, log=log.name)
        start = time.monotonic()
        try:
            with worker_slot():
                if sha(engine_original) != engine_hash or sha(Path(item["snapshot"])) != item["sha256"]:
                    raise ValueError("Engine or frozen map changed during suite")
                process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                           text=True, errors="replace", start_new_session=True)
                try:
                    output = process.communicate(timeout=args.timeout)[0]
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    output = process.communicate()[0]
                    log.write_text(output, encoding="utf-8")
                    raise ValueError(f"Match exceeded {args.timeout}s; spawned process group stopped")
            log.write_text(output, encoding="utf-8")
            if process.returncode:
                raise ValueError(f"CLI exit {process.returncode}")
            winner = bench.parse_result(output)
            if not replay.is_file() or not replay.stat().st_size:
                raise ValueError("Missing nonempty replay")
            clean = bench.ANSI.sub("", output)
            match = re.search(r"(?:team [AB] wins|draw) after (\d+) rounds[^\n]*", clean)
            row.update(winner=winner, outcome="draw" if winner == "draw" else "win" if winner == side else "loss",
                       rounds=match.group(1) if match else "", reason=match.group(0) if match else "")
            for team in "AB":
                row[f"{team.lower()}_invalid_action_deaths"] = len(re.findall(
                    rf"^round \d+: bot \d+ \(team {team}\) died: no valid action$", clean, re.MULTILINE))
            row["replay_sha256"] = sha(replay)
        except Exception as error:
            row["error"] = str(error)
            if not log.exists():
                log.write_text(str(error) + "\n", encoding="utf-8")
        row.update(seconds=round(time.monotonic() - start, 3), log_sha256=sha(log))
        return row

    rows = []
    with (out / "results.csv").open("x", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS)
        writer.writeheader()
        handle.flush()
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures = [pool.submit(run, task) for task in work]
            for future in concurrent.futures.as_completed(futures):
                row = future.result()
                rows.append(row)
                writer.writerow(row)
                handle.flush()
                print(f'[{len(rows)}/{len(work)}] {row["opponent"]} {row["map"]} {row["bot1_side"]}: '
                      f'{row["outcome"]} r{row["rounds"]} {row["error"]}', flush=True)
    mutation_errors = []
    for source in source_records:
        if tree_hashes(Path(source["snapshot"])) != source["files"]:
            mutation_errors.append(f'Frozen bot changed: {source["snapshot"]}')
    if sha(engine_original) != engine_hash:
        mutation_errors.append("Installed engine changed")
    for item in map_records:
        if sha(item["snapshot"]) != item["sha256"]:
            mutation_errors.append(f'Frozen map changed: {item["name"]}')
    summary = {"planned": len(work), "completed": len(rows), "mutation_errors": mutation_errors,
               "by_opponent": {sources[oi].name: dict(Counter(r["outcome"] for r in rows if r["opponent"] == sources[oi].name)) for oi in (1, 2)},
               "legacy_subset": {sources[oi].name: dict(Counter(r["outcome"] for r in rows if r["opponent"] == sources[oi].name and r["legacy_subset"])) for oi in (1, 2)}}
    write_json(out / "summary.json", summary)
    print(json.dumps(summary, indent=2), flush=True)
    return int(bool(mutation_errors) or any(r["outcome"] == "failed" for r in rows))


if __name__ == "__main__":
    raise SystemExit(main())
