#!/usr/bin/env python3
"""UNSW Battlecode benchmark. Bot/map names resolve beside this script or the working directory.

Usage: python3 benchmark.py [xxqz SW_Champion --maps all --games 2]
Requires `unswbc` on PATH. Each map receives an equal, even number of games.
Win rates mean wins / completed games (draws count in the denominator),
always from the first bot's perspective. Replays are saved beside this script under replays/<run>/.

`--maps all` (or ranked) means the 17-map ranked baseline verified on
2026-10-03 from the official site's recent ranked games. `--maps bundled`
includes every toolkit map, including unranked and deprecated maps.
Synchronize changed maps using battlecode_quickstart/README.md first.
"""

import argparse
import ast
import csv
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from importlib.metadata import PackageNotFoundError, version
from datetime import datetime
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
# Prefer the synchronized official toolkit bundle; historical folders are
# available only for explicit map selection.
MAP_FOLDERS = ("battlecode_quickstart/maps", "maps_BeforeSprinter", "maps")
OFFICIAL_MAP_FILES = {
    "australia": ("australia.map",),
    "autarky": ("autarky.map",),
    "default": ("default.map",),
    "devil": ("devil.map",),
    "portals": ("portals.map", "Portals.map"),
    "dilemma": ("dilemma.map", "Prisoners Dilemma.map", "prisoners_dilemma.map"),
    "queen_of_spades": ("queen_of_spades.map", "Queen Of Spades.map"),
    "schooltime": ("schooltime.map", "Schooltime.map"),
    "slithery_fight": ("slithery_fight.map", "Slithery Fight.map"),
    "trauma": ("trauma.map", "Trauma.map"),
    "trophy": ("trophy.map", "Trophy.map"),
    "islands": ("islands.map",),
    "maze": ("maze.map",),
    "stripes": ("stripes.map",),
    "tower_defense": ("tower_defense.map", "Tower Defense.map"),
    "unsw": ("unsw.map", "Around UNSW.map"),
    "weakhold": ("weakhold.map",),
}
# Both samples of 100 recent ranked games used exactly these 17 maps.
RANKED_MAP_FILES = tuple(names[0] for names in OFFICIAL_MAP_FILES.values())
RANKED_MAP_CHECKED = "2026-10-03"
RANKED_MAP_SOURCE = "https://game.battlecode.au/games?kind=ranked-autoscrim,ranked-scrim"
# Aliases for display names that differ from filenames.
OFFICIAL_MAP_ALIASES = {
    alias: canonical
    for canonical, names in OFFICIAL_MAP_FILES.items()
    for alias in {canonical, *(Path(name).stem.lower().replace(" ", "_") for name in names)}
}
RUNTIME_ERROR = re.compile(
    r"^round \d+: bot \d+ \(team [AB]\) (?:exited\b|ran out of time\b|.*(?:points|memory) limit)",
    re.MULTILINE,
)
ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
RESULT = re.compile(r"^(?:team ([AB]) wins|draw) after \d+ rounds\b", re.MULTILINE)


def parse_result(output):
    """Return A, B, or draw; refuse missing/ambiguous CLI results."""
    clean = ANSI.sub("", output)
    error = RUNTIME_ERROR.search(clean)
    if error:
        raise ValueError(f"Bot runtime failure; not counted as a match: {error.group(0)}")
    matches = list(RESULT.finditer(clean))
    if len(matches) != 1:
        raise ValueError("Expected exactly one CLI match result.")
    return matches[0].group(1) or "draw"


def path_candidates(value, maps=False):
    path = Path(str(value).strip().strip("\"'")).expanduser()
    if path.is_absolute():
        roots = [path]
    else:
        bases = list(dict.fromkeys((Path.cwd(), SCRIPT_DIR)))
        roots = []
        for base in bases:
            roots.append(base / path)
            if maps:
                roots.extend(base / folder / path for folder in MAP_FOLDERS)
    if maps and not path.suffix:
        roots += [p.with_suffix(".map") for p in roots[:]]
    return list(dict.fromkeys(roots))


def resolve_bot(value):
    for path in path_candidates(value):
        if path.is_dir() and ((path / "bot.toml").is_file() or (path / "main.py").is_file()):
            return path.resolve()
        if path.is_file() and (path.suffix in (".py", ".wasm") or os.access(path, os.X_OK)):
            return path.resolve()
    raise ValueError(f"Bot not found or unsupported: {value}. Use a bot.toml project, main.py directory, .py file, or executable.")


def available_maps():
    found = {}
    for base in dict.fromkeys((Path.cwd(), SCRIPT_DIR)):
        for folder in MAP_FOLDERS:
            for path in (base / folder).glob("*.map"):
                if path.is_file() and path.name not in found:
                    found[path.name] = path.resolve()
    return [found[name] for name in sorted(found)]


def official_maps(bundled=False):
    """Resolve the ranked baseline or toolkit bundle, verifying file contents."""
    from unswbc.maps import BUNDLED
    names = sorted(p.name for p in BUNDLED.glob("*.map") if p.is_file()) if bundled else RANKED_MAP_FILES
    if not names:
        raise ValueError("The installed toolkit has no bundled maps.")
    found = []
    for name in names:
        path = SCRIPT_DIR / MAP_FOLDERS[0] / name
        template = BUNDLED / name
        if not path.is_file() or not template.is_file() or path.read_bytes() != template.read_bytes():
            raise ValueError(f"Official map missing or outdated: {name}. Synchronize the toolkit maps first (see battlecode_quickstart/README.md).")
        found.append(path.resolve())
    return found


def resolve_maps(values):
    maps = []
    for value in values:
        selection = str(value).strip().lower()
        if selection in ("all", "ranked", "bundled"):
            found = official_maps(bundled=selection == "bundled")
        else:
            normalized = str(value).strip().lower().replace(" ", "_")
            canonical = OFFICIAL_MAP_ALIASES.get(normalized)
            if canonical:
                names = OFFICIAL_MAP_FILES[canonical]
                path = next(
                    (base / folder / name
                     for base in dict.fromkeys((Path.cwd(), SCRIPT_DIR))
                     for folder in MAP_FOLDERS
                     for name in names
                     if (base / folder / name).is_file()),
                    None,
                )
            else:
                path = next((p for p in path_candidates(value, maps=True) if p.is_file() or p.is_dir()), None)
            found = sorted(path.glob("*.map")) if path and path.is_dir() else [path] if path else []
        if not found:
            raise ValueError(f"Map not found: {value}. Checked the official map set and local map folders.")
        for path in found:
            if path.resolve() not in maps:
                maps.append(path.resolve())
    return maps


def ask_bot(prompt):
    while True:
        value = input(prompt).strip()
        try:
            if value:
                return resolve_bot(value)
        except ValueError as error:
            print(error)
        print("Enter an existing bot project directory, Python file, or executable.")


def ask_maps():
    maps = []
    known = available_maps()
    if known:
        print("Available local maps: " + ", ".join(p.stem for p in known))
    while True:
        value = input("Map name (ALL = ranked, BUNDLED = toolkit): " if not maps else "Add another map (name, ALL, BUNDLED, or N to finish): ").strip()
        if value.upper() == "N":
            if maps:
                return maps
            print("Please add at least one map.")
            continue
        if not value:
            continue
        try:
            for path in resolve_maps([value]):
                if path not in maps:
                    maps.append(path)
                    print(f"Added {path.name}")
            if value.upper() in ("ALL", "RANKED", "BUNDLED"):
                return maps
        except ValueError as error:
            print(error)


def needs_helper(entry):
    tree = ast.parse(entry.read_text(encoding="utf-8-sig"), filename=str(entry))
    return any(
        (isinstance(node, ast.Import) and any(n.name == "helper" for n in node.names))
        or (isinstance(node, ast.ImportFrom) and node.module == "helper" and node.level == 0)
        for node in ast.walk(tree)
    )


def prepare_bot(source, destination, sandbox=False):
    """Snapshot sources, add only a missing official helper, and build once."""
    if source.is_file() and source.suffix != ".py":
        if sandbox and source.suffix != ".wasm":
            raise ValueError("Sandbox needs C/C++ sources, Python, or .wasm; a native executable cannot run there.")
        if not sandbox and source.suffix == ".wasm":
            raise ValueError("A .wasm bot requires --sandbox.")
        return source
    project_dir = source if source.is_dir() else source.parent
    def ignore(directory, names):
        excluded = {".git", ".unswbc-build", "__pycache__", ".venv", "venv", ".DS_Store"}
        return [n for n in names if n in excluded or destination.is_relative_to(Path(directory) / n)]
    shutil.copytree(project_dir, destination, ignore=ignore)
    try:
        import unswbc
        from unswbc.project import Project
        from unswbc.errors import UserError
        from unswbc import toolchain
    except ImportError as error:
        raise ValueError("Run benchmark with a Python interpreter that has unswbc installed.") from error
    try:
        project = Project.from_dir(destination) if (destination / "bot.toml").is_file() and source.is_dir() else None
    except UserError as error:
        raise ValueError(f"{source.name}: {error}") from error
    entry = destination / (source.name if source.is_file() else "main.py")
    if entry.is_file() and (project is None or project.language == "python"):
        helper = entry.parent / "helper.py"
        if needs_helper(entry) and not helper.exists() and not (entry.parent / "helper" / "__init__.py").exists():
            official = Path(unswbc.__file__).parent / "templates" / "python" / "helper.py"
            if not official.is_file():
                raise ValueError(f"{source} needs helper.py, but the installed toolkit has no Python helper template.")
            shutil.copy2(official, helper)
            print(f"  {source.name}: added official helper.py to the test copy (original unchanged).")
        # Fail on import/syntax/startup problems before counting any games.
        try:
            python = toolchain.find_python()
        except UserError as error:
            raise ValueError(str(error)) from error
        result = subprocess.run(python + [str(entry)],
                                input="ID 0\nTEAM A\nMAP 11 11\nUNIT_LIMIT 64\nENDGAME\n",
                                cwd=entry.parent, capture_output=True, text=True, timeout=15)
        if result.returncode:
            raise ValueError(f"{source.name} startup check failed:\n{result.stderr[-6000:]}")
        return entry
    if project is not None:
        try:
            if sandbox and project.language in ("c", "cpp"):
                from unswbc import clangtool
                return clangtool.build(destination)
            built = project.compile()
        except UserError as error:
            raise ValueError(f"{source.name} compilation failed: {error}") from error
        for name in (toolchain.BOT_NAME, toolchain.BOT_NAME + ".exe"):
            binary = built / name
            if binary.is_file():
                print(f"  {source.name}: compiled once for all matches.")
                return binary
    raise ValueError(f"Cannot prepare bot: {source}")


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


def main(argv=None):
    # CLI and preparation must import the same toolkit and build API.
    local_python = SCRIPT_DIR / ".venv-battlecode" / "bin" / "python"
    if local_python.is_file() and Path(sys.prefix) != local_python.parent.parent:
        os.execv(str(local_python), [str(local_python), str(Path(__file__).resolve()), *(sys.argv[1:] if argv is None else argv)])
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("bots", nargs="*", help="First and second bot paths; omit for interactive prompts")
    parser.add_argument("--maps", nargs="+", help="Map names/paths/directory, all or ranked (17 ranked maps), or bundled (all toolkit maps); defaults to all with two bot arguments")
    parser.add_argument("--games", type=int, help="Even number of games PER MAP; default 20")
    parser.add_argument("--verbose", action="store_true", help="Capture each bot's output in logs (useful for EXP_METRIC telemetry)")
    parser.add_argument("--seed", type=int, help="Base seed; each A/B pair shares a seed, subsequent pairs increment it")
    parser.add_argument("--sandbox", action="store_true", help="Use the judge's CPU points, memory and output limits")
    args = parser.parse_args(argv)
    if len(args.bots) > 2:
        parser.error("Specify at most two bots.")
    if args.games is not None and (args.games < 2 or args.games % 2):
        parser.error("--games must be an even integer of at least 2 (per map).")
    if args.seed is not None and not 0 <= args.seed < 2**64:
        parser.error("--seed must be an unsigned 64-bit integer.")
    local_cli = SCRIPT_DIR / ".venv-battlecode" / "bin" / "unswbc"
    cli = str(local_cli) if local_cli.is_file() else shutil.which("unswbc")
    if cli is None:
        print("Cannot find unswbc on PATH. Install/activate the official CLI first.", file=sys.stderr)
        return 1
    try:
        toolkit_version = version("unswbc")
        if tuple(int(part) for part in toolkit_version.split(".")[:3]) < (1, 2, 7):
            raise ValueError(f"unswbc {toolkit_version} is outdated; install >=1.2.7 for queen scoring and current maps.")
    except (ValueError, ImportError, PackageNotFoundError) as error:
        print(error, file=sys.stderr)
        return 1
    try:
        bots = [resolve_bot(value) for value in args.bots]
        while len(bots) < 2:
            bots.append(ask_bot("First bot name/path: " if not bots else "Second bot name/path: "))
        maps = resolve_maps(args.maps) if args.maps else resolve_maps(["all"]) if len(args.bots) == 2 else ask_maps()
        games = args.games if args.games is not None else 20 if len(args.bots) == 2 else ask_games()
        if args.seed is not None and args.seed + games // 2 * len(maps) - 1 >= 2**64:
            raise ValueError("The seed range for this schedule exceeds the unsigned 64-bit limit.")
    except (EOFError, KeyboardInterrupt):
        print("\nCancelled.")
        return 130
    except ValueError as error:
        print(error, file=sys.stderr)
        return 1
    try:
        replay_root = SCRIPT_DIR / "replays"
        replay_root.mkdir(parents=True, exist_ok=True)
        replay_dir = Path(tempfile.mkdtemp(prefix=datetime.now().strftime("%Y%m%d-%H%M%S-"), dir=replay_root))
        print(f"\nReplay folder: {replay_dir}")
        print("Preparing bots ...", flush=True)
        prepared = [prepare_bot(bot, replay_dir / f"bot{i+1}" / bot.stem, args.sandbox) for i, bot in enumerate(bots)]
        (replay_dir / "run.json").write_text(json.dumps({"bots": list(map(str, bots)), "prepared": list(map(str, prepared)),
            "maps": list(map(str, maps)), "map_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in maps},
            "games_per_map": games, "seed": args.seed, "unswbc": toolkit_version,
            "map_selection": args.maps or (["all"] if len(args.bots) == 2 else ["interactive"]),
            "ranked_map_baseline": {"checked": RANKED_MAP_CHECKED, "source": RANKED_MAP_SOURCE, "files": RANKED_MAP_FILES},
            "cli": cli, "rules": "queen-longest-total; free steps ceil(initial move length/4)",
            "sandbox": args.sandbox}, indent=2), encoding="utf-8")
    except (OSError, ValueError, RuntimeError, SyntaxError, subprocess.TimeoutExpired) as error:
        print(f"Cannot prepare benchmark: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\nCancelled during preparation.")
        return 130
    planned = games * len(maps)
    print(f"\nRunning {planned} games: {games} per map, {games // 2} on each side.")
    records = []
    failed = 0
    interrupted = False
    try:
        with (replay_dir / "results.csv").open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle)
            writer.writerow(["game", "map", "bot1_side", "outcome", "winner", "replay", "log", "error", "seed"])
            handle.flush()
            for index, (map_path, side) in enumerate(schedule(maps, games), 1):
                ordered = prepared if side == "A" else prepared[::-1]
                map_label = re.sub(r"[^A-Za-z0-9_.-]+", "_", map_path.stem)[:80] or "map"
                replay_path = replay_dir / f"{index:04d}_{map_label}_bot1-side-{side}.replay"
                log_path = replay_path.with_suffix(".log")
                command = [cli, "run", str(map_path), *map(str, ordered), "-o", str(replay_path)]
                if args.verbose:
                    command.append("-v")
                if args.sandbox:
                    command.append("--sandbox")
                if args.seed is not None:
                    seed = args.seed + (index - 1) // 2
                elif side == "A":
                    seed = int.from_bytes(os.urandom(8), "little")
                command.extend(["--seed", str(seed)])
                print(f"[{index}/{planned}] {map_path.name} | first bot on side {side} ...", flush=True)
                result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                        text=True, errors="replace", check=False)
                log_path.write_text(result.stdout, encoding="utf-8")
                try:
                    if result.returncode != 0:
                        raise ValueError(f"CLI exited with code {result.returncode}.")
                    winner = parse_result(result.stdout)
                    if not replay_path.is_file() or replay_path.stat().st_size == 0:
                        raise ValueError("CLI did not save a nonempty replay.")
                except ValueError as error:
                    failed += 1
                    writer.writerow([index, map_path, side, "failed", "", replay_path.name, log_path.name, str(error), seed])
                    handle.flush()
                    print(f"FAILED: {error}\n{result.stdout[-6000:]}", file=sys.stderr)
                    print("Stopping so a configuration or runtime error does not repeat.")
                    break
                outcome = "draw" if winner == "draw" else "win" if winner == side else "loss"
                records.append((map_path, side, outcome))
                writer.writerow([index, map_path, side, outcome, winner, replay_path.name, log_path.name, "", seed])
                handle.flush()
                print(f"  {outcome.upper()} for {bots[0].name} | replay: {replay_path.name}", flush=True)
    except KeyboardInterrupt:
        interrupted = True
        print("\nInterrupted; showing completed games.")
    except OSError as error:
        failed += 1
        print(f"Could not run CLI or save results: {error}", file=sys.stderr)
    summary(bots, maps, records, failed, planned)
    print(f"Replay folder: {replay_dir}")
    print(f"CSV results: {replay_dir / 'results.csv'}")
    return 130 if interrupted else 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
