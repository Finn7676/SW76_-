#!/usr/bin/env python3
"""Read existing H14/xx B-seat trajectories; never launch games or alter bots."""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS

BASES = (82001, 83001, 84001)
CHECKPOINTS = (20, 50, 100, 200, 300, 350, 400, 450)
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"


def scan(game):
    path = Path(game["replay"])
    packed = path.read_bytes()
    assert hashlib.sha256(packed).hexdigest() == game["replay_sha256"]
    root = Message(packed).structure(0, 0)
    width, height, bodies, teams = parse_map(root.text(0))
    edges = {}
    for line in root.text(0).splitlines():
        words = line.split()
        if words[0] != "EDGE":
            continue
        key, kind, pid = map(int, words[1:4])
        boundary, x = divmod(key, width + 1)
        if boundary % 2 == 0:
            links = [((x, boundary // 2 % height), 0),
                     ((x, (boundary // 2 - 1) % height), 2)]
        else:
            y = (boundary - 1) // 2
            links = [((x % width, y), 3), (((x - 1) % width, y), 1)]
        for c, d in links:
            edges[c, d] = kind, pid

    def adjacent(c, d):
        dx, dy = ((0, -1), (1, 0), (0, 1), (-1, 0))[d]
        return (c[0] + dx) % width, (c[1] + dy) % height

    def visible(head, c):
        return min(abs(head[0] - c[0]), width - abs(head[0] - c[0])) <= 3 and \
            min(abs(head[1] - c[1]), height - abs(head[1] - c[1])) <= 3

    board = defaultdict(set)

    def put_body(dragon, old, new):
        for c in old:
            board[c].discard(dragon)
            if not board[c]:
                del board[c]
        for c in new:
            board[c].add(dragon)

    for dragon, body in bodies.items():
        put_body(dragon, [], body)

    pearls, removed = set(), set()
    unique_heads = {s: set() for s in "AB"}
    unique_view = {s: set() for s in "AB"}
    counts = {s: Counter() for s in "AB"}
    checkpoints, traps, longest_turns = {}, [], []
    round_index, turn, turn_logs, acting = -1, None, [], None
    deaths = {}

    def standing():
        return {s: {"population": sum(teams[d] == s for d in bodies),
                    "total": sum(len(b) for d, b in bodies.items() if teams[d] == s),
                    "longest": max((len(b) for d, b in bodies.items() if teams[d] == s), default=0),
                    "queen": len(bodies.get("AB".index(s), []))} for s in "AB"}

    def terminal_after(head, body):
        proof = []
        for d in range(4):
            kind = edges.get((head, d), (0, -1))[0]
            q = adjacent(head, d)
            if not visible(turn["head"], head) or not visible(turn["head"], q):
                return None
            if kind == 1:
                proof.append({"dir": "NESW"[d], "wall": True})
            elif kind == 0 and q in body:
                proof.append({"dir": "NESW"[d], "own_body": q})
            else:
                return None
        return proof

    def alternative_escape(body, d):
        head = body[0]
        if edges.get((head, d), (0, -1))[0] != 0:
            return None
        q = adjacent(head, d)
        if not visible(head, q) or q in board:
            return None
        ate = q in pearls
        after = [q] + body if ate else [q] + body[:-1]
        for e in range(4):
            r = adjacent(q, e)
            if edges.get((q, e), (0, -1))[0] != 0 or not visible(head, r):
                continue
            if r in after or any(owner != turn["id"] for owner in board.get(r, ())):
                continue
            after2 = [r] + after if r in pearls and r != q else [r] + after[:-1]
            if terminal_after(r, after2) is not None:
                continue
            return {"first": "NESW"[d], "cell": q, "pearl": ate,
                    "continuation": "NESW"[e], "continuation_cell": r}
        return None

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
            if round_index in CHECKPOINTS:
                checkpoints[round_index] = {"standings": standing(),
                    "counts": {s: dict(counts[s]) for s in "AB"},
                    "unique_heads": {s: len(unique_heads[s]) for s in "AB"},
                    "unique_view": {s: len(unique_view[s]) for s in "AB"}}
        elif kind == 1:
            acting = data.number(0)
            removed, turn_logs = set(), []
            body = bodies[acting]
            turn = {"id": acting, "round": round_index, "turn_event": index,
                    "head": body[0], "body_before": list(body), "side": teams[acting],
                    "moves": []}
            unique_heads[teams[acting]].add(body[0])
            unique_view[teams[acting]].update({((body[0][0] + dx) % width,
                (body[0][1] + dy) % height) for dx in range(-3, 4) for dy in range(-3, 4)})
        elif kind == 6:
            turn_logs.append(data.text(0))
        elif kind == 3:
            c, present = point(data.child(0)), bool(data.number(0, "B") & 1)
            if present:
                pearls.add(c)
            else:
                pearls.discard(c)
                removed.add(c)
        elif kind == 4:
            dragon, action = data.number(0), data.child(0)
            assert dragon == acting
            action_kind = action.number(0, "H")
            dirs = action.uint16s(0) if action_kind == 0 else []
            side = teams[dragon]
            turn.update({"action_event": index, "kind": action_kind,
                "action": "MOVE " + "".join("NESW"[d] for d in dirs) if action_kind == 0 else
                    f"SPLIT {action.number(4)}" if action_kind == 1 else "SUICIDE",
                "logs": list(turn_logs), "length": len(bodies[dragon]),
                "population": sum(teams[d] == side for d in bodies)})
            counts[side]["actions"] += 1
            counts[side]["splits"] += action_kind == 1
            counts[side]["suicide_requests"] += action_kind == 2
            if action_kind == 0:
                counts[side]["requested_steps"] += len(dirs)
            if side == "B" and dragon != 1 and len(bodies[dragon]) == 2 and len(dirs) == 1:
                d = dirs[0]
                q = adjacent(turn["head"], d)
                enemy_parts = [{"id": owner, "cell": c} for c, owners in board.items()
                               if visible(turn["head"], c) for owner in owners if teams[owner] == "A"]
                if edges.get((turn["head"], d), (0, -1))[0] == 0 and q in pearls and q not in board and not enemy_parts:
                    after = [q] + bodies[dragon]
                    proof = terminal_after(q, after)
                    if proof is not None:
                        alternatives = [a for e in range(4) if e != d
                                        if (a := alternative_escape(bodies[dragon], e))]
                        if alternatives:
                            traps.append({**turn, "pearl": q, "projected_body": after,
                                "terminal_proof": proof, "alternatives": alternatives})
        elif kind == 9:
            dragon = data.number(0)
            old = list(bodies[dragon])
            head, tail = point(data.child(0)), point(data.child(1))
            body = bodies[dragon]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            put_body(dragon, old, body)
            side = teams[dragon]
            counts[side]["successful_steps"] += 1
            if head in removed and dragon == acting:
                counts[side]["confirmed_pickups"] += 1
                counts[side]["pickup_gain"] += max(0, len(body) - len(old))
            removed.clear()
            if dragon == acting:
                turn["moves"].append({"event": index, "body_after": list(body)})
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            old = list(bodies[parent])
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            put_body(parent, old, bodies[parent])
            put_body(child, [], bodies[child])
        elif kind == 11:
            dragon, reason = data.number(0), REASONS[data.number(4, "H")]
            counts[teams[dragon]]["deaths_" + reason] += 1
            deaths[dragon] = {"round": round_index, "event": index, "reason": reason,
                              "body_before": list(bodies[dragon])}
            put_body(dragon, bodies[dragon], [])
            del bodies[dragon]
    final = standing()
    assert final == game["standings"]
    for trap in traps:
        trap["actual_death"] = deaths.get(trap["id"])
    return {"game": game["game"], "map": game["map"], "seed": game["seed"],
            "outcome": game["outcome"], "completed_rounds": game["completed_rounds"],
            "replay": game["replay"], "replay_sha256": game["replay_sha256"],
            "queen_deaths": game["queen_deaths"], "final": final,
            "counts": {s: dict(counts[s]) for s in "AB"}, "checkpoints": checkpoints,
            "terminal_LEN2_pickup_alternatives": traps}


def main():
    output = HERE / "bseat_diagnosis.json"
    assert not output.exists()
    games = []
    for base in BASES:
        label = f"robust_h14_{base}"
        run = RUNS / label
        config = json.loads((run / "run.json").read_text())
        audit = json.loads((run / "verified_audit.json").read_text())
        assert config["sources"][0]["files"]["main.cpp"] == H14_MAIN
        assert config["unswbc"] == "1.2.7" and not config["sandbox"]
        assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
        selected = [g for g in audit["games"] if g["opponent"] == "xxqztest" and g["bot1_side"] == "B"]
        assert len(selected) == 17
        for game in sorted(selected, key=lambda g: int(g["game"])):
            games.append({"base": base, **scan(game)})
            print(f"scanned {len(games)}/51", flush=True)
    payload = {"method": "Existing 51 audited H14/xx B-seat games, three development bases; "
               "replay SHA and final standings reconstructed. Checkpoint r is before round r, "
               "and early-ended games are not padded. Pickups require removal then acting-dragon "
               "successful move to that cell; vision coverage is offline union, not actor memory. "
               "Terminal witness requires all ordinary exits wall/own post-body, no portals, "
               "no currently visible enemy parts, and a visible ordinary alternate with continuation. "
               "No sonar/private bot state reconstructed. Descriptive only; no p-values or causal wins.",
               "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "source_main_sha256": H14_MAIN, "bases": BASES, "games": games}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"games": len(games), "wins": sum(g["outcome"] == "win" for g in games),
        "terminal_traps": sum(len(g["terminal_LEN2_pickup_alternatives"]) for g in games)}), flush=True)


if __name__ == "__main__":
    main()
