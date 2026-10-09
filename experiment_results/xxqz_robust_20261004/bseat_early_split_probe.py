#!/usr/bin/env python3
"""Read-only early split allocations and matched initial population growth."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS


def scan(game):
    path = Path(game["replay"])
    packed = path.read_bytes()
    assert hashlib.sha256(packed).hexdigest() == game["replay_sha256"]
    root = Message(packed).structure(0, 0)
    width, height, bodies, teams = parse_map(root.text(0))
    initial = {d: list(b) for d, b in bodies.items()}
    splits, counters, checkpoints, turns = [], {s: Counter() for s in "AB"}, {}, []
    turn, round_index, turn_logs = None, -1, []

    def visible(head, c):
        return min(abs(head[0] - c[0]), width - abs(head[0] - c[0])) <= 3 and \
            min(abs(head[1] - c[1]), height - abs(head[1] - c[1])) <= 3

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
            if round_index in (20, 50, 100, 200, 350):
                checkpoints[round_index] = {s: dict(counters[s]) for s in "AB"}
        elif kind == 1:
            d = data.number(0)
            turn_logs = []
            turn = {"id": d, "round": round_index, "turn_event": index,
                    "side": teams[d], "body_before": list(bodies[d])}
            if round_index < 100:
                turns.append(turn)
        elif kind == 6:
            turn_logs.append(data.text(0))
        elif kind == 4:
            d, action = data.number(0), data.child(0)
            action_kind = action.number(0, "H")
            if action_kind == 0:
                turn["action"] = "MOVE " + "".join("NESW"[v] for v in action.uint16s(0))
            elif action_kind == 1:
                turn["action"] = f"SPLIT {action.number(4)}"
            else:
                turn["action"] = "SUICIDE"
            turn["action_event"] = index
            turn["logs"] = list(turn_logs)
        elif kind == 9:
            d = data.number(0)
            body = bodies[d]
            head, tail = point(data.child(0)), point(data.child(1))
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            if turn is not None and turn["id"] == d:
                turn.setdefault("moves", []).append({"event": index, "body_after": list(body)})
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            side = teams[parent]
            queen = parent in (0, 1)
            category = "queen" if queen else "nonqueen"
            before = list(bodies[parent])
            counters[side]["births"] += 1
            counters[side][category + "_births"] += 1
            parent_body = [point(p) for p in data.structs(0)]
            child_body = [point(p) for p in data.structs(1)]
            allocated_length = len(parent_body) + len(child_body)
            # Initial position events can duplicate the initial head in the
            # generic stream reconstruction; official split arrays are exact.
            before = parent_body + list(reversed(child_body))
            if allocated_length >= 5:
                counters[side][category + "_LEN5plus_births"] += 1
            if allocated_length >= 6:
                counters[side][category + "_LEN6plus_births"] += 1
            if allocated_length >= 8:
                counters[side][category + "_LEN8plus_births"] += 1
            head = before[0]
            window = {((head[0] + dx) % width, (head[1] + dy) % height)
                      for dx in range(-3, 4) for dy in range(-3, 4)}
            visible_own = {c for c in before if c in window}
            enemies = [d for d, b in bodies.items() if teams[d] != side and
                       any(c in window for c in b)]
            if round_index < 350 and allocated_length >= 5:
                splits.append({"round": round_index, "event": index,
                    "parent": parent, "child": child, "side": side,
                    "queen": queen, "length_before": allocated_length, "body_before": before,
                    "parent_body": list(parent_body), "child_body": list(child_body),
                    "whole_body_current_visible": len(visible_own) == len(before),
                    "enemy_part_ids": enemies, "parent_logs": list(turn_logs),
                    "population_before": sum(teams[d] == side for d in bodies)})
            bodies[parent], bodies[child] = parent_body, child_body
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            d = data.number(0)
            category = "queen" if d in (0, 1) else "nonqueen"
            counters[teams[d]]["deaths"] += 1
            counters[teams[d]][category + "_deaths"] += 1
            del bodies[d]
    for split in splits:
        if split["round"] < 100:
            for category, d in (("parent", split["parent"]), ("child", split["child"])):
                ts = [t for t in turns if t["id"] == d and t["turn_event"] > split["event"]]
                split[category + "_first_turns"] = ts[:3]
    return {"game": game["game"], "map": game["map"], "seed": game["seed"],
            "outcome": game["outcome"], "replay": game["replay"],
            "replay_sha256": game["replay_sha256"], "initial": initial,
            "checkpoints": checkpoints, "early_LEN5plus_splits": splits}


def main():
    output = HERE / "bseat_early_split_probe_v2.json"
    assert not output.exists()
    games = []
    for base in (82001, 83001, 84001):
        audit = json.loads((RUNS / f"robust_h14_{base}" / "verified_audit.json").read_text())
        assert audit["summary"]["audit_passed"]
        for game in audit["games"]:
            if game["opponent"] == "xxqztest" and game["bot1_side"] == "B":
                games.append({"base": base, **scan(game)})
                print(f"scanned {len(games)}/51", flush=True)
    payload = {"method": "Existing 51 audited B-seat H14/xx native1.2.7 games; "
        "SHA checked. Checkpoint counts precede round r. Split events are official "
        "actions, not proof of ordinary branch. Current visibility is geometrically "
        "reconstructed radius3; full actual bodies are offline diagnosis only. "
        "No bot modifications, new matches or reserved seeds.",
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), "games": games}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")


if __name__ == "__main__":
    main()
