#!/usr/bin/env python3
"""Deterministic 1.2.7 rule fixtures, not competitive bot matches."""
import argparse
import hashlib
import importlib.metadata
import json
import sys
from dataclasses import asdict
from pathlib import Path

from unswbc.engine import EngineModule, WASM_PATH

ROOT = Path(__file__).resolve().parents[3]
PARSER = ROOT / "experiment_results/protect_local_20260927/analyze_replays.py"
sys.path.insert(0, str(PARSER.parent))
from analyze_replays import Message, point


def horizontal(length, x=16, y=8):
    return [(x - i, y) for i in range(length)]


def make_map(dragons, pearls=()):
    n = 32
    lines = [f"MAP {n} {n}", "SYMMETRY xy", "MAP_NAME Rules Fixture",
             f"TILE_COUNT {n * n}"]
    pearl_set = set(pearls) | {(n - 1 - x, n - 1 - y) for x, y in pearls}
    # TILE parameters are generation gaps, not an initial-pearl flag. A gap
    # of one deterministically generates at round zero on these empty cells.
    lines.extend(f"TILE {x} {y} {int((x, y) in pearl_set)} {int((x, y) in pearl_set)}"
                 for y in range(n) for x in range(n))
    lines.extend(["EDGE_COUNT 0", f"DRAGON_COUNT {len(dragons)}"])
    lines.extend(f"DRAGON {team} {len(body)} " +
                 " ".join(f"{x} {y}" for x, y in body) for team, body in dragons)
    return ("\n".join(lines + ["END"]) + "\n").encode()


def run_fixture(dragons, choose, pearls=()):
    engine = EngineModule()
    observations, deaths, calls = [], [], {}

    def reply(dragon_id, block):
        calls[dragon_id] = calls.get(dragon_id, 0) + 1
        rows = block.decode().splitlines()
        observation = {"id": dragon_id, "round": int(rows[0].split()[1]),
                       "length": int(next(r.split()[1] for r in rows if r.startswith("LENGTH ")))}
        observations.append(observation)
        return choose(dragon_id, observation["round"], calls[dragon_id])

    result = engine.run(make_map(dragons, pearls), reply,
                        lambda d, r, reason: deaths.append([d, r, reason]),
                        on_notice=lambda _: None, seed=7)
    root = Message(engine.replay("fixture A", "fixture B")).structure(0, 0)
    assert root.number(0, "I") == 2
    round_index, movements, split_events = -1, [], []
    bodies = {i: list(body) for i, (_, body) in enumerate(dragons)}
    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 9 and round_index >= 0:
            dragon_id = data.number(0)
            head, tail = point(data.child(0)), point(data.child(1))
            body = bodies[dragon_id]
            body.insert(0, head)
            while body[-1] != tail:
                body.pop()
            movements.append({"id": dragon_id, "round": round_index,
                              "head": head, "tail": tail, "length": len(body)})
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            split_events.append({"parent": parent, "child": child,
                                 "parent_length": len(bodies[parent]), "child_length": len(bodies[child])})
        elif kind == 11:
            bodies.pop(data.number(0), None)
    terminal = root.child(4)
    for i, side in enumerate("ab"):
        standing = terminal.child(i)
        assert standing.number(12) == getattr(result, f"{side}_queen")
        assert standing.number(4) == getattr(result, f"{side}_longest")
        assert standing.number(8) == getattr(result, f"{side}_length")
    return {"result": asdict(result), "observations": observations, "deaths": deaths,
            "movements": movements, "splits": split_events}


def sprint_case(length, steps, pearl_offsets=(), body=None):
    action = f"MOVE {'E' * steps}\nENDTURN\n".encode()
    info = run_fixture([(0, body or horizontal(length)), (1, horizontal(2, 24, 24))],
                       lambda d, r, _: action if d == 0 and r == 0 else
                       b"MOVE E\nENDTURN\n" if d == 1 and r == 0 else b"ENDTURN\n",
                       [(16 + offset, 8) for offset in pearl_offsets])
    moves = [m for m in info["movements"] if m["id"] == 0 and m["round"] == 0]
    current, free, expected = length, (length + 3) // 4, []
    for index in range(steps):
        paid = index >= free
        if paid and current <= 2:
            break
        current += int(index + 1 in pearl_offsets) - int(paid)
        expected.append(current)
    assert [m["length"] for m in moves] == expected, (length, steps, pearl_offsets, info)
    assert len(moves) == min(steps, len(expected))
    assert (any(d == 0 and r == 0 for d, r, _ in info["deaths"])) == (len(expected) < steps)
    return {"case": "sprint", "initial_length": length, "free_steps": free,
            "requested_steps": steps, "pearl_offsets": pearl_offsets, **info}


def queen_cases():
    cases = []
    # The queen dies while an initially present non-queen keeps team A alive.
    info = run_fixture([(0, horizontal(3)), (1, horizontal(3, 24, 24)),
                        (0, horizontal(12, 16, 16))],
                       lambda d, r, _: b"ENDTURN\n" if d == 0 or (d == 1 and r > 0)
                       else b"MOVE E\nENDTURN\n")
    assert info["result"]["a_queen"] == 0 and info["result"]["a_longest"] == 12
    assert info["result"]["a_dragons"] == 1 and info["result"]["winner"] == "A"
    assert any(o["id"] == 1 and o["round"] == 1 for o in info["observations"])
    cases.append({"case": "queen_zero_no_initial_teammate_succession", **info})
    for side in (0, 1):
        queen = side
        dragons = [(0, horizontal(8 if side == 0 else 3)),
                   (1, horizontal(8 if side == 1 else 3, 24, 24))]

        def choose(dragon_id, round_index, _):
            if dragon_id == queen and round_index == 0:
                return b"SPLIT 5\nENDTURN\n"
            if dragon_id != queen and dragon_id < 2 and round_index > 0:
                return b"ENDTURN\n"
            return b"MOVE W\nENDTURN\n" if dragon_id >= 2 else b"MOVE E\nENDTURN\n"

        info = run_fixture(dragons, choose)
        key = "a" if side == 0 else "b"
        assert info["splits"] == [{"parent": queen, "child": 2,
                                   "parent_length": 3, "child_length": 5}]
        assert info["result"][f"{key}_queen"] == 3
        assert info["result"][f"{key}_longest"] == 5
        assert info["result"][f"{key}_length"] == 8
        cases.append({"case": f"queen_{queen}_split_keeps_parent_identity", **info})

        def die_after_split(dragon_id, round_index, _):
            if dragon_id == queen:
                return b"SPLIT 5\nENDTURN\n" if round_index == 0 else b"ENDTURN\n"
            if dragon_id < 2 and round_index >= 2:
                return b"ENDTURN\n"
            return b"MOVE W\nENDTURN\n" if dragon_id >= 2 else b"MOVE E\nENDTURN\n"

        info = run_fixture(dragons, die_after_split)
        assert info["result"][f"{key}_queen"] == 0
        assert info["result"][f"{key}_longest"] == 5
        assert info["result"][f"{key}_dragons"] == 1
        cases.append({"case": f"queen_{queen}_death_no_child_succession", **info})
    return cases


def collision_case():
    body = [(16, 8), (16, 9), (17, 9), (18, 9)]
    info = run_fixture([(0, body), (1, horizontal(2, 24, 24))],
                       lambda d, r, _: b"MOVE ES\nENDTURN\n" if d == 0 else b"ENDTURN\n")
    movements = [m for m in info["movements"] if m["id"] == 0 and m["round"] == 0]
    assert len(movements) == 1 and movements[0]["length"] == 4
    assert [0, 0, "S"] in info["deaths"]
    return {"case": "paid_collision_before_tail_removal", **info}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    assert importlib.metadata.version("unswbc") == "1.2.7"
    cases = queen_cases()
    for length in (4, 5, 8, 9, 12):
        free = (length + 3) // 4
        cases.append(sprint_case(length, free))
        cases.append(sprint_case(length, free + 2))
        cases.append(sprint_case(length, free + length - 1))
    cases.extend([sprint_case(4, 2, (1,)), sprint_case(8, 3, (1,)),
                  sprint_case(2, 2, (2,)), sprint_case(2, 2, (1, 2)),
                  sprint_case(5, 3, (3,)), sprint_case(4, 5, (1, 5)), collision_case()])
    payload = {"passed": True, "case_count": len(cases), "toolkit_version": "1.2.7",
               "engine_sha256": hashlib.sha256(WASM_PATH.read_bytes()).hexdigest(),
               "fixture_source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "replay_parser_sha256": hashlib.sha256(PARSER.read_bytes()).hexdigest(), "cases": cases}
    if args.output:
        args.output.write_text(json.dumps(payload, indent=2) + "\n")
        print(json.dumps({key: payload[key] for key in ("passed", "case_count", "toolkit_version", "engine_sha256")}))
    else:
        print(json.dumps(payload, indent=2))


if __name__ == "__main__":
    main()
