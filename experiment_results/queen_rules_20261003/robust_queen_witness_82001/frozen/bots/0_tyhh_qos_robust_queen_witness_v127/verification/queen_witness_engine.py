#!/usr/bin/env python3
"""Native R20/H14 decisions at real input, then official 1.2.7 action-tape arms."""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import sys

from unswbc.bot import Bot, Pool
from unswbc.engine import WASM_PATH

ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / "experiment_results/queen_rules_20261003"
sys.path.insert(0, str(EVIDENCE))
import queen_immediate_engine as fixture


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def native(binary, spawn, raw):
    pool = Pool([str(binary.resolve())], size=0)
    bot = Bot(pool, init=spawn.encode(), name="queen0_real_round199")
    try:
        reply = bot.ask(raw.encode())
        assert bot.error is None, bot.error
        output = reply.decode()
        stderr = bot.take_stderr().decode()
    finally:
        bot.stop()
        pool.close()
    actions = [line for line in output.splitlines() if line.startswith("MOVE ") or line.startswith("SPLIT ")]
    assert len(actions) == 1, output
    return {"binary_sha256": sha(binary), "output": output, "stderr": stderr,
            "action": actions[0], "fresh_process": True,
            "scope": "Actual input native full pipeline; no prior-round memory"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--output-prefix", type=Path, required=True)
    args = parser.parse_args()
    paths = [args.output_prefix.with_suffix(".json")] + [
        args.output_prefix.with_name(args.output_prefix.name + "_" + arm).with_suffix(".replay")
        for arm in ("H14", "R20")]
    if any(path.exists() for path in paths):
        parser.error("Output exists; preserve prior evidence")
    assert importlib.metadata.version("unswbc") == "1.2.7"
    assert sha(WASM_PATH) == fixture.ENGINE_SHA and sha(fixture.REPLAY) == fixture.REPLAY_SHA
    evidence_path = EVIDENCE / "queen_head_witness_recheck_20261006.json"
    evidence = json.loads(evidence_path.read_text())
    observed = evidence["arms"][0]["queen_round199_input"]
    raw, spawn = observed["raw"], evidence["arms"][0]["queen_spawn_input"]
    assert hashlib.sha256(raw.encode()).hexdigest() == "6f2229283c3a5b5c06623d6106cff57ff3524420ba0cdcfdc51134478391d416"
    native_control = native(args.control, spawn, raw)
    native_candidate = native(args.candidate, spawn, raw)
    assert native_control["action"] == "MOVE N", native_control
    assert native_candidate["action"] == "MOVE NN", native_candidate
    assert "confirmed_before 1 confirmed_after 0 complete_heads 1" in native_candidate["output"]
    root = fixture.Message(fixture.REPLAY.read_bytes()).structure(0, 0)
    fixture.REPLAY_MAP = root.text(0).encode()
    source = fixture.replay_data(root)
    arms = []
    for label, decision, replay_path in zip(("H14", "R20"), (native_control, native_candidate), paths[1:]):
        official, packed = fixture.run_arm(source, decision["action"].split()[1])
        assert official["queen_round199_input"]["raw"] == raw
        with replay_path.open("xb") as handle:
            handle.write(packed)
        arms.append({"label": label, "native_decision": decision, "official": official,
                     "fixture_replay": str(replay_path)})
        print(label, decision["action"], "official immediate deaths", official["immediate_deaths"], flush=True)
    payload = {"passed": True, "scope": "Source-bound native input and action-tape fixtures, not competition",
               "toolkit_version": "1.2.7", "engine_sha256": sha(WASM_PATH),
               "source_sha256": sha(__file__), "parent_main_sha256": sha(ROOT / "tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"),
               "candidate_main_sha256": sha(ROOT / "tyhh_test/tyhh_qos_robust_queen_witness_v127/main.cpp"),
               "candidate_module_sha256": sha(ROOT / "tyhh_test/tyhh_qos_robust_queen_witness_v127/queen_witness.hpp"),
               "evidence": str(evidence_path), "evidence_sha256": sha(evidence_path),
               "raw_input_sha256": observed["sha256"], "arms": arms,
               "limitations": ["Fresh native bots receive real r199 input with no prior-round memory.",
                               "Recorded actions/sonar recreate original map/seed up to queen action.",
                               "Only the extracted native MOVE is applied in the engine; recorded sonar is held constant.",
                               "Cleanup after id14 is fixture-only ENDTURN and not bot behavior.",
                               "No global survival, competitive win-rate, hidden threat, or later-turn guarantee is established."]}
    with paths[0].open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"passed": True, "output": str(paths[0]), "native_moves": [native_control["action"], native_candidate["action"]]}))


if __name__ == "__main__":
    main()
