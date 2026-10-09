#!/usr/bin/env python3
"""Run fresh H14/R25 processes on the saved official target input."""
import hashlib
import importlib.metadata
import json
from pathlib import Path

from unswbc.bot import Bot, Pool

ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / "experiment_results/xxqz_robust_20261004"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def ask(binary, spawn, raw):
    pool = Pool([str(binary.resolve())], size=0)
    bot = Bot(pool, init=spawn.encode(), name=binary.stem)
    try:
        reply = bot.ask(raw.encode())
        error = bot.error
        output = reply.decode()
        stderr = bot.take_stderr().decode()
    finally:
        bot.stop()
        pool.close()
    actions = [line for line in output.splitlines() if line.startswith(("MOVE ", "SPLIT "))]
    assert error is None, error
    assert len(actions) == 1, output
    return {"binary_sha256": sha(binary), "action": actions[0],
            "output": output, "stderr": stderr, "fresh_process": True}


def main():
    source = json.loads((EVIDENCE / "r25_terminal_actual_state.json").read_text())
    assert source["passed"] and importlib.metadata.version("unswbc") == "1.2.7"
    spawn = source["target_spawn_input"]
    raw = source["target_turn_input"]["raw"]
    control = Path("/tmp/r25_h14_control_bot")
    candidate = Path("/tmp/r25_short_terminal_bot")
    arms = [ask(control, spawn, raw), ask(candidate, spawn, raw)]
    assert arms[0]["action"] == "MOVE S", arms[0]
    assert arms[1]["action"] == "MOVE E", arms[1]
    payload = {
        "passed": True,
        "scope": "Fresh-process native source-bound action fixture; no competition",
        "source_state": str(EVIDENCE / "r25_terminal_actual_state.json"),
        "source_state_sha256": sha(EVIDENCE / "r25_terminal_actual_state.json"),
        "raw_input_sha256": source["target_turn_input"]["sha256"],
        "spawn_sha256": hashlib.sha256(spawn.encode()).hexdigest(),
        "parent_main_sha256": sha(ROOT / "tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"),
        "candidate_main_sha256": sha(ROOT / "tyhh_test/tyhh_qos_robust_short_terminal_v127/main.cpp"),
        "candidate_module_sha256": sha(ROOT / "tyhh_test/tyhh_qos_robust_short_terminal_v127/short_terminal.hpp"),
        "target": {"round": source["target_round"], "id": source["target_id"],
                   "original_action": source["source_action"], "candidate_action": "MOVE E"},
        "arms": [{"label": "H14", **arms[0]}, {"label": "R25", **arms[1]}],
        "limitations": [
            "Fresh processes receive the exact target input but no prior-round persistent memory.",
            "This proves action selection at the registered input, not a counterfactual next-round escape.",
            "No competitive win-rate, hidden-body, future-enemy or global safety claim is made.",
        ],
    }
    out = Path(__file__).with_name("r25_terminal_native.json")
    assert not out.exists()
    out.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps({"passed": True, "output": str(out), "actions": [a["action"] for a in arms]}))


if __name__ == "__main__":
    main()
