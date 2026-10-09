#!/usr/bin/env python3
"""Offline visible-local late growth diagnosis; does not run or modify bots."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS

LABELS = ("robust_h14_82001", "robust_h14_83001", "robust_h14_84001")
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"


def scan(game):
    packed = Path(game["replay"]).read_bytes()
    assert hashlib.sha256(packed).hexdigest() == game["replay_sha256"]
    root = Message(packed).structure(0, 0)
    width, height, dragons, teams = parse_map(root.text(0))
    side = game["bot1_side"]
    board, population = {}, Counter(teams.values())
    histories, first_observe, last_visible_enemy = {}, {}, {}
    late_actions, first8, pickups, late4 = [], [], Counter(), Counter()
    phase_quiet_splits = []
    round_index, active, turn = -1, None, None
    pending_removed, turn_logs = set(), []

    def put_body(dragon, old, new):
        for c in old:
            if c in board:
                board[c] = {owner for owner in board[c] if owner[0] != dragon}
                if not board[c]:
                    board.pop(c)
        for part, c in enumerate(new):
            # Preserve all official body entries rather than discarding a
            # momentary overlap in intermediate position/split events.
            board.setdefault(c, set()).add((dragon, part))

    for dragon, body in dragons.items():
        put_body(dragon, [], body)

    def history(dragon):
        if dragon not in histories:
            histories[dragon] = {"id": dragon, "birth_round": round_index,
                "max_length": len(dragons[dragon]), "first8": None,
                "first_action_after8": None, "first_split_after8": None,
                "late6_actions": 0, "late6_splits": 0, "late6_pickups": 0,
                "last_actions": []}
        return histories[dragon]

    def reach8(dragon, index, origin):
        if teams[dragon] != side or dragon in (0, 1):
            return
        h = history(dragon)
        h["max_length"] = max(h["max_length"], len(dragons[dragon]))
        if len(dragons[dragon]) >= 8 and h["first8"] is None:
            item = {"id": dragon, "round": round_index, "event_index": index,
                    "length": len(dragons[dragon]), "head": dragons[dragon][0],
                    "origin": origin, "preceding_actions": list(h["last_actions"])}
            h["first8"] = item
            first8.append(item)

    def observation(dragon):
        head = dragons[dragon][0]
        local_counts, local_heads, overlap_tiles = Counter(), [], 0
        for c in {((head[0] + dx) % width, (head[1] + dy) % height)
                  for dx in range(-3, 4) for dy in range(-3, 4)}:
            if c in board:
                overlap_tiles += len(board[c]) > 1
                for other, part in board[c]:
                    local_counts[other] += 1
                    if part == 0:
                        local_heads.append(other)
        first_observe.setdefault(dragon, round_index)
        last_visible_enemy.setdefault(dragon, round_index)
        enemies = [d for d in local_counts if teams[d] != side]
        if enemies:
            last_visible_enemy[dragon] = round_index
        friends = [{"id": d, "lower_bound": local_counts[d], "actual_length": len(dragons[d]),
                    "head": dragons[d][0]} for d in local_heads
                   if teams[d] == side and d != dragon]
        return {"enemy_part_ids": enemies, "friendly_heads": sorted(friends, key=lambda x: x["id"]),
                "local_overlap_tiles": overlap_tiles,
                "queen_part_visible": any(d in (0, 1) and teams[d] == side for d in local_counts),
                "first_observe": first_observe[dragon],
                "last_visible_enemy": last_visible_enemy[dragon]}

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            turn, active = data.number(0), None
            pending_removed, turn_logs = set(), []
        elif kind == 6:
            turn_logs.append(data.text(0))
        elif kind == 4:
            dragon, action = data.number(0), data.child(0)
            if teams[dragon] != side or dragon in (0, 1):
                continue
            h = history(dragon)
            view = observation(dragon)
            length = len(dragons[dragon])
            action_kind = action.number(0, "H")
            active = {"id": dragon, "round": round_index, "event_index": index,
                "length": length, "head": dragons[dragon][0], "body": list(dragons[dragon]),
                "kind": action_kind, "directions": action.uint16s(0) if action_kind == 0 else [],
                "split_requested": action.number(4) if action_kind == 1 else 0,
                "population": population[side], "logs": list(turn_logs), **view}
            active["view_quiet8_proxy"] = (not view["queen_part_visible"] and population[side] >= 4
                and round_index - view["first_observe"] >= 8
                and round_index - view["last_visible_enemy"] >= 8)
            active["no_larger_lower_bound_head"] = all(f["lower_bound"] <= length for f in view["friendly_heads"])
            active["no_larger_actual_head"] = all(f["actual_length"] <= length for f in view["friendly_heads"])
            if length >= 8:
                reach8(dragon, index, "before_action")
                if h["first_action_after8"] is None:
                    h["first_action_after8"] = active
            if h["first8"] is not None and action_kind == 1 and h["first_split_after8"] is None:
                h["first_split_after8"] = active
            if round_index >= 350 and length >= 6:
                late_actions.append(active)
                h["late6_actions"] += 1
                h["late6_splits"] += action_kind == 1
            if round_index >= 350 and length >= 4:
                late4[f"actions_length{length}"] += 1
                if action_kind == 1:
                    late4[f"splits_length{length}"] += 1
                    if (active["view_quiet8_proxy"] and population[side] >= 8
                        and round_index < 470 and active["split_requested"] == 2
                        and not any("PROTECT_DIRECT" in log for log in turn_logs)):
                        phase_quiet_splits.append(active)
            h["last_actions"].append(active)
            h["last_actions"] = h["last_actions"][-4:]
        elif kind == 3:
            if not data.number(0, "B") & 1:
                pending_removed.add(point(data.child(0)))
        elif kind == 9:
            dragon = data.number(0)
            old = list(dragons[dragon])
            head, tail = point(data.child(0)), point(data.child(1))
            body = dragons[dragon]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            put_body(dragon, old, body)
            if teams[dragon] == side and dragon not in (0, 1):
                reach8(dragon, index, "movement")
                if turn == dragon and head in pending_removed and round_index >= 350:
                    pickups[f"length_before_{len(old)}"] += 1
                    if len(old) >= 6:
                        history(dragon)["late6_pickups"] += 1
                    if active is not None:
                        active.setdefault("pearl_pickups", []).append({"tile": head,
                            "length_before_step": len(old), "length_after_step": len(body)})
            pending_removed.clear()
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            old = list(dragons[parent])
            parent_body = [point(p) for p in data.structs(0)]
            child_body = [point(p) for p in data.structs(1)]
            put_body(parent, old, parent_body)
            put_body(child, [], child_body)
            dragons[parent], dragons[child] = parent_body, child_body
            teams[child] = "AB"[data.number(8, "H")]
            population[teams[child]] += 1
            if teams[parent] == side and parent not in (0, 1):
                if active is not None and active["id"] == parent:
                    active["split_result"] = {"child": child, "parent_length": len(parent_body),
                        "child_length": len(child_body), "parent_body": list(parent_body), "child_body": list(child_body)}
                reach8(parent, index, "split_parent")
                reach8(child, index, "split_child")
        elif kind == 11:
            dragon = data.number(0)
            reason = REASONS[data.number(4, "H")]
            if teams[dragon] == side and dragon not in (0, 1):
                history(dragon)["death"] = {"round": round_index, "reason": reason,
                    "length": len(dragons[dragon]), "head": dragons[dragon][0], "event_index": index}
                if active is not None and active["id"] == dragon:
                    active["own_death"] = reason
            put_body(dragon, dragons[dragon], [])
            population[teams[dragon]] -= 1
            dragons.pop(dragon)
        elif kind > 12:
            raise ValueError(f"Unsupported event {kind}")
    actual = {s: {"population": sum(teams[d] == s for d in dragons),
        "longest": max((len(b) for d, b in dragons.items() if teams[d] == s), default=0),
        "total": sum(len(b) for d, b in dragons.items() if teams[d] == s),
        "queen": len(dragons.get("AB".index(s), []))} for s in "AB"}
    assert actual == game["standings"]
    for h in histories.values():
        h.pop("last_actions")
        if h["id"] in dragons:
            h["final_length"] = len(dragons[h["id"]])
    return {"game": game["game"], "map": game["map"], "seed": game["seed"],
            "side": side, "outcome": game["outcome"], "replay": game["replay"],
            "replay_sha256": game["replay_sha256"], "standings": game["standings"],
            "late6_actions": late_actions, "first8": first8,
            "late4_counts": dict(late4), "phase_quiet_split2_before470": phase_quiet_splits,
            "late_pickups": dict(pickups), "unit_histories": list(histories.values())}


def summarize(games):
    actions = [a for g in games for a in g["late6_actions"]]
    splits = [a for a in actions if a["kind"] == 1]
    quiet = [a for a in splits if a["view_quiet8_proxy"] and a["no_larger_lower_bound_head"]]
    first8 = [h for g in games for h in g["unit_histories"] if h["first8"] is not None]
    phase = [a for g in games for a in g["phase_quiet_split2_before470"]]
    late4 = Counter()
    for g in games:
        late4.update(g["late4_counts"])
    return {"games": len(games), "wins": sum(g["outcome"] == "win" for g in games),
        "late4_counts": dict(late4), "phase_quiet_split2_before470": len(phase),
        "phase_quiet_split_lengths": dict(Counter(a["length"] for a in phase)),
        "phase_quiet_split_game_count": sum(bool(g["phase_quiet_split2_before470"]) for g in games),
        "late6_actions": len(actions), "late6_splits": len(splits),
        "late6_split_initial_lengths": dict(Counter(a["length"] for a in splits)),
        "late6_splits_viewquiet8_nolarger_lowerbound": len(quiet),
        "quiet_split_lengths": dict(Counter(a["length"] for a in quiet)),
        "quiet_splits_actual_larger_head_present": sum(not a["no_larger_actual_head"] for a in quiet),
        "ever8_nonqueens": len(first8),
        "ever8_firstfollowing_action_split": sum(h["first_action_after8"] is not None
            and h["first_action_after8"]["kind"] == 1 for h in first8),
        "first8_origins": dict(Counter(h["first8"]["origin"] for h in first8)),
        "first8_following_split_sizes": dict(Counter(h["first_action_after8"]["split_requested"]
            for h in first8 if h["first_action_after8"] is not None and h["first_action_after8"]["kind"] == 1)),
        "first8_at350plus": sum(h["first8"]["round"] >= 350 for h in first8),
        "first8_at350plus_firstfollowing_action_split": sum(h["first8"]["round"] >= 350
            and h["first_action_after8"] is not None and h["first_action_after8"]["kind"] == 1 for h in first8)}


def main():
    if len(sys.argv) > 1:
        if sys.argv[1:] == ["--help"]:
            print(__doc__)
            print("No arguments: scan the three fixed H14 development batches, write new artifacts only.")
            return
        raise SystemExit("No arguments supported except --help; scope is source-frozen.")
    output = HERE / "h14_late_growth.json"
    assert not output.exists() and not output.with_suffix(".md").exists()
    games = []
    for label in LABELS:
        config = json.loads((RUNS / label / "run.json").read_text())
        audit = json.loads((RUNS / label / "verified_audit.json").read_text())
        assert config["sources"][0]["files"]["main.cpp"] == H14_MAIN
        assert config["unswbc"] == "1.2.7" and not config["sandbox"]
        assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
        selected = [g for g in audit["games"] if g["opponent"] == "xxqztest"]
        assert len(selected) == 34
        for game in sorted(selected, key=lambda g: int(g["game"])):
            games.append({"label": label, **scan(game)})
            if len(games) % 8 == 0:
                print(f"scanned {len(games)}/102", flush=True)
    by_map = {m: summarize([g for g in games if g["map"] == m]) for m in sorted({g["map"] for g in games})}
    method = ("102 audited H14/xx development games, replay SHA and final standings verified. "
        "Local view is the wrapped radius-three square; visible body counts are lower bounds. "
        "view_quiet8_proxy uses own observed visible-enemy-part rounds only, not sonar sightings, "
        "so is not the exact production consolidateQuiet predicate. Actual neighbor length is "
        "offline diagnosis only. Recorded splits are official actions, not assumed routine branch; "
        "some may be emergency/protection splits. No bot, opponent or reserved data was changed/read.")
    payload = {"method": method, "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "source_main_sha256": H14_MAIN, "labels": LABELS, "summary": summarize(games),
        "by_map": by_map, "games": games}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    lines = ["# H14 Late Growth", "", method, "", "Descriptive trajectories only, three development bases; no significance test.", "",
        "| Map | Games | Late LEN>=6 Actions | Splits | View-Quiet/Local-Largest Splits | New LEN>=8 After350 | First Following Action Splits |",
        "|---|---:|---:|---:|---:|---:|---:|"]
    for m, x in by_map.items():
        lines.append(f'| {m} | {x["games"]} | {x["late6_actions"]} | {x["late6_splits"]} | '
            f'{x["late6_splits_viewquiet8_nolarger_lowerbound"]} | {x["first8_at350plus"]} | '
            f'{x["first8_at350plus_firstfollowing_action_split"]} |')
    lines += ["", "## Total", "", "```json", json.dumps(payload["summary"], indent=2), "```"]
    with output.with_suffix(".md").open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps(payload["summary"]), flush=True)


if __name__ == "__main__":
    main()
