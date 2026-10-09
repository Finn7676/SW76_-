#!/usr/bin/env python3
"""Freeze source provenance; inspect only explicitly listed development runs.

This script does not read any final-validation runs or result tables. Existing
focused test binaries are rerun solely to retain their output and hashes.
"""
import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
HISTORY = ROOT / "experiment_results/protect_local_20260927/full_v1"
VARIANTS = ["economy", "relay", "scout", "path", "economy_champion", "v2", "antiram", "v3"]
SKIP = {".unswbc-build", ".git", "__pycache__"}
TESTS = {
    "relay": [("verification/relay_tests.cpp.txt", "/tmp/tyhh_qos_protect_relay_tests")],
    "scout": [("verification/scout_tests.cpp.txt", "/tmp/tyhh-protect-scout-tests")],
    "path": [("verification/path_tests.cpp.txt", "/tmp/tyhh-protect-path-tests")],
    "v2": [("verification/scout_tests.cpp.txt", "/tmp/tyhh-protect-v2-scout-tests"),
           ("verification/relay_tests.cpp.txt", "/tmp/tyhh-protect-v2-relay-tests")],
    "antiram": [("verification/antiram_tests.cpp.txt", "/tmp/tyhh_qos_protect_antiram_tests")],
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def artifact(path):
    return {"path": str(path), "exists": path.is_file(),
            "sha256": sha(path) if path.is_file() else None,
            "bytes": path.stat().st_size if path.is_file() else None}


def manifest(directory):
    return {str(p.relative_to(directory)): sha(p) for p in sorted(directory.rglob("*"))
            if p.is_file() and not set(p.relative_to(directory).parts) & SKIP}


def production(man):
    return {name: value for name, value in man.items()
            if name == "bot.toml" or Path(name).suffix in {".cpp", ".hpp", ".h", ".c"}}


def compare(current, frozen):
    a, b = manifest(current), manifest(frozen)
    pa, pb = production(a), production(b)
    return {"current": str(current), "frozen": str(frozen),
            "production_sources_equal": pa == pb, "all_nonbuild_files_equal": a == b,
            "different_files": [name for name in sorted(a.keys() | b.keys()) if a.get(name) != b.get(name)],
            "current_sha256": a, "frozen_sha256": b}


def main():
    git = subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True, check=True)
    status = subprocess.run(["git", "status", "--porcelain"], cwd=ROOT, capture_output=True, text=True, check=True)
    route = ROOT / "tyhh_test/tyhh_qos_protect_route"
    v3 = ROOT / "tyhh_test/V3_portal"
    result = {
        "verified_at_utc": datetime.now(timezone.utc).isoformat(),
        "root_git_commit": git.stdout.strip(), "worktree_dirty": bool(status.stdout),
        "commit_scope": "HEAD at verification time; uncommitted bot state is identified by source SHA256, not by HEAD alone.",
        "result_access_scope": "No result table, final-validation directory, or held-out replay read by this script.",
        "baseline_route": compare(route, HISTORY / "sources/0/tyhh_qos_protect_route"),
        "opponent_V3_portal": compare(v3, HISTORY / "sources/2/V3_portal"),
        "candidate_verifications": [],
        "statistics_note": "Compare improvements only on matched opponent/map/seed/side conditions; different seed cohorts are not direct gains.",
    }
    for name in VARIANTS:
        source = ROOT / f"tyhh_test/tyhh_qos_protect_{name}"
        run_dir = OUT / f"{name}_dev"
        config_path = run_dir / "run.json"
        config = json.loads(config_path.read_text())
        snapshot = run_dir / "sources/0" / source.name
        item = {"name": name, "source": str(source), "development_run_config": artifact(config_path),
                "development_seeds": config["seeds"], "source_snapshot": compare(source, snapshot),
                "competition_build_binary": artifact(Path(config["built"][0])),
                "competition_build_marker": artifact(Path(config["built"][0]).parent / ".built-from"),
                "compilation_evidence": "Existing competition binary and run.json refer to the frozen source copy; compile stdout was not retained by the runner.",
                "opponent_source_snapshot": compare(v3, run_dir / "sources/1/V3_portal"),
                "helper_differs_only_in_trailing_whitespace_from_baseline":
                    (source / "helper.hpp").read_bytes().rstrip() == (route / "helper.hpp").read_bytes().rstrip(),
                "focused_tests": []}
        for test_source, binary_name in TESTS.get(name, []):
            binary, test = Path(binary_name), source / test_source
            evidence = {"source": artifact(test), "binary": artifact(binary),
                        "execution_mode": "rerun existing test binary; prior compile transcript is not persisted",
                        "compiler_flags_reported_by_implementer":
                            "clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -x c++",
                        "command": [str(binary)], "environment_overrides": {"ASAN_OPTIONS": "detect_leaks=0"}}
            if test.is_file() and binary.is_file():
                completed = subprocess.run([str(binary)], cwd=ROOT, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"},
                                           capture_output=True, text=True, timeout=60)
                evidence.update(exit_code=completed.returncode, stdout=completed.stdout, stderr=completed.stderr,
                                execution_passed=completed.returncode == 0)
            else:
                evidence.update(execution_passed=None, note="Missing source or binary; no test result claimed.")
            item["focused_tests"].append(evidence)
        result["candidate_verifications"].append(item)
    historical_config = json.loads((HISTORY / "run.json").read_text())
    result["official_maps"] = [{"path": p, "historical_sha256": h, "current_sha256": sha(Path(p)),
                                "matches_historical": sha(Path(p)) == h}
                               for p, h in sorted(historical_config["maps"].items())]
    result["evidence_files"] = [artifact(OUT / name) for name in
        ["PLAN.md", "hypothesis_tree.md", "diagnose_economy.py", "economy_diagnostics_raw.json",
         "DIAGNOSIS_BASELINE.md", "DIAGNOSIS_ECONOMY.md", "summarize_runs.py"]]
    result["checks"] = {
        "baseline_route_production_unchanged": result["baseline_route"]["production_sources_equal"],
        "V3_all_nonbuild_files_unchanged": result["opponent_V3_portal"]["all_nonbuild_files_equal"],
        "all_candidates_match_development_production_sources": all(x["source_snapshot"]["production_sources_equal"] for x in result["candidate_verifications"]),
        "all_development_V3_snapshots_match": all(x["opponent_source_snapshot"]["production_sources_equal"] for x in result["candidate_verifications"]),
        "all_official_maps_unchanged": all(x["matches_historical"] for x in result["official_maps"]),
        "all_executed_tests_passed": all(t["execution_passed"] is not False for x in result["candidate_verifications"] for t in x["focused_tests"]),
    }
    output = OUT / "verification.json"
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "checks": result["checks"]}, ensure_ascii=False))


if __name__ == "__main__":
    main()
