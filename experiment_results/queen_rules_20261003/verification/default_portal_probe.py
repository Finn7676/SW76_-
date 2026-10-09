#!/usr/bin/env python3
"""Read-only six-development-game portal/room action diagnosis from replays."""
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
SUITES = HERE.parent
sys.path.insert(0, str(SUITES))
from analyze import Message, parse_map, point
from default_room_diagnosis import topology


def scan(path, side):
    root = Message(path.read_bytes()).structure(0, 0)
    text = root.text(0)
    width, height, bodies, teams = parse_map(text)
    rooms = topology(text)
    room_for = {tuple(c): room["id"] for room in rooms for c in room["coords"]}
    edges, endpoints = {}, defaultdict(set)
    for line in text.splitlines():
        words = line.split()
        if words[0] != "EDGE":
            continue
        key, kind, pid = map(int, words[1:4])
        boundary, x = divmod(key, width + 1)
        if boundary % 2 == 0:
            links = [((x, boundary // 2 % height), 0), ((x, (boundary // 2 - 1) % height), 2)]
        else:
            y = (boundary - 1) // 2
            links = [((x % width, y), 3), (((x - 1) % width, y), 1)]
        for c, d in links:
            edges[c, d] = kind, pid
            if kind == 2:
                endpoints[pid].add((c, d))

    def geo(c, d):
        dx, dy = ((0, -1), (1, 0), (0, 1), (-1, 0))[d]
        return (c[0] + dx) % width, (c[1] + dy) % height

    def visible(a, b):
        return all(min(abs(a[i] - b[i]), (width, height)[i] - abs(a[i] - b[i])) <= 3 for i in (0, 1))

    def destination(c, d):
        kind, pid = edges.get((c, d), (0, -1))
        if kind == 1:
            return None
        if kind == 0:
            return geo(c, d)
        for target, td in endpoints[pid]:
            if td == d and target != c:
                return geo(target, td)
        return None

    def pearl_routes(state):
        queue = [(state["head"], [])]
        seen = {state["head"]}
        routes = []
        for c, path in queue:
            if len(path) >= 3:
                continue
            for d in range(4):
                if edges.get((c, d), (0, -1))[0] != 0:
                    continue
                q = geo(c, d)
                if q in seen or not visible(state["head"], q) or q in state["occupied"]:
                    continue
                seen.add(q)
                steps = path + [(d, q)]
                queue.append((q, steps))
                if q not in state["visible_pearls"]:
                    continue
                for next_dir in range(4):
                    r = geo(q, next_dir)
                    if edges.get((q, next_dir), (0, -1))[0] == 0 and visible(state["head"], r) and \
                            r not in state["occupied"] and r not in [p for _, p in steps] and r != state["head"]:
                        routes.append({"directions": "".join("NESW"[d] for d, _ in steps),
                                       "cells": [p for _, p in steps], "pearl": q,
                                       "clear_continuation": r})
                        break
        return routes

    pearls = set()
    memory = defaultdict(set)
    actions, turns, crosses, room_stats = [], {}, [], {}
    alive = {}
    for room in rooms:
        room_stats[room["id"]] = {"size": room["size"], "head_rounds": Counter(),
                                  "pickups": Counter(), "splits": Counter(),
                                  "deaths": Counter(), "entries": [], "exits": []}
    round_index, acting = -1, None
    consumed = Counter()
    previous_cross = {}
    crossed_by = defaultdict(list)
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
            if round_index in (30, 50, 100, 200):
                alive[round_index] = {s: {"population": sum(teams[d] == s for d in bodies),
                                        "total": sum(len(b) for d, b in bodies.items() if teams[d] == s)}
                                      for s in "AB"}
        elif kind == 1:
            acting = data.number(0)
            if acting not in bodies:
                continue
            body = bodies[acting]
            head = body[0]
            window = {((head[0] + dx) % width, (head[1] + dy) % height)
                      for dy in range(-3, 4) for dx in range(-3, 4)}
            memory[acting].update(window)
            occupied = {c: d for d, b in bodies.items() for c in b}
            enemy_heads = [{"id": d, "head": b[0], "length": len(b)} for d, b in bodies.items()
                           if teams[d] != teams[acting] and visible(head, b[0])]
            rid = room_for.get(head)
            if rid is not None:
                room_stats[rid]["head_rounds"][teams[acting]] += 1
            turns[acting] = {"round": round_index, "turn_event": index, "id": acting,
                             "side": teams[acting], "body": list(body), "head": head,
                             "visible_pearls": sorted(pearls & window),
                             "visible_enemy_heads": enemy_heads,
                             "occupied": occupied, "known": set(memory[acting]),
                             "population": sum(teams[d] == teams[acting] for d in bodies),
                             "step": 0, "room": rid}
        elif kind == 3:
            c, present = point(data.child(0)), bool(data.number(0, "B") & 1)
            if present:
                pearls.add(c)
            else:
                pearls.discard(c)
                if acting in teams:
                    consumed[acting] += 1
                    rid = room_for.get(c)
                    if rid is not None:
                        room_stats[rid]["pickups"][teams[acting]] += 1
        elif kind == 4:
            did, act = data.number(0), data.child(0)
            if did not in turns:
                continue
            action_kind = act.number(0, "H")
            dirs = act.uint16s(0) if action_kind == 0 else []
            state = turns[did]
            state["action_event"] = index
            state["action"] = "MOVE " + "".join("NESW"[d] for d in dirs) if action_kind == 0 else \
                f"SPLIT {act.number(4)}" if action_kind == 1 else "SUICIDE"
            state["dirs"] = dirs
            if round_index <= 200:
                opportunities = []
                for d in range(4):
                    edge_kind, pid = edges.get((state["head"], d), (0, -1))
                    if edge_kind != 2:
                        continue
                    target = destination(state["head"], d)
                    known = target in state["known"]
                    occupied = state["occupied"].get(target)
                    room = room_for.get(target)
                    if target is not None:
                        opportunities.append({"dir": "NESW"[d], "pid": pid, "target": target,
                                              "target_seen_by_self": known,
                                              "target_current_visible": visible(state["head"], target),
                                              "target_occupied": occupied, "target_room": room,
                                              "own_prior_crosses": [c for c in crossed_by[did] if c["pid"] == pid]})
                if opportunities:
                    row = ({k: state[k] for k in ("round", "turn_event", "action_event", "id", "side", "population",
                                                        "body", "head", "room", "action", "visible_pearls",
                                                        "visible_enemy_heads")} |
                                   {"portal_options": opportunities, "ordinary_pearl_routes": pearl_routes(state),
                                    "visible_parts": [{"cell": c, "id": did, "side": teams[did],
                                                       "head": bodies[did][0] == c}
                                                      for c, did in state["occupied"].items()
                                                      if visible(state["head"], c)]})
                    if (path.name, did, round_index) in (("0040_xxqztest_default_seed82003_B.replay", 5, 39),
                                                        ("0040_xxqztest_default_seed84003_B.replay", 5, 55)):
                        row["self_observed_cells"] = sorted(state["known"], key=lambda p: (p[1], p[0]))
                    actions.append(row)
        elif kind == 9:
            did = data.number(0)
            body = bodies[did]
            old_head, head, tail = body[0], point(data.child(0)), point(data.child(1))
            state = turns.get(did)
            if state and state.get("dirs") and state["step"] < len(state["dirs"]):
                d = state["dirs"][state["step"]]
                state["step"] += 1
                edge_kind, pid = edges.get((old_head, d), (0, -1))
                if edge_kind == 2:
                    prev = previous_cross.get(did)
                    crossing = {"round": round_index, "event": index, "id": did, "side": teams[did],
                                "pid": pid, "from": old_head, "to": head, "length": len(body),
                                "from_room": room_for.get(old_head), "to_room": room_for.get(head),
                                "action": state["action"], "visible_pearls": state["visible_pearls"],
                                "visible_enemy_heads": state["visible_enemy_heads"],
                                "previous_cross": prev,
                                "new_pickups_since_previous": consumed[did] - prev["pickups"] if prev else None}
                    crosses.append(crossing)
                    crossed_by[did].append({"round": round_index, "event": index, "pid": pid,
                                            "from": old_head, "dir": "NESW"[d], "to": head})
                    previous_cross[did] = {"round": round_index, "pid": pid, "from": old_head,
                                           "to": head, "pickups": consumed[did]}
            old_room, new_room = room_for.get(old_head), room_for.get(head)
            if old_room != new_room:
                row = {"round": round_index, "event": index, "id": did, "side": teams[did],
                       "from": old_head, "to": head, "length": len(body)}
                if old_room is not None:
                    room_stats[old_room]["exits"].append(row)
                if new_room is not None:
                    room_stats[new_room]["entries"].append(row)
            body.insert(0, head)
            while body[-1] != tail:
                body.pop()
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            rid = room_for.get(bodies[parent][0])
            if rid is not None:
                room_stats[rid]["splits"][teams[parent]] += 1
        elif kind == 11:
            did = data.number(0)
            if did in bodies:
                rid = room_for.get(bodies[did][0])
                if rid is not None:
                    room_stats[rid]["deaths"][teams[did]] += 1
            bodies.pop(did, None)
    early = [c for c in crosses if c["round"] < 80]
    bounces = [c for c in early if c["previous_cross"] and c["previous_cross"]["pid"] == c["pid"] and
               c["previous_cross"]["round"] == c["round"] - 1 and c["new_pickups_since_previous"] == 0]
    return {"path": str(path), "side": side, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "checkpoints": alive, "room_stats": room_stats, "early_crosses": early,
            "immediate_bounces": bounces, "portal_actions": actions,
            "limitations": "Current 7x7 visibility exact geometrically; self-known cells conservative historical window only. Sonar topology/claims not reconstructed. No private state is proposed as bot input."}


def main():
    reports = []
    for base in (82001, 83001, 84001):
        suite = SUITES / f"robust_h14_{base}"
        with (suite / "results.csv").open() as handle:
            rows = list(csv.DictReader(handle))
        for row in rows:
            if row["map"] != "default" or row["opponent"] != "xxqztest":
                continue
            reports.append({"base": base, "outcome": row["outcome"], **scan(suite / row["replay"], row["bot1_side"])})
    output = HERE / "default_portal_probe_v4.json"
    with output.open("x") as handle:
        json.dump(reports, handle, indent=2)
        handle.write("\n")
    for report in reports:
        counts = Counter(c["side"] for c in report["immediate_bounces"])
        quiet_exits = [a for a in report["portal_actions"] if a["side"] == report["side"] and
                       a["room"] in (0, 10) and a["round"] < 80 and not a["visible_enemy_heads"] and
                       a["ordinary_pearl_routes"] and a["action"] in ("MOVE N", "MOVE S")]
        print(json.dumps({"base": report["base"], "H14_side": report["side"], "outcome": report["outcome"],
                          "immediate_bounces": dict(counts), "quiet_exit_candidates": quiet_exits}))


if __name__ == "__main__":
    main()
