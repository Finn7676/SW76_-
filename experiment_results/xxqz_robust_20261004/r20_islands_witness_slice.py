#!/usr/bin/env python3
"""Read the one R20/H14 changed game, including recorded queen r30-32 events."""
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, sha
from queen_immediate_engine import canonical, action


def selected(label):
    audit_path = RUNS / label / "verified_audit.json"
    audit = json.loads(audit_path.read_text())
    assert audit["summary"]["audit_passed"]
    game = next(g for g in audit["games"] if g["opponent"] == "xxqztest" and
                g["map"] == "islands" and g["bot1_side"] == "B")
    assert sha(game["replay"]) == game["replay_sha256"]
    root = Message(Path(game["replay"]).read_bytes()).structure(0, 0)
    _, _, bodies, teams = parse_map(root.text(0))
    round_index, turn = -1, -1
    window, prefix, witness_logs, checkpoints = [], [], [], {}

    def standing():
        return {s: {"population": sum(teams[d] == s for d in bodies),
                    "total": sum(len(b) for d, b in bodies.items() if teams[d] == s),
                    "longest": max((len(b) for d, b in bodies.items() if teams[d] == s), default=0),
                    "queen": len(bodies.get("AB".index(s), []))} for s in "AB"}

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
            if round_index in (30,31,32,33,50,100,194,350,450):
                checkpoints[round_index] = standing()
        elif kind == 1:
            turn = data.number(0)
        item = canonical(event)
        if item is not None and (round_index < 30 or (round_index == 30 and turn < 1)):
            prefix.append(item)
        if kind == 6 and "QUEEN_FIRST_STEP_WITNESS" in data.text(0):
            witness_logs.append({"event": index, "round": round_index, "turn": turn,
                                 "id": data.number(0), "text": data.text(0)})
        if kind == 4 and 29 <= round_index <= 33:
            did = data.number(0)
            window.append({"event": index, "round": round_index, "turn": turn,
                           "kind": "action", "id": did, "team": teams[did],
                           "action": action(data), "body_before": list(bodies[did])})
        elif kind == 9:
            did, head, tail = data.number(0), point(data.child(0)), point(data.child(1))
            before = list(bodies[did])
            if head != before[0] or tail != before[-1]:
                bodies[did].insert(0, head)
                while len(bodies[did]) > 1 and bodies[did][-1] != tail:
                    bodies[did].pop()
            if 29 <= round_index <= 33:
                window.append({"event": index, "round": round_index, "turn": turn,
                               "kind": "move", "id": did, "team": teams[did],
                               "body_before": before, "body_after": list(bodies[did])})
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8,"H")]
        elif kind == 11:
            did = data.number(0)
            if 29 <= round_index <= 33:
                window.append({"event": index, "round": round_index, "turn": turn,
                               "kind": "death", "id": did, "team": teams[did],
                               "reason": ("wall","self","body","head","invalid_action")[data.number(4,"H")],
                               "body_before": list(bodies[did])})
            bodies.pop(did,None)
    return {"label": label, "audit_json": str(audit_path), "audit_sha256": sha(audit_path),
            "game": game, "prefix_to_before_queen_r30": prefix,
            "r29_to_r33_events": window, "witness_logs": witness_logs,
            "checkpoints_before_round": checkpoints}


def main():
    output = HERE / "r20_islands_witness_slice.json"
    assert not output.exists()
    left, right = selected("robust_h14_82001"), selected("robust_queen_witness_82001")
    assert left["prefix_to_before_queen_r30"] == right["prefix_to_before_queen_r30"]
    prefix = left["prefix_to_before_queen_r30"]
    for arm in (left,right):
        arm.pop("prefix_to_before_queen_r30")
    payload = {"passed": True, "source_sha256": sha(__file__),
               "scope": "Audited recorded event slice only; no match or action-tape engine run",
               "prefix_equal_before_queen_r30": True, "prefix_canonical_events": len(prefix),
               "prefix_sha256": hashlib.sha256(json.dumps(prefix,separators=(",",":")).encode()).hexdigest(),
               "arms": [left,right],
               "limitations": ["Recorded whole-board bodies describe actual events, not candidate information eligibility.",
                               "Checkpoint r is before round r; early-ended H14 games are not padded.",
                               "A one-game downstream difference is not a win-rate or causal resource claim."]}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2); handle.write("\n")
    for arm in (left,right):
        print(arm["label"],arm["witness_logs"])
        for item in arm["r29_to_r33_events"]:
            if item["id"] == 1 or (item["kind"] == "death" and item["turn"] in (1,8,14)):
                print(item)
    print("output",output,"prefix_events",len(prefix))


if __name__ == "__main__":
    main()
