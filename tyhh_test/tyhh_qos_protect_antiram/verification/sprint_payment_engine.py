#!/usr/bin/env python3
"""Confirm length-2 pearl-then-sprint semantics using the installed WASM engine."""
import hashlib
import json
import sys
from pathlib import Path
from unswbc.engine import EngineModule, WASM_PATH

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "experiment_results/protect_local_20260927"))
from analyze_replays import Message, point


def check(feed):
    # Keep installed-map terrain unchanged, alter only resource configuration
    # and initial dragons in memory. The fixture is not a competitive match.
    source = ROOT / "battlecode_quickstart/maps/default_small.map"
    lines = []
    for line in source.read_text().splitlines():
        words = line.split()
        if words and words[0] == "TILE":
            line = f"TILE {words[1]} {words[2]} " + ("1 1" if feed else "0 0")
        elif words and words[0] == "DRAGON_COUNT":
            line = "DRAGON_COUNT 2"
        elif words and words[0] == "DRAGON":
            continue
        elif words and words[0] == "END":
            lines.extend(["DRAGON 0 2 3 3 2 3", "DRAGON 1 2 11 11 10 11"])
        lines.append(line)
    engine, calls, deaths = EngineModule(), {}, []
    first_input = []

    def reply(dragon_id, block):
        calls[dragon_id] = calls.get(dragon_id, 0) + 1
        if dragon_id == 0 and calls[dragon_id] == 1:
            first_input.extend(block.decode().splitlines())
            return b"MOVE EE\nENDTURN\n"
        # End the fixture on the next round. These intentional invalid actions
        # are not interpreted as failures of the two-step test.
        return b"MOVE E\nENDTURN\n" if calls[dragon_id] == 1 else b"ENDTURN\n"

    engine.run(("\n".join(lines) + "\n").encode(), reply,
               lambda dragon_id, round_index, reason: deaths.append([dragon_id, round_index, reason]),
               on_notice=lambda _: None, seed=7)
    root = Message(engine.replay("fixture A", "fixture B")).structure(0, 0)
    round_index, positions = -1, []
    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 9 and round_index == 0 and data.number(0) == 0:
            positions.append({"head": point(data.child(0)), "tail": point(data.child(1))})
    assert "LENGTH 2" in first_input
    if feed:
        assert len(positions) == 2 and not any(dragon_id == 0 and r == 0 for dragon_id, r, _ in deaths)
    else:
        assert len(positions) == 1 and [0, 0, "A"] in deaths
    return {"first_step_has_pearl": feed, "start_length": 2, "request": "MOVE EE",
            "round_zero_movement_events": positions, "deaths": deaths,
            "fixture_map_source_sha256": hashlib.sha256(source.read_bytes()).hexdigest()}


if __name__ == "__main__":
    print(json.dumps({"engine_sha256": hashlib.sha256(WASM_PATH.read_bytes()).hexdigest(),
                      "cases": [check(False), check(True)], "passed": True}, indent=2))
