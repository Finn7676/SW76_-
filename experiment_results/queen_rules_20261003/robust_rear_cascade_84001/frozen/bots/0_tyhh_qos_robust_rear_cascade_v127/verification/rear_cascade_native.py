#!/usr/bin/env python3
"""Official 1.2.7 same-round rear-allocation fixtures, not competitive games.

Runner mechanics are adapted from R15 verification/pocket_engine_smoke.py.
All fixture dragons deliberately receive ENDTURN at round one to finish.
"""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import sys

from unswbc.bot import Bot, Pool
from unswbc.engine import EngineModule, WASM_PATH

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "experiment_results/protect_local_20260927"))
from analyze_replays import Message, point


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fixture_map(length):
    original = ROOT / "battlecode_quickstart/maps/autarky.map"
    body = [(18, y) for y in range(2, 2 + length)]
    lines = []
    for row in original.read_text().splitlines():
        words = row.split()
        if words[0] in ("DRAGON_COUNT", "DRAGON", "END"):
            continue
        if words[0] == "TILE":
            row = f"TILE {words[1]} {words[2]} 0 0"
        elif words[0] == "EDGE":
            row = f"EDGE {words[1]} 0 -1"
        elif words[0] == "MAP_NAME":
            row = "MAP_NAME Rear Cascade Native Fixture"
        lines.append(row)
    lines += ["DRAGON_COUNT 3", "DRAGON 0 2 0 0 1 0", "DRAGON 1 2 40 10 41 10",
              f"DRAGON 0 {length} " + " ".join(f"{x} {y}" for x, y in body), "END"]
    return ("\n".join(lines) + "\n").encode(), body


def run(binary, length, candidate):
    engine = EngineModule()
    pool = Pool([str(binary.resolve())], size=0)
    live, replies, deaths, notices = {}, [], [], []

    def spawn(dragon, init):
        if dragon >= 2:
            live[dragon] = Bot(pool, init=init, name=str(dragon))

    def reply(dragon, block):
        round_index = int(block.decode().splitlines()[0].split()[1])
        if round_index >= 1:
            out = b"ENDTURN\n"
        elif dragon < 2:
            out = b"MOVE W\nENDTURN\n"
        else:
            out = live[dragon].ask(block)
            assert live[dragon].error is None, live[dragon].error
        replies.append({"id": dragon, "round": round_index, "output": out.decode()})
        return out

    def death(dragon, round_index, reason):
        deaths.append({"id": dragon, "round": round_index, "reason": reason})
        if dragon in live:
            live.pop(dragon).stop()

    map_bytes, initial = fixture_map(length)
    try:
        engine.run(map_bytes, reply, death, spawn, notices.append, seed=7)
    finally:
        for bot in live.values():
            bot.stop()
        pool.close()

    root = Message(engine.replay("native fixture", "scripted fixture")).structure(0, 0)
    round_index, splits, moves = -1, [], []
    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 10:
            splits.append({"round": round_index, "event": event_index,
                           "parent": data.number(0), "child": data.number(4),
                           "parent_body": [point(p) for p in data.structs(0)],
                           "child_body": [point(p) for p in data.structs(1)]})
        elif kind == 9:
            moves.append({"round": round_index, "event": event_index, "id": data.number(0),
                          "head": point(data.child(0)), "tail": point(data.child(1))})

    expected_sizes = ([5, 3] if length == 7 else [12, 10, 8, 6, 4, 2]) if candidate else [2]
    assert [len(s["child_body"]) for s in splits] == expected_sizes, splits
    remaining = list(initial)
    parent = 2
    for split, child_size in zip(splits, expected_sizes):
        assert split["round"] == 0 and split["parent"] == parent, split
        assert split["parent_body"] == remaining[:-child_size], split
        assert split["child_body"] == list(reversed(remaining[-child_size:])), split
        if candidate:
            assert len(split["parent_body"]) == 2, split
            a, b = split["parent_body"]
            assert abs(a[0] - b[0]) + abs(a[1] - b[1]) == 1, split
        committed = next(r for r in replies if r["id"] == parent and r["round"] == 0)
        assert f"SPLIT {child_size}\n" in committed["output"], committed
        remaining, parent = split["child_body"], split["child"]
    final_child = parent
    final_reply = next(r for r in replies if r["id"] == final_child and r["round"] == 0)
    assert any(line.startswith("MOVE ") for line in final_reply["output"].splitlines()), final_reply
    assert any(m["round"] == 0 and m["id"] == final_child and m["event"] > splits[-1]["event"]
               for m in moves), moves
    assert not any(d["round"] < 1 for d in deaths), deaths
    return {"candidate": candidate, "length": length, "binary_sha256": sha(binary),
            "fixture_map_sha256": hashlib.sha256(map_bytes).hexdigest(),
            "initial_body": initial, "expected_split_sizes": expected_sizes,
            "replies": replies, "splits": splits, "moves": moves, "deaths": deaths,
            "notices": notices}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert importlib.metadata.version("unswbc") == "1.2.7"
    cases = [run(binary, length, candidate)
             for candidate, binary in ((False, args.control), (True, args.candidate))
             for length in (7, 14)]
    payload = {"passed": True, "case_count": len(cases), "toolkit_version": "1.2.7",
               "engine_sha256": sha(WASM_PATH), "source_sha256": sha(__file__),
               "original_autarky_map_sha256": sha(ROOT / "battlecode_quickstart/maps/autarky.map"),
               "termination": "Every survivor intentionally receives ENDTURN at round one; those deaths are not competitive outcomes.",
               "cases": cases}
    with args.output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({k: payload[k] for k in ("passed", "case_count", "toolkit_version", "engine_sha256")}))


if __name__ == "__main__":
    main()
