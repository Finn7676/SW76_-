#!/usr/bin/env python3
"""Read-only topology and room-event diagnosis for two frozen default games."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from analyze import Message, parse_map, point

SUITE = HERE / "queen_v127"


def topology(text):
    width, height, _, _ = parse_map(text)
    edges, tiles = {}, {}
    for line in text.splitlines():
        words = line.split()
        if not words:
            continue
        if words[0] == "TILE":
            x, y, low, high = map(int, words[1:5])
            tiles[x, y] = {"min_gap": low, "max_gap": high}
        elif words[0] == "EDGE":
            edge, kind, pid = map(int, words[1:4])
            boundary_y, x = divmod(edge, width + 1)
            if boundary_y % 2 == 0:
                y = boundary_y // 2
                edges[((x, y % height), 0)] = (kind, pid)
                edges[((x, (y - 1) % height), 2)] = (kind, pid)
            else:
                y = (boundary_y - 1) // 2
                edges[((x % width, y), 3)] = (kind, pid)
                edges[(((x - 1) % width, y), 1)] = (kind, pid)
    cells = {(x, y) for y in range(height) for x in range(width)}
    rooms = []
    while cells:
        start = min(cells, key=lambda p: (p[1], p[0]))
        cells.remove(start)
        queue, boundary = [start], set()
        for c in queue:
            for d, (dx, dy) in enumerate(((0, -1), (1, 0), (0, 1), (-1, 0))):
                kind, pid = edges.get((c, d), (0, -1))
                if kind == 2:
                    boundary.add((c, d, pid))
                elif kind == 0:
                    q = ((c[0] + dx) % width, (c[1] + dy) % height)
                    if q in cells:
                        cells.remove(q)
                        queue.append(q)
        if len(queue) <= 64 and boundary:
            coords = sorted(queue, key=lambda p: (p[1], p[0]))
            rooms.append({"id": len(rooms), "size": len(coords), "coords": coords,
                          "portal_boundaries": sorted(boundary),
                          "pearl_tiles": [{"cell": p, **tiles.get(p, {"min_gap": 0, "max_gap": 0})}
                                          for p in coords if tiles.get(p, {}).get("max_gap", 0) > 0]})
    return rooms


def scan(path, rooms, side):
    root = Message(path.read_bytes()).structure(0, 0)
    _, _, dragons, teams = parse_map(root.text(0))
    room_for = {tuple(c): room["id"] for room in rooms for c in room["coords"]}
    states = [{"room": r["id"], "snapshots": [], "entries": [], "exits": [],
               "splits": [], "deaths": [], "pickups": Counter(), "head_moves": Counter(),
               "head_rounds": Counter()} for r in rooms]
    round_index, turn = -1, None
    pending_pickups = []

    def snapshot(completed):
        if completed not in (50, 100, 200):
            return
        for state in states:
            occupants = [{"id": did, "team": teams[did], "length": len(body), "head": body[0]}
                         for did, body in dragons.items() if room_for.get(body[0]) == state["room"]]
            state["snapshots"].append({"completed_rounds": completed, "heads": occupants,
                                       "team_heads": dict(Counter(o["team"] for o in occupants))})

    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            if round_index >= 0:
                snapshot(round_index + 1)
                for did, body in dragons.items():
                    rid = room_for.get(body[0])
                    if rid is not None:
                        states[rid]["head_rounds"][teams[did]] += 1
            round_index = data.number(0)
        elif kind == 1:
            turn = data.number(0)
            pending_pickups = []
        elif kind == 3:
            tile, present = point(data.child(0)), bool(data.number(0, "B") & 1)
            if not present and tile in room_for:
                pending_pickups.append(tile)
        elif kind == 9:
            did = data.number(0)
            body = dragons[did]
            head, tail, old_head = point(data.child(0)), point(data.child(1)), body[0]
            old_room, new_room = room_for.get(old_head), room_for.get(head)
            before = len(body)
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            if round_index < 0:
                continue
            if old_room != new_room:
                item = {"round_index": round_index, "id": did, "team": teams[did],
                        "from": old_head, "to": head, "length": len(body)}
                if old_room is not None:
                    states[old_room]["exits"].append(item)
                if new_room is not None:
                    states[new_room]["entries"].append(item)
            if new_room is not None:
                states[new_room]["head_moves"][teams[did]] += 1
            if turn == did and head in pending_pickups:
                states[room_for[head]]["pickups"][teams[did]] += 1
                pending_pickups.remove(head)
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            before_body = dragons[parent]
            rid = room_for.get(before_body[0])
            parent_body = [point(p) for p in data.structs(0)]
            child_body = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            if rid is not None:
                states[rid]["splits"].append({"round_index": round_index, "parent": parent,
                    "child": child, "team": teams[parent], "length_before": len(before_body),
                    "parent_length": len(parent_body), "child_length": len(child_body)})
            dragons[parent], dragons[child] = parent_body, child_body
        elif kind == 11:
            did = data.number(0)
            if did in dragons:
                rid = room_for.get(dragons[did][0])
                if rid is not None:
                    states[rid]["deaths"].append({"round_index": round_index, "id": did,
                        "team": teams[did], "length": len(dragons[did]),
                        "reason": ("wall", "self", "body", "head", "invalid_action")[data.number(4, "H")]})
            dragons.pop(did, None)
    snapshot(round_index + 1)
    for state in states:
        for field in ("pickups", "head_moves", "head_rounds"):
            state[field] = dict(state[field])
    return {"replay": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "our_side": side, "rooms": states}


def main():
    output = HERE / "default_room_diagnosis.json"
    if output.exists():
        raise SystemExit("Output exists; preserving prior evidence")
    rooms = topology((SUITE / "frozen/maps/default.map").read_text())
    print(json.dumps({"topology_rooms": rooms}), flush=True)
    games = []
    for game, side in (("0005_V3_portal_default_seed78003_A.replay", "A"),
                       ("0006_V3_portal_default_seed78003_B.replay", "B")):
        result = scan(SUITE / game, rooms, side)
        games.append(result)
        print(json.dumps({"side": side, "rooms": [{k: v for k, v in r.items()
            if k in ("room", "snapshots", "pickups", "head_rounds")} for r in result["rooms"]]}), flush=True)
    payload = {"map": str(SUITE / "frozen/maps/default.map"), "rooms": rooms, "games": games,
               "definition": "Components connected by open ordinary edges only; kelp and portals are boundaries. "
                   "Pickups require same-turn pearl-removal event and a subsequent head update to that tile. "
                   "Split events are not automatically classified as routine rather than emergency.",
               "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")


if __name__ == "__main__":
    main()
