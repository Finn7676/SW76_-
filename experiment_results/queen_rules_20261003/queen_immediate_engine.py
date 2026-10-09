#!/usr/bin/env python3
"""Official 1.2.7 action-tape rule evidence; no bot binaries or competition."""
from __future__ import annotations

import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import time

from unswbc.engine import EngineModule, WASM_PATH
from queen_immediate_witness import (Message, parse_map, point, topology,
                                     analyze_snapshot, all_visible_witnesses,
                                     landing, enemy_witness, visible_cells)

HERE = Path(__file__).resolve().parent
REPLAY = HERE / "robust_h14_84001/0045_xxqztest_dilemma_seed84006_A.replay"
REPLAY_SHA = "b762d013933f195a0b7e8003f71ea5d62f8b71434f64cebc0157eeb98a575832"
ENGINE_SHA = "26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546"
TARGET_ROUND, QUEEN, ATTACKER = 199, 0, 14
LETTERS = "NESW"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def action(data):
    item = data.child(0)
    kind = item.number(0, "H")
    if kind == 0:
        return "MOVE " + "".join(LETTERS[d] for d in item.uint16s(0))
    if kind == 1:
        return f"SPLIT {item.number(4)}"
    assert kind == 2
    return ""


def canonical(event):
    kind, data = event.number(0, "H"), event.child(0)
    if kind in (0, 1):
        return [kind, data.number(0)]
    if kind in (2, 3):
        return [kind, data.number(0), point(data.child(0))]
    if kind == 4:
        return [kind, data.number(0), action(data)]
    if kind == 9:
        return [kind, data.number(0), data.number(4, "H"), point(data.child(0)), point(data.child(1))]
    if kind == 10:
        return [kind, data.number(0), data.number(4), data.number(8, "H"), data.number(10, "H"),
                [point(p) for p in data.structs(0)], [point(p) for p in data.structs(1)]]
    if kind == 11:
        return [kind, data.number(0), data.number(4, "H")]
    if kind == 12:
        return [kind, data.number(0), data.number(4, "H"), data.number(6, "H"),
                data.number(12), data.number(16, "Q"), data.number(24, "H"),
                point(data.child(0)), point(data.child(1))]
    return None


def replay_data(root):
    width, height, bodies, teams = parse_map(root.text(0))
    dirs = ((0, -1), (1, 0), (0, 1), (-1, 0))
    facing = {}
    for did, body in bodies.items():
        facing[did] = next(d for d, (dx, dy) in enumerate(dirs)
                           if ((body[1][0] + dx) % width, (body[1][1] + dy) % height) == body[0])
    pearls, tape, prefix, target, after = set(), {}, [], None, None
    round_index, current_turn = -1, -1
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            current_turn = data.number(0)
            if round_index == TARGET_ROUND and current_turn == 15:
                after = {"bodies": {i: list(b) for i, b in bodies.items()}, "pearls": sorted(pearls)}
            if round_index <= TARGET_ROUND:
                tape[(round_index, current_turn)] = {"action": "", "sonar": []}
        elif kind == 3:
            cell = point(data.child(0))
            if data.number(0, "B") & 1:
                pearls.add(cell)
            else:
                pearls.discard(cell)
        elif kind == 4:
            did = data.number(0)
            if round_index == TARGET_ROUND and did == QUEEN:
                target = {"queen": QUEEN, "attack_id": ATTACKER, "next_turn": ATTACKER,
                          "directions": data.child(0).uint16s(0), "event": index,
                          "bodies": {i: list(b) for i, b in bodies.items()},
                          "teams": dict(teams), "pearls": set(pearls)}
            if round_index <= TARGET_ROUND:
                tape[(round_index, did)]["action"] = action(data)
        elif kind == 9:
            did = data.number(0)
            facing[did] = data.number(4, "H")
            body = bodies[did]
            body.insert(0, point(data.child(0)))
            tail = point(data.child(1))
            while body[-1] != tail:
                body.pop()
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            facing[child] = data.number(10, "H")
        elif kind == 11:
            bodies.pop(data.number(0), None)
        elif kind == 12 and round_index <= TARGET_ROUND:
            did, origin = data.number(0), point(data.child(0))
            direction = data.number(4, "H")
            if origin != bodies[did][0]:
                # A backwards ping follows the sender body and exits its tail;
                # replay stores that outgoing beam, not the requested direction.
                assert origin == bodies[did][-1], (did, origin, bodies[did])
                direction = (facing[did] + 2) % 4
            tape[(round_index, did)]["sonar"].append(
                f"SONAR {LETTERS[direction]} {data.number(16, 'Q')}")
        item = canonical(event)
        if item is not None and target is None:
            prefix.append(item)
    assert target is not None
    return {"width": width, "height": height, "tape": tape, "prefix": prefix,
            "target": target, "after_attacker": after}


def observation(block):
    rows = block.decode().splitlines()
    cursor = 0

    def labelled(label):
        nonlocal cursor
        words = rows[cursor].split()
        cursor += 1
        assert words[0] == label, words
        return words[1:]

    round_index = int(labelled("ROUND")[0])
    facing = labelled("DIR")[0]
    length = int(labelled("LENGTH")[0])
    units = int(labelled("UNIT_COUNT")[0])
    count = int(labelled("NUM_MSGS")[0])
    messages = [int(rows[cursor + i]) for i in range(count)]
    cursor += count
    echoes = list(map(int, labelled("ECHOES"))) if rows[cursor].startswith("ECHOES ") else None
    tiles = [list(map(int, rows[cursor + i].split())) for i in range(49)]
    cursor += 49
    count = int(labelled("DRAGON_BODIES")[0])
    parts = [rows[cursor + i].split() for i in range(count)]
    cursor += count
    edge_rows = rows[cursor:]
    return {"round": round_index, "facing": facing, "length": length, "unit_count": units,
            "messages": messages, "echoes": echoes, "tiles": tiles,
            "parts": parts, "edge_rows": edge_rows, "raw": block.decode(),
            "sha256": hashlib.sha256(block).hexdigest()}


def observed_enemy_bodies(observed, width, height, edges):
    """Infer each tail only from visible part facing and an ordinary rear edge."""
    visible = {(x, y) for x, y, _, _ in observed["tiles"]}
    groups = {}
    for team, did, x, y, facing, head in observed["parts"]:
        if team == "B":
            groups.setdefault(int(did), {})[(int(x), int(y))] = (LETTERS.index(facing), int(head))
    result = []
    for did, parts in sorted(groups.items()):
        heads = [cell for cell, (_, head) in parts.items() if head]
        if len(heads) != 1:
            result.append({"id": did, "complete": False, "reason": "head_hidden_or_ambiguous"})
            continue
        body, current = [], heads[0]
        reason = None
        while current in parts:
            if current in body:
                reason = "visible_chain_cycle"
                break
            body.append(current)
            rear_dir = (parts[current][0] + 2) % 4
            if edges.get((current, rear_dir), (0, -1))[0] != 0:
                reason = "rear_edge_nonordinary"
                break
            current = landing(current, rear_dir, width, height)
            if current not in visible:
                reason = "rear_cell_hidden"
                break
        if reason is None and set(body) != set(parts):
            reason = "disconnected_visible_parts"
        result.append({"id": did, "complete": reason is None, "reason": reason,
                       "body": body, "tail_rear_cell": current})
    return result


def observed_witnesses(target, queen_post, body_status, width, height, edges):
    visible = visible_cells(target["bodies"][QUEEN][0], width, height)
    other = {cell for did, body in target["bodies"].items() if did != QUEEN
             for cell in body if cell in visible}
    pearls = (target["pearls"] & visible) - set(map(tuple, queen_post["eaten"]))
    return [{"id": item["id"], **enemy_witness(item["body"], queen_post["body"][0],
                (other - set(item["body"])) | set(queen_post["body"]),
                pearls, visible, edges, width, height)}
            for item in body_status if item["complete"]]


def run_arm(source, move):
    start = time.perf_counter()
    engine = EngineModule()
    target_block, target_spawn = None, None
    deaths, notices, cleanup, calls = [], [], [], 0
    spawned = {}

    def spawn(did, block):
        spawned[did] = block.decode()

    def reply(did, block):
        nonlocal target_block, target_spawn, calls
        calls += 1
        round_index = int(block.decode().splitlines()[0].split()[1])
        if round_index > TARGET_ROUND or (round_index == TARGET_ROUND and did > ATTACKER):
            cleanup.append([did, round_index])
            return b"ENDTURN\n"
        entry = source["tape"][(round_index, did)]
        output = entry["action"]
        if (round_index, did) == (TARGET_ROUND, QUEEN):
            target_block = observation(block)
            target_spawn = spawned[did]
            assert output == "MOVE N"
            output = f"MOVE {move}"
        lines = ([output] if output else []) + entry["sonar"] + ["ENDTURN"]
        return ("\n".join(lines) + "\n").encode()

    engine.run(REPLAY_MAP, reply, lambda d, r, why: deaths.append([d, r, why]),
               spawn, notices.append, seed=84006)
    packed = engine.replay("action tape fixture A", "action tape fixture B")
    regenerated = replay_data(Message(packed).structure(0, 0))
    if regenerated["prefix"] != source["prefix"]:
        mismatch = next((i for i, (a, b) in enumerate(zip(regenerated["prefix"], source["prefix"]))
                         if a != b), min(len(regenerated["prefix"]), len(source["prefix"])))
        raise AssertionError(f"Physical/sonar replay prefix mismatch at {mismatch}: "
                             f"got {regenerated['prefix'][mismatch:mismatch + 2]} "
                             f"expected {source['prefix'][mismatch:mismatch + 2]}")
    assert regenerated["target"]["bodies"] == source["target"]["bodies"]
    assert regenerated["target"]["pearls"] == source["target"]["pearls"]
    assert target_block is not None
    target = source["target"]
    visible = {(x, y) for x, y, _, _ in target_block["tiles"]}
    shown = {(int(p[1]), int(p[2]), int(p[3])) for p in target_block["parts"]}
    expected = {(did, x, y) for did, body in target["bodies"].items() for x, y in body if (x, y) in visible}
    assert shown == expected
    assert {(x, y) for x, y, has_pearl, _ in target_block["tiles"] if has_pearl} == target["pearls"] & visible
    assert target_block["length"] == len(target["bodies"][QUEEN]) == 8
    after = regenerated["after_attacker"]
    assert after is not None
    immediate_deaths = [d for d in deaths if d[1] == TARGET_ROUND and d[0] in (QUEEN, ATTACKER)]
    if move == "N":
        assert immediate_deaths == [[QUEEN, TARGET_ROUND, "H"], [ATTACKER, TARGET_ROUND, "H"]]
        assert QUEEN not in after["bodies"] and ATTACKER not in after["bodies"]
    else:
        assert immediate_deaths == [[ATTACKER, TARGET_ROUND, "O"]], immediate_deaths
        assert after["bodies"][QUEEN] == [(24, 1), (24, 2), (24, 3), (25, 3),
                                          (26, 3), (26, 4), (25, 4), (24, 4)]
        assert ATTACKER not in after["bodies"]
    return {"move": move, "elapsed_seconds": time.perf_counter() - start,
            "callbacks": calls, "physical_and_sonar_prefix_equal": True,
            "prefix_canonical_events": len(source["prefix"]),
            "prefix_sha256": hashlib.sha256(json.dumps(source["prefix"]).encode()).hexdigest(),
            "queen_spawn_input": target_spawn, "queen_round199_input": target_block,
            "immediate_deaths": immediate_deaths,
            "after_attacker": after, "cleanup_turns": cleanup,
            "all_fixture_deaths": deaths, "notices": notices,
            "fixture_replay_sha256": hashlib.sha256(packed).hexdigest()}, packed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-prefix", type=Path, required=True)
    args = parser.parse_args()
    output = args.output_prefix.with_suffix(".json")
    paths = [output] + [args.output_prefix.with_name(args.output_prefix.name + f"_{move}").with_suffix(".replay")
                        for move in ("N", "NN")]
    if any(p.exists() for p in paths):
        parser.error("Output exists; preserve prior evidence")
    assert importlib.metadata.version("unswbc") == "1.2.7"
    assert sha(WASM_PATH) == ENGINE_SHA and sha(REPLAY) == REPLAY_SHA
    root = Message(REPLAY.read_bytes()).structure(0, 0)
    global REPLAY_MAP
    REPLAY_MAP = root.text(0).encode()
    source = replay_data(root)
    width, height, edges = topology(root.text(0))
    start = time.perf_counter()
    mechanism = analyze_snapshot(source["target"], width, height, edges)
    mechanism["original_all_visible_complete_witnesses"] = all_visible_witnesses(
        source["target"], mechanism["original_projection"], width, height, edges)
    for candidate in mechanism["alternatives"]:
        candidate["all_visible_complete_witnesses"] = all_visible_witnesses(
            source["target"], candidate["projection"], width, height, edges)
    search_time = time.perf_counter() - start
    arms = []
    for move, path in zip(("N", "NN"), paths[1:]):
        arm, packed = run_arm(source, move)
        with path.open("xb") as handle:
            handle.write(packed)
        arm["fixture_replay"] = str(path)
        arms.append(arm)
        print(f"{move} official fixture passed in {arm['elapsed_seconds']:.3f}s", flush=True)
    assert arms[0]["queen_round199_input"] == arms[1]["queen_round199_input"]
    input_start = time.perf_counter()
    enemy_body_status = observed_enemy_bodies(arms[0]["queen_round199_input"], width, height, edges)
    for item in enemy_body_status:
        if item["complete"]:
            assert item["body"] == source["target"]["bodies"][item["id"]]
    mechanism["input_proven_enemy_body_status"] = enemy_body_status
    mechanism["original_input_proven_witnesses"] = observed_witnesses(
        source["target"], mechanism["original_projection"], enemy_body_status, width, height, edges)
    for candidate in mechanism["alternatives"]:
        candidate["input_proven_witnesses"] = observed_witnesses(
            source["target"], candidate["projection"], enemy_body_status, width, height, edges)
    input_time = time.perf_counter() - input_start
    payload = {"passed": True, "scope": "Existing dev replay action-tape rule fixture, not competition",
               "toolkit_version": "1.2.7", "engine_sha256": ENGINE_SHA,
               "source_sha256": sha(__file__), "witness_source_sha256": sha(HERE / "queen_immediate_witness.py"),
               "replay": str(REPLAY), "replay_sha256": REPLAY_SHA, "seed": 84006,
               "map_sha256": hashlib.sha256(REPLAY_MAP).hexdigest(), "round": TARGET_ROUND,
               "search_seconds": search_time, "input_enemy_search_seconds": input_time,
               "mechanism": mechanism, "arms": arms,
               "limitations": ["Only id14 acts immediately after queen; id16 comparison holds other bodies static.",
                               "Hidden/partial enemies, portals, paths beyond four, later-turn bodies remain unknown.",
                               "All ENDTURN-only replies after id14 are deliberate fixture cleanup, not bot behavior."]}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"passed": True, "search_seconds": search_time,
                      "search_nodes": mechanism["search_nodes"], "output": str(output)}))


if __name__ == "__main__":
    main()
