#!/usr/bin/env python3
"""Native bot/official 1.2.7 terminal fixtures, not competitive matches."""
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


def fixture_map(mirror, terminal_enemy):
    original = ROOT / "battlecode_quickstart/maps/stripes.map"
    target = (6, 1) if mirror else (17, 10)
    pearls = {target, (23 - target[0], 11 - target[1]), (17, 9), (6, 2)}
    lines = []
    for row in original.read_text().splitlines():
        words = row.split()
        if words[0] in ("DRAGON_COUNT", "DRAGON", "END"):
            continue
        if words[0] == "TILE":
            x, y = map(int, words[1:3])
            row = f"TILE {x} {y} {int((x, y) in pearls)} {int((x, y) in pearls)}"
        lines.append(row)
    body = [(16, 10), (16, 9), (17, 8), (18, 8)]
    if mirror:
        body = [(23 - x, 11 - y) for x, y in body]
    queens = ["DRAGON 0 2 21 1 22 1", "DRAGON 1 2 2 10 1 10"] if terminal_enemy else [
        "DRAGON 0 2 11 5 12 5", "DRAGON 1 2 12 6 11 6"]
    lines += ["DRAGON_COUNT 3", *queens,
              "DRAGON 0 4 " + " ".join(f"{x} {y}" for x, y in body), "END"]
    return ("\n".join(lines) + "\n").encode(), body


def run(binary, mirror, candidate, terminal_enemy=False):
    engine = EngineModule()
    pool = Pool([str(binary.resolve())], size=0)
    live, replies, deaths, notices = {}, [], [], []

    def spawn(dragon, init):
        if dragon >= 2:
            live[dragon] = Bot(pool, init=init, name=str(dragon))

    def reply(dragon, block):
        round_index = int(block.decode().splitlines()[0].split()[1])
        # Deliberately stop the deterministic fixture after two child turns.
        if round_index >= 3:
            out = b"ENDTURN\n"
        elif dragon < 2:
            if terminal_enemy:
                action = "W" if dragon == 0 else "E"
            else:
                action = ("NNW" if dragon == 0 else "SSE")[round_index]
            out = f"MOVE {action}\nENDTURN\n".encode()
        else:
            out = live[dragon].ask(block)
            assert live[dragon].error is None, live[dragon].error
        replies.append({"id": dragon, "round": round_index, "output": out.decode()})
        return out

    def death(dragon, round_index, reason):
        deaths.append([dragon, round_index, reason])
        if dragon in live:
            live.pop(dragon).stop()

    map_bytes, before = fixture_map(mirror, terminal_enemy)
    try:
        engine.run(map_bytes, reply, death, spawn, notices.append, seed=7)
    finally:
        for bot in live.values():
            bot.stop()
        pool.close()
    root = Message(engine.replay("native fixture", "scripted fixture")).structure(0, 0)
    round_index, splits, moves = -1, [], []
    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 10:
            splits.append({"round": round_index, "parent": data.number(0), "child": data.number(4),
                           "parent_body": [point(p) for p in data.structs(0)],
                           "child_body": [point(p) for p in data.structs(1)]})
        elif kind == 9:
            moves.append({"round": round_index, "id": data.number(0),
                          "head": point(data.child(0)), "tail": point(data.child(1))})
    target = (6, 1) if mirror else (17, 10)
    first = next(item for item in replies if item["id"] == 2 and item["round"] == 0)
    if terminal_enemy:
        assert candidate and mirror
        assert "pickup 4" in first["output"], first
        second = next(r for r in replies if r["id"] == 2 and r["round"] == 1)
        assert "SPLIT 2" in second["output"] and "POCKET_SALVAGE" not in second["output"], second
        assert splits[0]["round"] == 1 and len(splits[0]["child_body"]) == 2, splits
    elif candidate:
        assert "POCKET_SALVAGE 0 2 pickup 4" in first["output"], first
        assert len(splits) == 1, splits
        split = splits[0]
        assert split["round"] == 1 and split["parent"] == 2
        assert split["parent_body"] == [target, before[0]], split
        assert split["child_body"] == list(reversed(before[1:])), split
        child = split["child"]
        assert "reverse 5" in next(r["output"] for r in replies if r["id"] == 2 and r["round"] == 1)
        assert len([m for m in moves if m["id"] == child and m["round"] in (1, 2)]) >= 2
        assert not any(d == child and r < 3 for d, r, _ in deaths), deaths
    else:
        assert "SPLIT 2" in first["output"]
        assert splits[0]["round"] == 0 and splits[0]["parent"] == 2
        assert len(splits[0]["parent_body"]) == len(splits[0]["child_body"]) == 2
        assert [2, 2, "S"] in deaths, deaths
    return {"candidate": candidate, "mirror": mirror, "terminal_enemy": terminal_enemy,
            "binary_sha256": sha(binary),
            "fixture_map_sha256": hashlib.sha256(map_bytes).hexdigest(),
            "replies": replies, "splits": splits, "moves": moves, "deaths": deaths,
            "notices": notices}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert importlib.metadata.version("unswbc") == "1.2.7"
    cases = [run(binary, mirror, candidate)
             for candidate, binary in ((False, args.control), (True, args.candidate))
             for mirror in (False, True)]
    cases.append(run(args.candidate, True, True, terminal_enemy=True))
    payload = {"passed": True, "case_count": len(cases), "toolkit_version": "1.2.7",
               "engine_sha256": sha(WASM_PATH), "source_sha256": sha(__file__),
               "stripes_map_sha256": sha(ROOT / "battlecode_quickstart/maps/stripes.map"),
               "cases": cases}
    with args.output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({k: payload[k] for k in ("passed", "case_count", "toolkit_version", "engine_sha256")}))


if __name__ == "__main__":
    main()
