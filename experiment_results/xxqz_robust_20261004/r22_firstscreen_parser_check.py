#!/usr/bin/env python3
"""Source and synthetic classifier audit only; no games or result analysis."""
from __future__ import annotations
import argparse
import ast
from collections import Counter
import copy
import hashlib
import json
from pathlib import Path
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
SUBJECT = HERE / "r22_firstscreen_mechanism.py"
EXPECTED_SHA = "33fe2ac8c73e9c6b952b16f9a2784027a584121d980b3e7f9350c57119152999"
FACTOR = b"        if (!isQueen() && ROUND < 350 && LEN >= 5 &&\n            ct.can_split(LEN - 2) && splitSafe(LEN - 2)) {\n            doSplit(LEN - 2);\n            return;\n        }\n"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def source_binding():
    configs = [json.loads((RUNS / name / "run.json").read_text()) for name in
               ("robust_queen_hunt_82001_retry1", "robust_queen_rear_82001")]
    directories = [Path(config["sources"][0]["snapshot"]) for config in configs]
    old, new = [(directory / "main.cpp").read_bytes() for directory in directories]
    assert new.count(FACTOR) == 1 and new.replace(FACTOR, b"", 1) == old
    shared = ("helper.hpp", "queen_feed.hpp", "consolidate.hpp", "bot.toml",
              "verification/queen_hunt_tests.cpp.txt", "verification/sprint4_tests.cpp.txt")
    for name in shared:
        assert (directories[0] / name).read_bytes() == (directories[1] / name).read_bytes()
    for config, source in zip(configs, (old, new)):
        assert hashlib.sha256(source).hexdigest() == config["sources"][0]["files"]["main.cpp"]
        assert sha(config["sources"][0]["prepared"]) == config["sources"][0]["prepared_sha256"]
        assert sha(config["engine_original"]) == config["engine_sha256"]
    assert configs[0]["engine_sha256"] == configs[1]["engine_sha256"]
    assert {m["name"]: m["sha256"] for m in configs[0]["maps"]} == \
        {m["name"]: m["sha256"] for m in configs[1]["maps"]}
    assert [s["prepared_sha256"] for s in configs[0]["sources"][1:]] == \
        [s["prepared_sha256"] for s in configs[1]["sources"][1:]]
    return {"passed": True, "five_line_factor_only": True,
        "R10_main_sha256": hashlib.sha256(old).hexdigest(),
        "R22_main_sha256": hashlib.sha256(new).hexdigest(),
        "shared_byte_identical": list(shared), "same_engine_maps_opponents": True}


def synthetic_checks():
    tree = ast.parse(SUBJECT.read_text())
    hunting = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "hunting")
    finish = next(node for node in hunting.body if isinstance(node, ast.FunctionDef) and node.name == "finish")
    code = compile(ast.fix_missing_locations(ast.Module(body=[finish], type_ignores=[])), str(SUBJECT), "exec")
    directions = ((0, -1), (1, 0), (0, 1), (-1, 0))
    def landing(cell, direction, width, height):
        dx, dy = directions[direction]
        return (cell[0] + dx) % width, (cell[1] + dy) % height
    base = SimpleNamespace(LETTERS="NESW", landing=landing)
    def active(steps=3, hit=3, actor=2, victim=1, prefix_length=5):
        previous = (hit, 8)
        return {"side": "A", "id": actor, "kind": 0, "directions": "E" * steps,
            "length_before": 6, "updates": [{"head": (i + 2, 8), "ordinary_step": True}
                for i in range(hit - 1)], "deaths": [
                {"id": actor, "side": "A", "reason": "head", "body_before": [previous],
                 "actor_length_before_death": prefix_length},
                {"id": victim, "side": "B", "reason": "head",
                 "body_before": [(hit + 1, 8)]}]}
    def run(record, edges=None, rejected=False):
        namespace = {"active": copy.deepcopy(record), "own": "A", "width": 16, "height": 16,
            "counts": Counter(), "trades": [], "edges": edges or {}, "base": base}
        exec(code, namespace)
        try:
            namespace["finish"]()
        except AssertionError:
            assert rejected
            return namespace
        assert not rejected
        return namespace
    results = []
    def checked(name, condition):
        assert condition, name
        results.append({"case": name, "passed": True})
    result = run(active())
    checked("three_step_hit_is_two_successful_updates_plus_collision",
            result["counts"]["head_hits_after_two_steps"] == 1 and result["trades"][0]["actual_hit_step"] == 3)
    result = run(active(steps=4, hit=1))
    checked("four_requested_steps_early_hit_not_long_range", result["counts"]["head_hits_after_two_steps"] == 0)
    result = run(active(steps=4, hit=3))
    checked("unused_fourth_direction_not_counted_as_fourth_hit", result["trades"][0]["actual_hit_step"] == 3)
    result = run(active(actor=0))
    checked("own_queen_route_excluded_from_nonqueen_sprint", result["counts"]["head_hits_after_two_steps"] == 0)
    result = run(active(), {((3, 8), 1): (2, 0)})
    checked("portal_collision_excluded_from_ordinary_sprint", result["counts"]["head_hits_after_two_steps"] == 0)
    record = active(); record["updates"][0]["ordinary_step"] = False
    checked("nonordinary_prefix_excluded", run(record)["counts"]["head_hits_after_two_steps"] == 0)
    record = active(); record["deaths"] = []
    checked("generic_successful_harvest_not_head_trade", not run(record)["trades"])
    record = active(); record["deaths"] = record["deaths"][1:]
    checked("victim_head_death_without_actor_pair_not_trade", not run(record)["trades"])
    record = active(); record["side"] = "B"
    checked("enemy_acting_turn_not_our_initiated_trade", not run(record)["trades"])
    result = run(active(victim=5))
    checked("ordinary_enemy_not_fixed_queen", result["counts"]["our_initiated_enemy_queen_hits"] == 0)
    run(active(prefix_length=2), rejected=True)
    checked("paid_collision_prefix_length_two_rejected", True)
    checked("paid_collision_prefix_length_three_allowed", run(active(prefix_length=3))["trades"][0]["actual_hit_step_eligible"])
    record = active(); record.update(kind=None, directions="", updates=[], deaths=[])
    checked("null_action_pointer_counted_separately", run(record)["counts"]["no_action_pointer"] == 1)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=HERE / "r22_firstscreen_parser_check.json")
    args = parser.parse_args()
    assert not args.output.exists()
    assert sha(SUBJECT) == EXPECTED_SHA
    payload = {"passed": True, "scope": "Frozen source and synthetic nested classifier only; no engine invocation",
        "subject_sha256": EXPECTED_SHA, "script_sha256": sha(__file__),
        "source_binding": source_binding(), "synthetic_cases": synthetic_checks()}
    assert sha(SUBJECT) == EXPECTED_SHA
    with args.output.open("x", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"passed": True, "synthetic_cases": len(payload["synthetic_cases"]),
                      "output": str(args.output)}, indent=2))


if __name__ == "__main__":
    main()
