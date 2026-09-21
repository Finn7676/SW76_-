#!/usr/bin/env python3
"""Interactive UNSW Battlecode benchmark. Run from your bots/maps directory.

Usage: python3 /path/to/benchmark.py
Requires `unswbc` on PATH. Each map receives an equal, even number of games.
Win rates mean wins / completed games (draws count in the denominator),
always from the first bot's perspective. Replays are saved beside this script under replays/<run>/.
"""

import re
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime
from pathlib import Path


ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
RESULT = re.compile(r"^(?:team ([AB]) wins|draw) after \d+ rounds\b", re.MULTILINE)


def parse_result(output):
    """Return A, B, or draw; refuse missing/ambiguous CLI results."""
    matches = list(RESULT.finditer(ANSI.sub("", output)))
    if len(matches) != 1:
        raise ValueError("Expected exactly one CLI match result.")
    return matches[0].group(1) or "draw"


def ask_bot(prompt):
    while True:
        value = input(prompt).strip()
        path = Path(value).expanduser()
        if value and path.exists():
            return path.resolve()
        print("Enter an existing bot project directory, Python file, or executable.")


def ask_maps():
    maps = []
    while True:
        prompt = "Map name " if not maps else "Add another map (name, or N to finish): "
        value = input(prompt).strip()
        if value.upper() == "N":
            if maps:
                return maps
            print("Please add at least one map.")
            continue
        if not value:
            continue
        path = Path(value).expanduser()
        candidates = [path, Path("maps") / path]
        if not path.suffix:
            candidates += [p.with_suffix(".map") for p in candidates[:]]
        found = next((p.resolve() for p in candidates if p.is_file()), None)
        if found is None:
            print("Map not found. Checked the current directory and maps/.")
        elif found in maps:
            print("That map is already included.")
        else:
            maps.append(found)
            print(f"Added {found.name}")


def ask_games():
    while True:
        value = input("Games per map (even number; default 20): ").strip() or "20"
        try:
            games = int(value)
            if games >= 2 and games % 2 == 0:
                return games
        except ValueError:
            pass
        print("Enter an even integer of at least 2, so each bot gets both sides equally.")


def schedule(maps, games):
    for _ in range(games // 2):
        for map_path in maps:
            for side in ("A", "B"):
                yield map_path, side


def percentage(results):
    return f"{100 * results.count('win') / len(results):.1f}%" if results else "N/A"


def summary(bots, maps, records, failed, planned):
    labels = [p.name for p in bots]
    if labels[0] == labels[1]:
        labels = [f"{labels[0]} (bot 1)", f"{labels[1]} (bot 2)"]
    outcomes = [r[2] for r in records]
    rows = [
        ("Total games", str(len(records))),
        (f"{labels[0]} wins", str(outcomes.count("win"))),
        (f"{labels[1]} wins", str(outcomes.count("loss"))),
        ("draws", str(outcomes.count("draw"))),
        ("", ""),
        (f"{labels[0]} winrate", percentage(outcomes)),
        ("", ""),
        ("Side A", percentage([r[2] for r in records if r[1] == "A"])),
        ("Side B", percentage([r[2] for r in records if r[1] == "B"])),
        ("", ""),
    ]
    for path in maps:
        label = path.name if sum(p.name == path.name for p in maps) == 1 else str(path)
        rows.append((label, percentage([r[2] for r in records if r[0] == path])))
    width = max(len(label) for label, _ in rows) + 3
    print("\n" + "=" * 40)
    print(f"{labels[0]} vs {labels[1]}")
    print("=" * 40)
    for label, value in rows:
        print(f"{label:<{width}}{value}" if label else "")
    print(f"\nWin rates above are for {labels[0]}; draws count as non-wins.")
    if len(records) != planned:
        print(f"Incomplete: {len(records)}/{planned} completed, {failed} failed.")
        print("Side counts may be unequal in this partial summary.")


def main():
    cli = shutil.which("unswbc")
    if cli is None:
        print("Cannot find unswbc on PATH. Install/activate the official CLI first.", file=sys.stderr)
        return 1
    try:
        bots = [ask_bot("First bot name/path: "), ask_bot("Second bot name/path: ")]
        maps = ask_maps()
        games = ask_games()
    except (EOFError, KeyboardInterrupt):
        print("\nCancelled.")
        return 130
    try:
        replay_root = Path(__file__).resolve().parent / "replays"
        replay_root.mkdir(parents=True, exist_ok=True)
        replay_dir = Path(tempfile.mkdtemp(
            prefix=datetime.now().strftime("%Y%m%d-%H%M%S-"), dir=replay_root))
    except OSError as error:
        print(f"Cannot create replay folder: {error}", file=sys.stderr)
        return 1
    print(f"\nReplay folder: {replay_dir}")
    planned = games * len(maps)
    print(f"\nRunning {planned} games: {games} per map, {games // 2} on each side.")
    records = []
    failed = 0
    interrupted = False
    try:
        for index, (map_path, side) in enumerate(schedule(maps, games), 1):
            ordered = bots if side == "A" else bots[::-1]
            map_label = re.sub(r"[^A-Za-z0-9_.-]+", "_", map_path.stem)[:80] or "map"
            replay_path = replay_dir / f"{index:04d}_{map_label}_bot1-side-{side}.replay"
            command = [cli, "run", str(map_path), *map(str, ordered), "-o", str(replay_path)]
            print(f"[{index}/{planned}] {map_path.name} | first bot on side {side} ...", flush=True)
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, errors="replace", check=False)
            try:
                if result.returncode != 0:
                    raise ValueError(f"CLI exited with code {result.returncode}.")
                winner = parse_result(result.stdout)
            except ValueError as error:
                failed += 1
                print(f"FAILED: {error}\n{result.stdout[-6000:]}", file=sys.stderr)
                print("Stopping so a configuration or CLI error does not repeat.")
                break
            outcome = "draw" if winner == "draw" else "win" if winner == side else "loss"
            records.append((map_path, side, outcome))
            print(f"  {outcome.upper()} for {bots[0].name} | replay: {replay_path.name}", flush=True)
    except KeyboardInterrupt:
        interrupted = True
        print("\nInterrupted; showing completed games.")
    except OSError as error:
        failed += 1
        print(f"Could not run CLI: {error}", file=sys.stderr)
    summary(bots, maps, records, failed, planned)
    print(f"Replay folder: {replay_dir}")
    return 130 if interrupted else 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
