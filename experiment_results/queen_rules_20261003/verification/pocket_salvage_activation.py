#!/usr/bin/env python3
"""Read-only R15/H14 stripes final-action audit of already completed games."""
import csv
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from analyze import Message, parse_map, point


def scan(path):
    root = Message(path.read_bytes()).structure(0, 0)
    _, _, bodies, teams = parse_map(root.text(0))
    round_num, current = -1, None
    turns, splits, deaths = [], [], {}
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_num = data.number(0)
        elif kind == 1:
            dragon = data.number(0)
            current = {"round": round_num, "event": index, "id": dragon,
                       "body_before": list(bodies[dragon]), "tags": [], "moves": []}
            turns.append(current)
        elif kind == 4 and current is not None:
            dragon, action = data.number(0), data.child(0)
            assert current["id"] == dragon
            mode = action.number(0, "H")
            current["action"] = "MOVE " + "".join("NESW"[d] for d in action.uint16s(0)) if mode == 0 else \
                f"SPLIT {action.number(4)}" if mode == 1 else "SUICIDE"
            current["action_event"] = index
        elif kind == 6 and current is not None:
            text = data.text(0)
            if "POCKET_SALVAGE" in text:
                assert data.number(0) == current["id"]
                current["tags"].append({"event": index, "text": text})
        elif kind == 9:
            dragon, head, tail = data.number(0), point(data.child(0)), point(data.child(1))
            body = bodies[dragon]
            body.insert(0, head)
            while body[-1] != tail:
                body.pop()
            if current is not None and current["id"] == dragon:
                current["moves"].append({"event": index, "body_after": list(body)})
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            split = {"round": round_num, "event": index, "parent": parent, "child": child,
                     "parent_body": list(bodies[parent]), "child_body": list(bodies[child])}
            splits.append(split)
            if current is not None and current["id"] == parent:
                current["split"] = split
        elif kind == 11:
            dragon = data.number(0)
            deaths[dragon] = {"round": round_num, "event": index, "reason": data.number(4, "H"),
                             "body_before": list(bodies.get(dragon, []))}
            bodies.pop(dragon, None)
    triggers = [turn for turn in turns if turn["tags"]]
    for turn in triggers:
        turn["death"] = deaths.get(turn["id"])
        if "split" in turn:
            child = turn["split"]["child"]
            turn["child_death"] = deaths.get(child)
            turn["first_child_turns"] = [t for t in turns if t["id"] == child][:3]
        pickup = any("pickup" in tag["text"] for tag in turn["tags"])
        reverse = any("reverse" in tag["text"] for tag in turn["tags"])
        turn["committed_match"] = bool(pickup and turn["action"].startswith("MOVE ") and
                                        turn["moves"] and len(turn["moves"][-1]["body_after"]) == 5) or \
            bool(reverse and turn["action"] == "SPLIT 3" and "split" in turn and
                 len(turn["split"]["child_body"]) == 3)
        assert turn["committed_match"], turn
    return {"replay_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "triggers": triggers, "turns": turns, "deaths": deaths}


def main():
    candidate = HERE.parent / "robust_pocket_salvage_82001"
    control = HERE.parent / "robust_h14_82001"
    rows = list(csv.DictReader((candidate / "results.csv").open()))
    control_rows = {row["replay"]: row for row in
                    csv.DictReader((control / "results.csv").open())}
    result = []
    for row in rows:
        if row["opponent"] != "xxqztest" or row["map"] != "stripes":
            continue
        cand = scan(candidate / row["replay"])
        base = scan(control / row["replay"])
        initial_control = None
        if cand["triggers"]:
            initial = cand["triggers"][0]
            matches = [t for t in base["turns"] if t["id"] == initial["id"] and
                       t["round"] == initial["round"] and
                       t["body_before"] == initial["body_before"]]
            assert len(matches) == 1, matches
            initial_control = dict(matches[0])
            initial_control["death"] = base["deaths"].get(initial_control["id"])
            if "split" in initial_control:
                child = initial_control["split"]["child"]
                initial_control["first_child_turns"] = [
                    t for t in base["turns"] if t["id"] == child][:3]
                initial_control["child_death"] = base["deaths"].get(child)
        result.append({"side": row["bot1_side"], "candidate_rounds": int(row["rounds"]),
                       "H14_rounds": int(control_rows[row["replay"]]["rounds"]),
                       "candidate_outcome": row["outcome"],
                       "H14_outcome": control_rows[row["replay"]]["outcome"],
                       "candidate_replay": str(candidate / row["replay"]),
                       "H14_replay": str(control / row["replay"]),
                       "candidate_sha256": cand["replay_sha256"], "H14_sha256": base["replay_sha256"],
                       "committed_triggers": cand["triggers"],
                       "H14_initial_same_body_control": initial_control,
                       "identity_caution": "Child numeric IDs after the initial action divergence "
                                           "are not matched actor identities across games."})
    output = HERE / "pocket_salvage_activation_v2.json"
    with output.open("x") as handle:
        json.dump(result, handle, indent=2)
        handle.write("\n")
    print(json.dumps([{ "side": r["side"], "committed_triggers": len(r["committed_triggers"]),
                       "candidate_rounds": r["candidate_rounds"]} for r in result]))


if __name__ == "__main__":
    main()
