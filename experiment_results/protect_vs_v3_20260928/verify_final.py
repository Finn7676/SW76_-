#!/usr/bin/env python3
"""Fail-closed acceptance of the frozen, paired 60-condition final runs.

This does not run matches, select candidates, or compute a performance verdict.
It independently checks the prespecified design and its retained artifacts, and
freshly compiles the frozen relay scenario test with ASan and UBSan.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import traceback
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
HISTORY = ROOT / "experiment_results/protect_local_20260927/full_v1"
SEEDS = (20261011, 20261012, 20261013)
MAPS = ("autarky", "default", "devil", "portals", "dilemma", "queen_of_spades",
        "schooltime", "slithery_fight", "trauma", "trophy")
EXPECTED = {("V3_portal", seed, name, side) for seed in SEEDS for name in MAPS for side in "AB"}
SKIP = {".unswbc-build", ".git", "__pycache__"}
SOURCE_SUFFIXES = {".cpp", ".hpp", ".cc", ".hh", ".c", ".h", ".cxx", ".hxx"}
ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
RESULT = re.compile(r"^(?:team ([AB]) wins|draw) after (\d+) rounds\b[^\n]*", re.MULTILINE)
BOT_NOTICE = re.compile(r"^round (\d+|\?): bot (\d+) \(team ([AB?])\) (.*)$", re.MULTILINE)
VIEWER_NOTICE = re.compile(r"replay viewer: \} -- run \x60unswbc vscode\x60 to retry")
NORMAL_DEATHS = {"hit a wall", "hit itself", "hit another dragon", "lost a head-to-head"}
INVALID_ACTIONS = {"no valid action", "invalid action"}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def artifact(path):
    path = Path(path)
    exists = path.is_file()
    return {"path": str(path), "exists": exists,
            "bytes": path.stat().st_size if exists else None,
            "sha256": sha(path) if exists else None}


def condition(row):
    return row["opponent"], int(row["seed"]), row["map"], row["bot1_side"]


def log_seed_values(clean):
    return [int(seed, 16) for seed in re.findall(r"^seed 0x([0-9a-fA-F]+)$", clean, re.MULTILINE)]


def replay_map_matches(replay, map_name, configured_maps):
    path = str(ROOT / "battlecode_quickstart/maps" / (map_name + ".map"))
    expected = configured_maps.get(path)
    return isinstance(expected, str) and len(expected) == 64 and replay.get("embedded_map_sha256") == expected


def log_notices(raw):
    clean = ANSI.sub("", raw)
    viewer = VIEWER_NOTICE.findall(clean)
    clean = VIEWER_NOTICE.sub("", clean)
    runtime, invalid_lines, invalid_events = [], [], []
    for notice in BOT_NOTICE.finditer(clean):
        message = notice.group(4)
        if message.startswith("died: "):
            reason = message[6:]
            if reason in INVALID_ACTIONS:
                invalid_lines.append(notice.group(0))
                invalid_events.append({"round": int(notice.group(1)) if notice.group(1) != "?" else None,
                                       "dragon": int(notice.group(2)), "team": notice.group(3)})
            elif reason not in NORMAL_DEATHS:
                runtime.append(notice.group(0))
        elif not message.startswith(("points ", "stdout:", "stderr:")):
            runtime.append(notice.group(0))
    return clean, {"runtime_notices": runtime, "invalid_action_log_lines": invalid_lines,
                   "invalid_action_log_events": invalid_events, "replay_viewer_notices": viewer}


def validate_conditions(rows):
    counts, malformed, failed = Counter(), [], []
    for index, row in enumerate(rows, 1):
        try:
            counts[condition(row)] += 1
        except (KeyError, TypeError, ValueError) as exc:
            malformed.append({"row": index, "error": str(exc)})
        if row.get("outcome") not in {"win", "loss", "draw"} or row.get("error"):
            failed.append({"row": index, "outcome": row.get("outcome"), "error": row.get("error")})
    missing = sorted(EXPECTED - counts.keys())
    unexpected = sorted(counts.keys() - EXPECTED)
    duplicates = [{"condition": key, "count": count} for key, count in sorted(counts.items()) if count != 1]
    passed = len(rows) == 60 and not (missing or unexpected or duplicates or malformed or failed)
    return {"passed": passed, "expected": 60, "rows": len(rows), "unique_conditions": len(counts),
            "completed": sum(r.get("outcome") in {"win", "loss", "draw"} and not r.get("error") for r in rows),
            "missing": missing, "unexpected": unexpected, "duplicates": duplicates,
            "malformed": malformed, "failed": failed,
            "per_seed": dict(sorted(Counter(str(k[1]) for k in counts.elements()).items())),
            "per_map": dict(sorted(Counter(k[2] for k in counts.elements()).items()))}


def selftests():
    complete = [dict(zip(("opponent", "seed", "map", "bot1_side"), key), outcome="win", error="")
                for key in sorted(EXPECTED)]
    failed = [dict(row) for row in complete]
    failed[0]["outcome"] = "failed"
    wrong = [dict(row) for row in complete]
    wrong[0]["seed"] = 20261001
    partial = [r for r in complete if r["map"] == "schooltime" and r["bot1_side"] == "A"]
    cases = {"complete_60_accepted": validate_conditions(complete)["passed"],
             "missing_condition_rejected": not validate_conditions(complete[:-1])["passed"],
             "duplicate_condition_rejected": not validate_conditions(complete + [complete[0]])["passed"],
             "duplicate_replacing_missing_rejected": not validate_conditions(complete[:-1] + [complete[0]])["passed"],
             "failed_condition_rejected": not validate_conditions(failed)["passed"],
             "unexpected_seed_rejected": not validate_conditions(wrong)["passed"],
             "balanced_three_row_subset_rejected": not validate_conditions(partial)["passed"],
             "empty_run_rejected": not validate_conditions([])["passed"]}
    viewer = "replay viewer: } -- run \x60unswbc vscode\x60 to retry"
    normal = log_notices("round 2: bot 1 (team B) died: hit itself" + viewer)[1]
    invalid = log_notices("round 2: bot 1 (team B) died: no valid action" + viewer)[1]
    crash = log_notices("round 2: bot 1 (team B) exited with code 1" + viewer)[1]
    cases.update(viewer_stderr_interleave_not_runtime_failure=not normal["runtime_notices"] and len(normal["replay_viewer_notices"]) == 1,
                 viewer_interleave_does_not_hide_invalid_action=len(invalid["invalid_action_log_events"]) == 1,
                 viewer_interleave_does_not_hide_crash=len(crash["runtime_notices"]) == 1)
    maps = {str(ROOT / "battlecode_quickstart/maps/autarky.map"): "a" * 64}
    cases.update(correct_embedded_map_accepted=replay_map_matches({"embedded_map_sha256": "a" * 64}, "autarky", maps),
                 wrong_embedded_map_rejected=not replay_map_matches({"embedded_map_sha256": "b" * 64}, "autarky", maps),
                 wrong_map_name_rejected=not replay_map_matches({"embedded_map_sha256": "a" * 64}, "portals", maps),
                 correct_logged_seed_accepted=log_seed_values(f"seed 0x{SEEDS[0]:016x}\n") == [SEEDS[0]],
                 wrong_logged_seed_rejected=log_seed_values(f"seed 0x{SEEDS[0]:016x}\n") != [SEEDS[1]],
                 missing_logged_seed_rejected=log_seed_values("no seed\n") != [SEEDS[0]])
    return {"passed": all(cases.values()), "cases": cases}


class Audit:
    def __init__(self):
        self.checks, self.issues = {}, []

    def check(self, name, passed, detail=None):
        self.checks[name] = bool(passed)
        if not passed:
            self.issues.append({"check": name, "detail": detail})
        return bool(passed)

    def required_artifact(self, path, label):
        evidence = artifact(path)
        self.check(label, evidence["exists"] and evidence["bytes"] > 0, evidence)
        return evidence


def production(directory):
    return {str(p.relative_to(directory)): sha(p) for p in sorted(directory.rglob("*"))
            if p.is_file() and not set(p.relative_to(directory).parts) & SKIP
            and (p.name == "bot.toml" or p.suffix in SOURCE_SUFFIXES)}


def source_evidence(directory, label, audit):
    manifest = production(directory)
    audit.check(label + ".required_sources_exist", directory.is_dir() and
                all(name in manifest and (directory / name).stat().st_size > 0
                    for name in ("main.cpp", "helper.hpp", "bot.toml")), str(directory))
    return {"path": str(directory), "production_sha256": manifest}


def compare_sources(a, b, label, audit):
    ma, mb = a["production_sha256"], b["production_sha256"]
    differences = [name for name in sorted(ma.keys() | mb.keys()) if ma.get(name) != mb.get(name)]
    audit.check(label, bool(ma) and bool(mb) and not differences, differences)


def compare_binaries(a, b, label, audit):
    ea, eb = artifact(a), artifact(b)
    passed = all(e["exists"] and e["bytes"] > 0 for e in (ea, eb)) and ea["sha256"] == eb["sha256"]
    audit.check(label, passed, {"a": ea, "b": eb})
    return {"passed": passed, "a": ea, "b": eb}


def load_replay_reader():
    path = ROOT / "experiment_results/protect_local_20260927/analyze_replays.py"
    spec = importlib.util.spec_from_file_location("frozen_replay_reader", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module, artifact(path)


def replay_evidence(path, reader):
    root = reader.Message(path.read_bytes()).structure(0, 0)
    version = root.number(0, "I")
    if version > 2:
        raise ValueError(f"unsupported replay version {version}")
    if root.pointer(3) is None or root.pointer(4) is None:
        raise ValueError("replay lacks events or final result")
    _, _, _, teams = reader.parse_map(root.text(0))
    deaths, rounds, invalid = Counter(), [], []
    deaths_by_team = {side: Counter() for side in "AB"}
    for event in root.structs(3):
        kind = event.number(0, "H")
        if kind > 12:
            raise ValueError(f"unknown event kind {kind}")
        if kind == 0:
            rounds.append(event.child(0).number(0))
        elif kind == 10:
            data = event.child(0)
            teams[data.number(4)] = "AB"[data.number(8, "H")]
        elif kind == 11:
            data = event.child(0)
            code = data.number(4, "H")
            if code >= len(reader.DEATH_REASONS):
                raise ValueError(f"unknown death reason {code}")
            deaths[reader.DEATH_REASONS[code]] += 1
            team = teams[data.number(0)]
            deaths_by_team[team][reader.DEATH_REASONS[code]] += 1
            if code == 4:
                invalid.append({"round": rounds[-1] if rounds else None, "dragon": data.number(0), "team": team})
    if rounds != list(range(len(rounds))) or not rounds:
        raise ValueError("round events are missing or non-consecutive")
    result = root.child(4)
    kind = result.number(4, "H")
    if kind not in {0, 1}:
        raise ValueError(f"unknown result kind {kind}")
    side = result.number(6, "H")
    if kind == 1 and side not in {0, 1}:
        raise ValueError(f"unknown winner {side}")
    return {"version": version, "rounds": len(rounds), "winner": "AB"[side] if kind else "draw",
            "deaths": dict(deaths), "deaths_by_team": deaths_by_team, "invalid_action_deaths": invalid,
            "embedded_map_sha256": hashlib.sha256(root.text(0).encode()).hexdigest()}


def inspect_run(name, bot_name, reader, audit):
    directory = OUT / name
    evidence = {"directory": str(directory), "files": {}, "games": []}
    for filename in ("run.json", "results.csv", "summary.json"):
        evidence["files"][filename] = audit.required_artifact(directory / filename, name + ".exists." + filename)
    config = json.loads((directory / "run.json").read_text())
    with (directory / "results.csv").open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    evidence["conditions"] = validate_conditions(rows)
    audit.check(name + ".exact_60_completed_conditions", evidence["conditions"]["passed"], evidence["conditions"])
    audit.check(name + ".configured_seeds", config.get("seeds") == list(SEEDS), config.get("seeds"))
    sources = [ROOT / "tyhh_test" / bot_name, ROOT / "tyhh_test/V3_portal"]
    snapshots = [directory / "sources" / str(i) / source.name for i, source in enumerate(sources)]
    expected_built = [str(snapshot / ".unswbc-build/bot") for snapshot in snapshots]
    audit.check(name + ".source_paths", config.get("sources") == list(map(str, sources)), config.get("sources"))
    audit.check(name + ".binary_paths", config.get("built") == expected_built, config.get("built"))
    evidence["sources"] = [source_evidence(p, name + f".source{i}", audit) for i, p in enumerate(snapshots)]
    evidence["binaries"] = [audit.required_artifact(p, name + f".binary{i}") for i, p in enumerate(expected_built)]
    expected_config_hashes = {str(source / filename): sha(snapshot / filename)
                              for source, snapshot in zip(sources, snapshots)
                              for filename in ("main.cpp", "helper.hpp", "bot.toml")}
    audit.check(name + ".config_source_hashes", config.get("hashes") == expected_config_hashes, config.get("hashes"))
    paths = set()
    for row in rows:
        game = {"condition": condition(row), "game": row["game"], "outcome": row["outcome"], "errors": []}
        for kind in ("log", "replay"):
            filename = row[kind]
            safe = Path(filename).name == filename and filename.endswith("." + kind)
            audit.check(name + f".game{row['game']}.{kind}_safe_name", safe, filename)
            if not safe:
                raise ValueError("artifact reference is not a local filename")
            audit.check(name + f".game{row['game']}.{kind}_unique", filename not in paths, filename)
            paths.add(filename)
            game[kind] = audit.required_artifact(directory / filename, name + f".game{row['game']}.{kind}_exists")
        clean, notices = log_notices((directory / row["log"]).read_text(errors="replace"))
        game.update(notices)
        logged_seeds = log_seed_values(clean)
        audit.check(name + f".game{row['game']}.actual_logged_seed", logged_seeds == [int(row["seed"])], logged_seeds)
        audit.check(name + f".game{row['game']}.runtime_clean", not game["runtime_notices"], game["runtime_notices"])
        audit.check(name + f".game{row['game']}.invalid_action_log_clean", not game["invalid_action_log_lines"], game["invalid_action_log_lines"])
        matches = list(RESULT.finditer(clean))
        consistent = len(matches) == 1
        if consistent:
            match = matches[0]
            winner = match.group(1) or "draw"
            outcome = "draw" if winner == "draw" else "win" if winner == row["bot1_side"] else "loss"
            consistent = winner == row["winner"] and outcome == row["outcome"] and int(match.group(2)) == int(row["rounds"])
            game["log_result"] = {"winner": winner, "rounds": int(match.group(2)), "outcome": outcome}
        audit.check(name + f".game{row['game']}.csv_log_result_consistent", consistent, game.get("log_result"))
        try:
            replay = replay_evidence(directory / row["replay"], reader)
            game["replay_result"] = replay
            audit.check(name + f".game{row['game']}.embedded_map_matches_condition",
                        replay_map_matches(replay, row["map"], config.get("maps", {})),
                        {"map": row["map"], "embedded_sha256": replay["embedded_map_sha256"]})
            audit.check(name + f".game{row['game']}.replay_result_consistent", replay["winner"] == row["winner"] and replay["rounds"] == int(row["rounds"]), replay)
            audit.check(name + f".game{row['game']}.invalid_action_replay_clean", not replay["invalid_action_deaths"], replay["invalid_action_deaths"])
            audit.check(name + f".game{row['game']}.invalid_action_log_replay_consistent", game["invalid_action_log_events"] == replay["invalid_action_deaths"])
        except Exception as exc:
            game["errors"].append(str(exc))
            audit.check(name + f".game{row['game']}.replay_readable", False, str(exc))
        evidence["games"].append(game)
    disk_artifacts = {p.name for suffix in ("*.log", "*.replay") for p in directory.glob(suffix)}
    audit.check(name + ".no_unreferenced_match_artifacts", disk_artifacts == paths, sorted(disk_artifacts - paths))
    audit.check(name + ".unique_sequential_game_ids", sorted(int(r["game"]) for r in rows) == list(range(1, 61)))
    evidence["maps"] = config.get("maps")
    evidence["conditions_keys"] = [list(k) for k in sorted(condition(r) for r in rows)]
    return evidence


def command_evidence(command, timeout=60, env=None):
    result = {"command": command, "shell_command": shlex.join(command), "cwd": str(ROOT),
              "environment_overrides": env or {}}
    try:
        completed = subprocess.run(command, cwd=ROOT, env={**os.environ, **(env or {})},
                                   capture_output=True, text=True, timeout=timeout)
        result.update(exit_code=completed.returncode, stdout=completed.stdout, stderr=completed.stderr)
    except Exception as exc:
        result.update(exit_code=None, stdout="", stderr=str(exc))
    return result


def frozen_test(audit):
    source = OUT / "relay_final/sources/0/tyhh_qos_protect_relay"
    test = source / "verification/relay_tests.cpp.txt"
    folder = OUT / "verification_artifacts" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    folder.mkdir(parents=True, exist_ok=False)
    binary = folder / "relay_frozen_asan_ubsan"
    before = {**production(source), "verification/relay_tests.cpp.txt": sha(test)}
    compiler = shutil.which("clang++")
    if not compiler:
        raise FileNotFoundError("clang++ is required for a fresh sanitizer compilation")
    command = [compiler, "-std=c++20", "-O1", "-g", "-fsanitize=address,undefined",
               "-fno-omit-frame-pointer", "-Wall", "-Wextra", "-x", "c++", str(test), "-o", str(binary)]
    evidence = {"source": artifact(test), "source_manifest_before": before,
                "mode": "Fresh compilation of frozen final source; not an existing test-binary rerun",
                "compile": command_evidence(command, timeout=180)}
    audit.check("frozen_test.fresh_compile", evidence["compile"]["exit_code"] == 0, evidence["compile"])
    for channel in ("stdout", "stderr"):
        (folder / ("compile." + channel + ".txt")).write_text(evidence["compile"][channel])
    evidence["binary"] = audit.required_artifact(binary, "frozen_test.binary")
    if evidence["compile"]["exit_code"] == 0 and binary.is_file():
        evidence["run"] = command_evidence([str(binary)], env={"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                                                               "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
        for channel in ("stdout", "stderr"):
            (folder / ("run." + channel + ".txt")).write_text(evidence["run"][channel])
        run = evidence["run"]
        audit.check("frozen_test.sanitizers_and_assertions", run["exit_code"] == 0 and
                    "relay room/topology assertions passed" in run["stdout"] and not run["stderr"], run)
    else:
        audit.check("frozen_test.sanitizers_and_assertions", False, "Compile did not produce a runnable binary")
    after = {**production(source), "verification/relay_tests.cpp.txt": sha(test)}
    evidence["source_manifest_after"] = after
    audit.check("frozen_test.source_unchanged_during_test", before == after)
    (folder / "transcript.json").write_text(json.dumps(evidence, indent=2) + "\n")
    evidence["transcript"] = artifact(folder / "transcript.json")
    return evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test-only", action="store_true")
    parser.add_argument("--output", type=Path, default=OUT / "final_verification.json")
    args = parser.parse_args()
    tests = selftests()
    if args.self_test_only:
        print(json.dumps(tests, indent=2))
        return 0 if tests["passed"] else 1
    audit = Audit()
    result = {"passed": False, "verified_at_utc": datetime.now(timezone.utc).isoformat(),
              "scope": "Artifact integrity and exact planned conditions, not a statistical significance or strategy-strength verdict.",
              "expected": {"opponent": "V3_portal", "seeds": list(SEEDS), "maps": list(MAPS), "sides": ["A", "B"], "per_run": 60},
              "selftests": tests, "verifier": artifact(Path(__file__)), "runs": {}}
    audit.check("selftests", tests["passed"], tests)
    try:
        reader, result["replay_reader"] = load_replay_reader()
        for name, bot in (("relay_final", "tyhh_qos_protect_relay"), ("baseline_final", "tyhh_qos_protect_route")):
            try:
                result["runs"][name] = inspect_run(name, bot, reader, audit)
            except Exception as exc:
                audit.check(name + ".inspection_completed", False, str(exc))
                result["runs"][name] = {"exception": str(exc), "traceback": traceback.format_exc()}
        relay, baseline = result["runs"]["relay_final"], result["runs"]["baseline_final"]
        audit.check("paired_exact_condition_sets", relay.get("conditions_keys") == baseline.get("conditions_keys") and
                    len(relay.get("conditions_keys", [])) == 60 and len(baseline.get("conditions_keys", [])) == 60)
        comparison_paths = {
            "current_relay": ROOT / "tyhh_test/tyhh_qos_protect_relay",
            "development_relay": OUT / "relay_dev/sources/0/tyhh_qos_protect_relay",
            "current_route": ROOT / "tyhh_test/tyhh_qos_protect_route",
            "historical_route": HISTORY / "sources/0/tyhh_qos_protect_route",
            "current_V3": ROOT / "tyhh_test/V3_portal",
            "historical_V3": HISTORY / "sources/2/V3_portal"}
        result["reference_sources"] = refs = {name: source_evidence(path, name, audit) for name, path in comparison_paths.items()}
        for name in ("current_relay", "development_relay"):
            compare_sources(relay["sources"][0], refs[name], "relay_final.matches." + name, audit)
        for name in ("current_route", "historical_route"):
            compare_sources(baseline["sources"][0], refs[name], "baseline_final.matches." + name, audit)
        for run in ("relay_final", "baseline_final"):
            for name in ("current_V3", "historical_V3"):
                compare_sources(result["runs"][run]["sources"][1], refs[name], run + ".opponent_matches." + name, audit)
        compare_sources(relay["sources"][1], baseline["sources"][1], "final_opponent_manifests_equal", audit)
        historical_maps = json.loads((HISTORY / "run.json").read_text())["maps"]
        expected_paths = {str(ROOT / "battlecode_quickstart/maps" / (name + ".map")) for name in MAPS}
        current_maps = {path: sha(Path(path)) for path in sorted(expected_paths)}
        result["map_hashes"] = {"historical": historical_maps, "current": current_maps,
                                "relay_final": relay["maps"], "baseline_final": baseline["maps"]}
        audit.check("exact_official_map_paths", all(set(maps) == expected_paths for maps in result["map_hashes"].values()))
        audit.check("all_map_hashes_equal", all(maps == historical_maps for maps in result["map_hashes"].values()))
        result["binary_comparisons"] = {}
        pairs = {
            "opponents_across_finals": (Path(relay["binaries"][1]["path"]), Path(baseline["binaries"][1]["path"])),
            "opponent_vs_historical": (Path(relay["binaries"][1]["path"]), comparison_paths["historical_V3"] / ".unswbc-build/bot"),
            "relay_vs_development": (Path(relay["binaries"][0]["path"]), comparison_paths["development_relay"] / ".unswbc-build/bot"),
            "route_vs_historical": (Path(baseline["binaries"][0]["path"]), comparison_paths["historical_route"] / ".unswbc-build/bot")}
        for name, (a, b) in pairs.items():
            result["binary_comparisons"][name] = compare_binaries(a, b, "binary." + name, audit)
        result["tool_versions"] = {name: command_evidence([executable, "--version"]) for name, executable in
                                   (("clang++", shutil.which("clang++") or "clang++"), ("unswbc", shutil.which("unswbc") or "unswbc"))}
        for name, evidence in result["tool_versions"].items():
            audit.check("tool_version." + name, evidence["exit_code"] == 0, evidence)
        result["tool_version_scope"] = "Versions collected at audit time; the match runner did not retain compilation commands or tool versions. Equal competition binaries are verified directly."
        result["frozen_sanitizer_test"] = frozen_test(audit)
    except Exception as exc:
        audit.check("all_required_audit_stages_completed", False, str(exc))
        result["exception"] = traceback.format_exc()
    result["checks"], result["issues"] = audit.checks, audit.issues
    invalid_checks = (".invalid_action_log_clean", ".invalid_action_replay_clean")
    result["artifact_integrity_passed"] = bool(audit.checks) and all(value for name, value in audit.checks.items()
                                                                           if not name.endswith(invalid_checks))
    result["runtime_failure_free"] = all(value for name, value in audit.checks.items() if name.endswith(".runtime_clean")) and \
                                    sum(name.endswith(".runtime_clean") for name in audit.checks) == 120
    result["invalid_action_summary"] = {}
    for name, run in result["runs"].items():
        games = run.get("games", [])
        events = [{"condition": game["condition"], **event,
                   "role": "candidate_or_baseline" if event["team"] == game["condition"][3] else "V3_portal"}
                  for game in games for event in game.get("invalid_action_log_events", [])]
        affected_games = [{"seed": game["condition"][1], "map": game["condition"][2],
                           "candidate_or_baseline_side": game["condition"][3], "outcome": game.get("outcome"),
                           "candidate_or_baseline_code4": sum(event["team"] == game["condition"][3]
                                                               for event in game["invalid_action_log_events"]),
                           "V3_portal_code4": sum(event["team"] != game["condition"][3]
                                                  for event in game["invalid_action_log_events"])}
                          for game in games if game.get("invalid_action_log_events")]
        result["invalid_action_summary"][name] = {"deaths": len(events),
            "games": sum(bool(game.get("invalid_action_log_events")) for game in games),
            "by_role": dict(Counter(event["role"] for event in events)), "events": events,
            "affected_games": affected_games}
    result["pass_policy"] = "passed requires exact complete artifact integrity, no runtime failure, no invalid-action death on either team, and fresh sanitizer test success. Invalid-action deaths are normal engine death events, distinct from runner failures; their presence does not remove completed games from the result table. artifact_integrity_passed reports those separate checks without silently clearing invalid actions."
    result["passed"] = bool(audit.checks) and all(audit.checks.values())
    result["check_count"] = len(audit.checks)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps({"output": str(args.output), "passed": result["passed"], "check_count": result["check_count"],
                      "issues": result["issues"]}, ensure_ascii=False, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
