#!/usr/bin/env python3
"""Bind fresh native candidate decisions to existing exact official rule arms."""
import argparse
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(RUNS))
from analyze import sha
from r26_headtrade_fixture import native


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, default=Path("/tmp/r26_headtrade_bot"))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.output is not None:
        assert not args.output.exists(), "Preserve earlier evidence"
    actual = HERE / "r26_headtrade_actual_state.json"
    source = ROOT / "tyhh_test/tyhh_qos_robust_headtrade_v127"
    saved = json.loads(actual.read_text())
    assert saved["passed"] and len(saved["cases"]) == 2
    cases = []
    for case in saved["cases"]:
        original, applied = case["arms"]
        assert original["target_input"] == applied["target_input"]
        assert original["prefix_sha256"] == applied["prefix_sha256"]
        assert sha(applied["fixture_replay"]) == applied["fixture_replay_sha256"]
        assert applied["target_and_attacker_deaths_same_round"] == []
        result = native(args.candidate, original["target_spawn"], original["target_input"]["raw"])
        assert result["action"] == applied["action"] == "MOVE N", result
        assert "SHORTER_HEAD_TRADE" in result["output"]
        cases.append({"label": case["label"], "target_round": case["round"],
            "target_id": case["id"], "source_replay_sha256": case["source_replay_sha256"],
            "target_raw_input_sha256": original["target_input"]["sha256"],
            "native_candidate": result, "official_applied_arm": applied["fixture_replay"],
            "official_applied_arm_sha256": applied["fixture_replay_sha256"],
            "native_action_exactly_matches_official_scripted_arm": True})
    payload = {"passed": True, "scope": "Fresh native actual-input decisions linked to existing official rule arms; not competitive outcome",
        "script_sha256": sha(__file__), "actual_fixture_sha256": sha(actual),
        "candidate_sources": {name: sha(source / name) for name in
            ("main.cpp", "headtrade.hpp", "helper.hpp", "consolidate.hpp", "queen_feed.hpp")},
        "candidate_binary_sha256": sha(args.candidate), "cases": cases,
        "limitations": ["Fresh native controls do not reconstruct prior-round private memory.",
            "Only native movement is applied; official arms retain recorded SONAR and other actor replies.",
            "Local survival against the exact witnessed enemy does not establish wins or global safety."]}
    if args.output is not None:
        with args.output.open("x") as handle:
            json.dump(payload, handle, indent=2)
            handle.write("\n")
    print(json.dumps({"passed": True, "source_main_sha256": payload["candidate_sources"]["main.cpp"],
                      "candidate_binary_sha256": payload["candidate_binary_sha256"],
                      "cases": [{"label": c["label"], "action": c["native_candidate"]["action"]} for c in cases]}))


if __name__ == "__main__":
    main()
