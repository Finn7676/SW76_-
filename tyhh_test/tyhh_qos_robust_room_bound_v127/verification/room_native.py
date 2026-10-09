#!/usr/bin/env python3
"""One-turn official-engine room fixtures with fresh production processes."""
from __future__ import annotations

import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import subprocess
import sys

from unswbc.bot import Bot, Pool
from unswbc.engine import EngineModule, WASM_PATH

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "experiment_results/protect_local_20260927"))
from analyze_replays import Message, point


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fixture_map(pearl_layout):
    original = ROOT / "battlecode_quickstart/maps/default.map"
    pearls = {(4, 5), (6, 4), (6, 6)} if pearl_layout == "r55" else {(5, 3)}
    pearls |= {(31 - x, 31 - y) for x, y in pearls}
    lines, dragons = [], []
    for row in original.read_text().splitlines():
        words = row.split()
        if not words:
            continue
        if words[0] in ("DRAGON_COUNT", "END"):
            continue
        if words[0] == "DRAGON":
            dragons.append(row)
            continue
        if words[0] == "TILE":
            x, y = map(int, words[1:3])
            row = f"TILE {x} {y} {int((x, y) in pearls)} {int((x, y) in pearls)}"
        elif words[0] == "MAP_NAME":
            row = "MAP_NAME Room Native Observable Fixture"
        lines.append(row)
    assert len(dragons) == 8
    dragons[5] = "DRAGON 1 2 4 4 3 4"
    if pearl_layout == "r39":
        dragons[7] = "DRAGON 1 3 5 5 4 5 4 6"
    return ("\n".join(lines + ["DRAGON_COUNT 8", *dragons, "END"]) + "\n").encode()


def run(binary, proof_binary, pearl_layout, candidate, evidence_dir):
    engine = EngineModule()
    pool = Pool([str(binary.resolve())], size=0)
    live, initializations, replies, deaths, notices = {}, {}, [], [], []

    def spawn(dragon, init):
        if dragon == 5:
            initializations[dragon] = init
            live[dragon] = Bot(pool, init=init, name=str(dragon))

    def reply(dragon, block):
        round_index = int(block.decode().splitlines()[0].split()[1])
        if round_index >= 1:
            out = b"ENDTURN\n"
        elif dragon == 5:
            out = live[dragon].ask(block)
            assert live[dragon].error is None, live[dragon].error
        else:
            direction = "W" if dragon % 2 == 0 else "E"
            out = f"MOVE {direction}\nENDTURN\n".encode()
        replies.append({"id": dragon, "round": round_index,
                        "input": block.decode(), "output": out.decode()})
        return out

    def death(dragon, round_index, reason):
        deaths.append({"id": dragon, "round": round_index, "reason": reason})
        if dragon in live:
            live.pop(dragon).stop()

    map_bytes = fixture_map(pearl_layout)
    try:
        engine.run(map_bytes, reply, death, spawn, notices.append, seed=7)
    finally:
        for bot in live.values():
            bot.stop()
        pool.close()

    replay = engine.replay("native room fixture", "scripted room fixture")
    root = Message(replay).structure(0, 0)
    round_index, moves, tile_changes = -1, [], []
    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 9:
            moves.append({"round": round_index, "event": event_index, "id": data.number(0),
                          "head": point(data.child(0)), "tail": point(data.child(1))})
        elif kind == 3:
            tile_changes.append({"round": round_index, "event": event_index,
                                 "cell": point(data.child(0)), "has_pearl": bool(data.number(0, "B") & 1)})
    first = next(r for r in replies if r["id"] == 5 and r["round"] == 0)
    input_bytes = initializations[5] + first["input"].encode()
    proof = subprocess.run([str(proof_binary.resolve())], input=input_bytes, capture_output=True, check=True)
    proof_state = json.loads(proof.stdout)
    assert proof_state["id"] == 5 and proof_state["round"] == 0 and proof_state["length"] == 2
    assert proof_state["head"] == [4, 4] and proof_state["visible_tiles"] == 49
    assert proof_state["num_sonar_messages"] == 0
    assert proof_state["room_pid"] == (0 if candidate else -1), proof_state
    expected = "S" if candidate and pearl_layout == "r55" else "N"
    assert f"MOVE {expected}\n" in first["output"], first
    actual_move = next(m for m in moves if m["id"] == 5 and m["round"] == 0)
    assert actual_move["head"] == ((4, 5) if expected == "S" else (24, 7)), actual_move
    assert not any(d["id"] == 5 and d["round"] < 1 for d in deaths), deaths
    second = next(r for r in replies if r["id"] == 5 and r["round"] == 1)
    length_line = next(line for line in second["input"].splitlines() if line.startswith("LENGTH "))
    assert length_line == ("LENGTH 3" if expected == "S" else "LENGTH 2"), length_line
    if expected == "S":
        assert any(t["cell"] == (4, 5) and not t["has_pearl"] and t["round"] == 0
                   and t["event"] >= actual_move["event"] - 1 for t in tile_changes), tile_changes
    stem = ("r24" if candidate else "h14") + "_" + pearl_layout
    replay_path = evidence_dir / (stem + ".replay")
    map_path = evidence_dir / (stem + ".map")
    input_path = evidence_dir / (stem + ".input.txt")
    output_path = evidence_dir / (stem + ".output.txt")
    for path, contents in ((replay_path, replay), (map_path, map_bytes),
                           (input_path, input_bytes), (output_path, first["output"].encode())):
        with path.open("xb") as handle:
            handle.write(contents)
    return {"candidate": candidate, "pearl_layout": pearl_layout,
        "fresh_process_round": 0, "not_exact_historical_r55_r39": True,
        "binary_sha256": sha(binary), "probe_binary_sha256": sha(proof_binary),
        "fixture_map_sha256": hashlib.sha256(map_bytes).hexdigest(), "fixture_map_text": map_bytes.decode(),
        "replay_sha256": hashlib.sha256(replay).hexdigest(),
        "replay_path": str(replay_path), "map_path": str(map_path),
        "input_path": str(input_path), "output_path": str(output_path),
        "initialization": initializations[5].decode(), "genuine_input_sha256": hashlib.sha256(input_bytes).hexdigest(),
        "native_input_proof": proof_state, "actual_subject_move": actual_move,
        "next_turn_length": int(length_line.split()[1]),
        "replies": replies, "moves": moves, "tile_changes": tile_changes,
        "deaths": deaths, "notices": notices}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--candidate-probe", type=Path, required=True)
    parser.add_argument("--control-probe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    evidence_dir = args.output.with_suffix("").with_name(args.output.stem + "_artifacts")
    evidence_dir.mkdir(exist_ok=False)
    assert importlib.metadata.version("unswbc") == "1.2.7"
    assert sha(HERE.parent / "main.cpp") == "cea6542e3c5d8b462730456afa0289503f06e7d507098a5a39b38af9cf005857"
    assert sha(ROOT / "tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp") == "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
    cases = [run(binary, probe, layout, candidate, evidence_dir)
             for candidate, binary, probe in ((False, args.control, args.control_probe),
                                              (True, args.candidate, args.candidate_probe))
             for layout in ("r55", "r39")]
    payload = {"passed": True, "case_count": len(cases), "toolkit_version": "1.2.7",
        "engine_sha256": sha(WASM_PATH), "script_sha256": sha(__file__),
        "probe_source_sha256": sha(HERE / "room_native_probe.cpp.txt"),
        "candidate_main_sha256": sha(HERE.parent / "main.cpp"),
        "h14_main_sha256": sha(ROOT / "tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"),
        "official_default_map_sha256": sha(ROOT / "battlecode_quickstart/maps/default.map"),
        "scope": "Fresh-process round0 fixtures retaining default topology and observable historical pearl layouts; not historical production memories or competitive games",
        "termination": "All survivors intentionally receive ENDTURN at round1; those deaths are not competitive outcomes",
        "cases": cases}
    with args.output.open("x", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({k: payload[k] for k in ("passed", "case_count", "toolkit_version", "engine_sha256")}))


if __name__ == "__main__":
    main()
