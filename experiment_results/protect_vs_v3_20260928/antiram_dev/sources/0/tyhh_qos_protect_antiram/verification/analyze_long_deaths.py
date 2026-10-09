#!/usr/bin/env python3
"""Reconstruct the two development Schooltime replays; emit auditable JSON.

No bot decisions or match results are changed. Uses the repository's replay
decoder and verifies reconstructed final standings against the replay footer.
Rounds are the replay's zero-based indices; threshold is round >= 200.
"""
import hashlib
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "experiment_results/protect_local_20260927"))
from analyze_replays import Message, parse_map, point, DEATH_REASONS


def analyze(path, candidate_side):
    packed = path.read_bytes()
    root = Message(packed).structure(0, 0)
    width, height, dragons, teams = parse_map(root.text(0))
    round_index, actor = -1, -1
    actions, decision_heads, removed = {}, {}, {}
    counts, lost_length = defaultdict(Counter), defaultdict(Counter)
    cases = []
    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 4:
            actor = data.number(0)
            action = data.child(0)
            action_kind = action.number(0, "H")
            actions[actor] = {"round": round_index, "kind": action_kind,
                              "value": action.uint16s(0) if action_kind == 0 else action.number(4)}
            if round_index >= 195 and len(dragons[actor]) >= 7:
                hx, hy = dragons[actor][0]
                enemies = []
                for dragon_id, body in dragons.items():
                    if teams[dragon_id] == teams[actor]:
                        continue
                    x, y = body[0]
                    dx, dy = abs(hx - x), abs(hy - y)
                    dx, dy = min(dx, width - dx), min(dy, height - dy)
                    if max(dx, dy) <= 3:
                        enemies.append({"id": dragon_id, "head": body[0], "length": len(body)})
                decision_heads[actor] = {"head": dragons[actor][0], "length": len(dragons[actor]),
                                         "visible_enemy_heads": enemies}
        elif kind == 9:
            dragon_id = data.number(0)
            head, tail = point(data.child(0)), point(data.child(1))
            body = dragons[dragon_id]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            teams[child] = "AB"[data.number(8, "H")]
            dragons[parent] = [point(p) for p in data.structs(0)]
            dragons[child] = [point(p) for p in data.structs(1)]
        elif kind == 11:
            dragon_id = data.number(0)
            reason = DEATH_REASONS[data.number(4, "H")]
            body = dragons[dragon_id]
            if round_index >= 200 and len(body) >= 8:
                owner = "economy" if teams[dragon_id] == candidate_side else "V3_portal"
                counts[owner][reason] += 1
                lost_length[owner][reason] += len(body)
                if owner == "economy":
                    attacker = dragons.get(actor, removed.get(actor, []))
                    decision = decision_heads.get(dragon_id)
                    cases.append({"round": round_index, "id": dragon_id, "length": len(body),
                                  "reason": reason, "body": list(body), "action": actions.get(dragon_id),
                                  "decision_state": decision, "active_actor": actor,
                                  "actor_team": teams.get(actor), "actor_length": len(attacker),
                                  "actor_action": actions.get(actor),
                                  "champion_at_decision": bool(decision and actions[dragon_id]["round"] >= 250 and
                                                               (dragon_id % 8 == 0 or decision["length"] >= 12))})
            removed[dragon_id] = list(body)
            dragons.pop(dragon_id)
    final = {}
    for index, side in enumerate("AB"):
        lengths = [len(body) for dragon_id, body in dragons.items() if teams[dragon_id] == side]
        calculated = [len(lengths), max(lengths, default=0), sum(lengths)]
        standing = root.child(4).child(index)
        recorded = [standing.number(0), standing.number(4), standing.number(8)]
        assert calculated == recorded, (calculated, recorded)
        final[side] = dict(zip(("population", "max_length", "total_length"), calculated))
    return {"replay": str(path.relative_to(ROOT)), "replay_sha256": hashlib.sha256(packed).hexdigest(),
            "candidate_side": candidate_side, "min_round_index": 200, "min_length": 8,
            "counts": dict(counts), "lost_length": dict(lost_length),
            "candidate_death_cases": cases, "verified_final_standings": final}


if __name__ == "__main__":
    runs = []
    for side, number in (("A", 13), ("B", 14)):
        replay = ROOT / "experiment_results/protect_vs_v3_20260928/economy_dev" / (
            f"{number:04}_V3_portal_schooltime_seed20260927_{side}.replay")
        runs.append(analyze(replay, side))
    print(json.dumps({"analysis": "long dragon deaths after round 200", "runs": runs}, indent=2))
