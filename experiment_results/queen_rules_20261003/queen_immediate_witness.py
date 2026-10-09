#!/usr/bin/env python3
"""Bounded legal-attack evidence from audited H14/xx development replays only."""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import time

from analyze import Message, parse_map, point

HERE = Path(__file__).resolve().parent
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
DIRS = ((0, -1), (1, 0), (0, 1), (-1, 0))
LETTERS = "NESW"


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def topology(text):
    width, height, _, _ = parse_map(text)
    edges = {}
    for line in text.splitlines():
        words = line.split()
        if not words or words[0] != "EDGE":
            continue
        key, kind, pid = map(int, words[1:4])
        boundary, x = divmod(key, width + 1)
        if boundary % 2 == 0:
            y = boundary // 2
            edges[((x % width, y % height), 0)] = (kind, pid)
            edges[((x % width, (y - 1) % height), 2)] = (kind, pid)
        else:
            y = (boundary - 1) // 2
            edges[((x % width, y), 3)] = (kind, pid)
            edges[(((x - 1) % width, y), 1)] = (kind, pid)
    return width, height, edges


def landing(cell, direction, width, height):
    dx, dy = DIRS[direction]
    return ((cell[0] + dx) % width, (cell[1] + dy) % height)


def visible_cells(head, width, height):
    return {((head[0] + dx) % width, (head[1] + dy) % height)
            for dx in range(-3, 4) for dy in range(-3, 4)}


def ordinary_complete(body, visible, edges, width, height):
    if len(set(body)) != len(body) or not set(body) <= visible:
        return False
    for front, rear in zip(body, body[1:]):
        if not any(landing(rear, d, width, height) == front and
                   edges.get((rear, d), (0, -1))[0] == 0 for d in range(4)):
            return False
    # Every possible rear continuation is visible, so the shown tail cannot
    # be a truncated prefix of an unseen ordinary-edge body.
    return all(edges.get((body[-1], d), (0, -1))[0] == 1 or
               (edges.get((body[-1], d), (0, -1))[0] == 0 and
                landing(body[-1], d, width, height) in visible) for d in range(4))


def project(body, directions, occupancy, pearls, visible, edges, width, height,
            target=None):
    initial_free = (len(body) + 3) // 4
    current, eaten = list(body), set()
    trace = []
    for index, direction in enumerate(directions):
        paid = index >= initial_free
        if paid and len(current) <= 2:
            return {"known": True, "survives": False, "reason": "payment", "trace": trace}
        edge = edges.get((current[0], direction), (0, -1))[0]
        if current[0] not in visible or edge not in (0, 1):
            return {"known": False, "survives": False, "reason": "hidden_or_portal", "trace": trace}
        if edge == 1:
            return {"known": True, "survives": False, "reason": "wall", "trace": trace}
        q = landing(current[0], direction, width, height)
        if q not in visible:
            return {"known": False, "survives": False, "reason": "hidden", "trace": trace}
        if q in current:
            return {"known": True, "survives": False, "reason": "self", "trace": trace}
        if q in occupancy:
            return {"known": True, "survives": False,
                    "reason": "hit_target" if q == target else "body", "trace": trace,
                    "hit_step": index + 1}
        eats = q in pearls and q not in eaten
        current.insert(0, q)
        if eats:
            eaten.add(q)
        else:
            current.pop()
        if paid:
            current.pop()
        trace.append({"step": index + 1, "direction": LETTERS[direction],
                      "paid": paid, "eats": eats, "head": q, "length": len(current)})
    return {"known": True, "survives": True, "body": current, "eaten": sorted(eaten),
            "initial_free_steps": initial_free, "trace": trace}


def continuation(body, occupancy, visible, edges, width, height):
    for d in range(4):
        if edges.get((body[0], d), (0, -1))[0] != 0:
            continue
        q = landing(body[0], d, width, height)
        if q in visible and q not in body and q not in occupancy:
            return LETTERS[d]
    return None


def enemy_witness(enemy_body, target, occupancy, pearls, visible, edges, width, height):
    nodes = 0
    unknown = False
    stack = [()]
    while stack:
        path = stack.pop()
        if len(path) >= 4:
            continue
        for d in range(4):
            trial = path + (d,)
            nodes += 1
            result = project(enemy_body, trial, occupancy, pearls, visible, edges, width, height,
                             target=target)
            if result.get("reason") == "hit_target":
                return {"confirmed": True, "directions": "".join(LETTERS[x] for x in trial),
                        "projection": result, "nodes": nodes,
                        "initial_length": len(enemy_body), "initial_free_steps": (len(enemy_body) + 3) // 4}
            if not result["known"]:
                unknown = True
            elif result["survives"]:
                stack.append(trial)
    return {"confirmed": False, "unknown": unknown,
            "beyond_four_steps_unknown": True, "nodes": nodes}


def analyze_snapshot(snapshot, width, height, edges):
    queen = snapshot["queen"]
    bodies = snapshot["bodies"]
    own = bodies[queen]
    visible = visible_cells(own[0], width, height)
    if not set(own) <= visible:
        return {"eligible": False, "reason": "own_body_partial"}
    occupancy = {cell: did for did, body in bodies.items() for cell in body}
    other = {cell for cell, did in occupancy.items() if did != queen and cell in visible}
    pearls = snapshot["pearls"] & visible
    actual = snapshot["directions"]
    if not actual:
        return {"eligible": False, "reason": "original_nonmovement"}
    original = project(own, actual, other, pearls, visible, edges, width, height)
    if not original["known"] or not original["survives"]:
        return {"eligible": False, "reason": "original_not_known_living"}
    attack_id = snapshot["attack_id"]
    if attack_id not in bodies or not ordinary_complete(bodies[attack_id], visible, edges, width, height):
        return {"eligible": False, "reason": "attacker_body_partial_or_portal"}
    # Restrict evidence to an enemy acting immediately after this queen, with
    # no intervening turn or environmental update beyond the queen's action.
    if snapshot["next_turn"] != attack_id:
        return {"eligible": False, "reason": "intervening_turn"}
    candidates = []
    stack = [()]
    while stack:
        path = stack.pop()
        if len(path) >= 4:
            continue
        for d in range(4):
            trial = path + (d,)
            post = project(own, trial, other, pearls, visible, edges, width, height)
            if not post["known"] or not post["survives"]:
                continue
            stack.append(trial)
            onward = continuation(post["body"], other, visible, edges, width, height)
            if not onward:
                continue
            remaining = pearls - set(map(tuple, post["eaten"]))
            projected_occupancy = (other - set(bodies[attack_id])) | set(post["body"])
            witness = enemy_witness(bodies[attack_id], post["body"][0], projected_occupancy,
                                    remaining, visible, edges, width, height)
            candidates.append({"directions": "".join(LETTERS[x] for x in trial),
                               "projection": post, "continuation": onward, "attacker_witness": witness})
    original_target = original["body"][0]
    baseline_occupancy = (other - set(bodies[attack_id])) | set(original["body"])
    baseline = enemy_witness(bodies[attack_id], original_target, baseline_occupancy,
                             pearls - set(map(tuple, original["eaten"])), visible, edges, width, height)
    alternatives = [c for c in candidates if not c["attacker_witness"]["confirmed"]]
    alternatives.sort(key=lambda c: (-len(c["projection"]["body"]), len(c["directions"]), c["directions"]))
    return {"eligible": True, "own_body": own, "visible_cells": sorted(visible),
            "visible_parts": [{"id": did, "head": body[0], "shown_body": [c for c in body if c in visible],
                               "full_body_visible": set(body) <= visible}
                              for did, body in sorted(bodies.items()) if set(body) & visible],
            "visible_pearls": sorted(pearls), "attacker_body": bodies[attack_id],
            "original_projection": original, "baseline_witness": baseline,
            "candidate_count": len(candidates), "alternatives": alternatives[:3],
            "positive": baseline["confirmed"] and bool(alternatives),
            "search_nodes": baseline["nodes"] + sum(c["attacker_witness"]["nodes"] for c in candidates)}


def complete_visible_enemies(snapshot, width, height, edges):
    queen, bodies, teams = snapshot["queen"], snapshot["bodies"], snapshot["teams"]
    visible = visible_cells(bodies[queen][0], width, height)
    return [did for did, body in sorted(bodies.items())
            if teams[did] != teams[queen] and body[0] in visible and
            ordinary_complete(body, visible, edges, width, height)]


def all_visible_witnesses(snapshot, queen_post, width, height, edges):
    """Static post-queen alternatives, not a later-turn-order safety proof."""
    queen, bodies = snapshot["queen"], snapshot["bodies"]
    visible = visible_cells(bodies[queen][0], width, height)
    other = {cell for did, body in bodies.items() if did != queen for cell in body if cell in visible}
    pearls = snapshot["pearls"] & visible
    remaining = pearls - set(map(tuple, queen_post["eaten"]))
    return [{"id": did, **enemy_witness(bodies[did], queen_post["body"][0],
                (other - set(bodies[did])) | set(queen_post["body"]),
                remaining, visible, edges, width, height)}
            for did in complete_visible_enemies(snapshot, width, height, edges)]


def scan(game):
    path = Path(game["replay"])
    if digest(path) != game["replay_sha256"]:
        raise ValueError(f"Replay changed: {path}")
    root = Message(path.read_bytes()).structure(0, 0)
    width, height, bodies, teams = parse_map(root.text(0))
    _, _, edges = topology(root.text(0))
    queen = 0 if game["bot1_side"] == "A" else 1
    pearls, action_snapshots = set(), {}
    round_index, turn, last_turn = -1, -1, -1
    queen_snapshot = None
    findings = []
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            last_turn, turn = turn, data.number(0)
            if queen_snapshot is not None and "next_turn" not in queen_snapshot:
                queen_snapshot["next_turn"] = turn
        elif kind == 3:
            cell = point(data.child(0))
            if data.number(0, "B") & 1:
                pearls.add(cell)
            else:
                pearls.discard(cell)
        elif kind == 4:
            did = data.number(0)
            action = data.child(0)
            detail = action.uint16s(0) if action.number(0, "H") == 0 else []
            action_snapshots[did] = {"event": index, "round": round_index, "directions": detail}
            if did == queen:
                queen_snapshot = {**action_snapshots[did], "queen": queen,
                    "bodies": {i: list(b) for i, b in bodies.items()}, "teams": dict(teams),
                    "pearls": set(pearls)}
        elif kind == 9:
            did = data.number(0)
            head, tail = point(data.child(0)), point(data.child(1))
            body = bodies[did]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            if body[-1] != tail:
                raise ValueError(f"Tail mismatch: {path} id {did}")
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            did, reason = data.number(0), data.number(4, "H")
            if did == queen and reason == 3:
                meta = {"replay": str(path), "replay_sha256": game["replay_sha256"],
                        "map": game["map"], "seed": game["seed"], "side": game["bot1_side"],
                        "death_event": index, "death_round": round_index, "attack_id": turn,
                        "attack_action": action_snapshots.get(turn, {}).get("directions"),
                        "queen_action_event": queen_snapshot["event"] if queen_snapshot else None,
                        "queen_action_round": queen_snapshot["round"] if queen_snapshot else None,
                        "queen_action": queen_snapshot["directions"] if queen_snapshot else None}
                if queen_snapshot is None or turn == queen or teams.get(turn) == teams.get(queen):
                    result = {"eligible": False, "reason": "own_action_or_friendly_hit"}
                else:
                    queen_snapshot["attack_id"] = turn
                    result = analyze_snapshot(queen_snapshot, width, height, edges)
                    if result.get("eligible"):
                        result["original_all_visible_complete_witnesses"] = all_visible_witnesses(
                            queen_snapshot, result["original_projection"], width, height, edges)
                        for candidate in result["alternatives"]:
                            candidate["all_visible_complete_witnesses"] = all_visible_witnesses(
                                queen_snapshot, candidate["projection"], width, height, edges)
                findings.append({**meta, **result})
            bodies.pop(did, None)
    return findings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-prefix", type=Path, required=True)
    args = parser.parse_args()
    output = args.output_prefix.with_suffix(".json")
    if output.exists():
        parser.error("Output exists; preserve prior evidence")
    start = time.perf_counter()
    findings, audit_records = [], []
    for base in (82001, 83001, 84001):
        suite = HERE / f"robust_h14_{base}"
        audit_path = suite / "verified_audit.json"
        audit = json.loads(audit_path.read_text())
        config = json.loads((suite / "run.json").read_text())
        if not audit["summary"]["audit_passed"] or config["sources"][0]["files"]["main.cpp"] != H14_MAIN:
            raise ValueError(f"Suite is not audited frozen H14: {suite}")
        games = [g for g in audit["games"] if g["opponent"] == "xxqztest"]
        if len(games) != 34:
            raise ValueError("Expected 34 H14/xx development games")
        audit_records.append({"base": base, "audit_sha256": digest(audit_path), "games": len(games)})
        for game in games:
            if game["queen_deaths"][game["bot1_side"]] and game["queen_deaths"][game["bot1_side"]]["reason"] == "head":
                findings.extend(scan(game))
        print(f"scanned {base}: {len(findings)} accumulated queen head deaths", flush=True)
    payload = {"scope": "102 audited H14/xx development games only, bases82001/83001/84001",
               "h14_sha256": H14_MAIN, "script_sha256": digest(__file__), "audits": audit_records,
               "queen_head_deaths": len(findings), "eligibility": dict(Counter(f.get("reason", "eligible") for f in findings)),
               "positive_count": sum(bool(f.get("positive")) for f in findings),
               "elapsed_seconds": time.perf_counter() - start, "findings": findings,
               "limitation": "No attack witness within four is a bounded absence, never global enemy safety. "
                             "Partial body, hidden geometry, portals and attacks beyond four remain unknown."}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({k: payload[k] for k in ("queen_head_deaths", "eligibility", "positive_count", "elapsed_seconds")}))


if __name__ == "__main__":
    main()
