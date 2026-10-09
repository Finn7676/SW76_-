#!/usr/bin/env python3
"""Read audited R21/H14 replays; never launch games or modify frozen evidence."""
from __future__ import annotations

from bisect import bisect_right
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS, sha, tree_hashes

R21 = "robust_rear_cascade_82001"
H14 = "robust_h14_82001"
R21_MAIN = "eb364e6e9ccf138a4b3b12d071e356a2ecee6300a4b0209675fe6888382f5573"
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
DIRS = ((0, -1), (1, 0), (0, 1), (-1, 0))
LETTERS = "NESW"


def key(game):
    return game["opponent"], game["map"], int(game["seed"]), game["bot1_side"]


def verified_run(label, main_hash):
    run = RUNS / label
    config_path, audit_path = run / "run.json", run / "verified_audit.json"
    config, audit = json.loads(config_path.read_text()), json.loads(audit_path.read_text())
    assert config["unswbc"] == "1.2.7" and not config["sandbox"]
    assert config["planned"] == len(config["schedule"]) == 68
    summary = audit["summary"]
    assert summary["audit_passed"] and summary["games"] == summary["planned"] == 68
    assert not summary["audit_failures"] and not summary["manifest_failures"] and not summary["failed_matches"]
    assert len(audit["games"]) == 68
    assert config["sources"][0]["files"]["main.cpp"] == main_hash
    for source in config["sources"]:
        assert tree_hashes(Path(source["snapshot"])) == source["files"]
        assert sha(source["prepared"]) == source["prepared_sha256"]
    for item in config["maps"]:
        assert sha(item["snapshot"]) == item["sha256"]
    assert sha(config["cli"]) == config["cli_sha256"]
    assert sha(config["engine_original"]) == config["engine_sha256"]
    assert tree_hashes(run / "frozen/engine") == config["engine_snapshot_files"]
    package = Path(config["engine_original"]).parent
    for name, digest in config["engine_snapshot_files"].items():
        assert sha(package / name) == digest
    schedule = {(g["opponent"], g["map"], int(g["seed"]), g["side"]) for g in config["schedule"]}
    games = {key(g): g for g in audit["games"]}
    assert len(games) == 68 and set(games) == schedule
    return config, games, {"label": label, "run_json": str(config_path),
        "run_json_sha256": sha(config_path), "audit_json": str(audit_path),
        "audit_json_sha256": sha(audit_path), "manifest_rechecked": True,
        "engine_sha256": config["engine_sha256"], "main_sha256": main_hash,
        "binary_sha256": config["sources"][0]["prepared_sha256"],
        "by_opponent": summary["by_opponent"]}


def action(data):
    if data.pointer(0) is None:
        return None, "NO_VALID_ACTION", None
    item = data.child(0)
    kind = item.number(0, "H")
    if kind == 0:
        return kind, "MOVE " + "".join(LETTERS[d] for d in item.uint16s(0)), None
    if kind == 1:
        length = item.number(4)
        return kind, f"SPLIT {length}", length
    assert kind == 2
    return kind, "SUICIDE", None


def canonical(event):
    kind, data = event.number(0, "H"), event.child(0)
    if kind in (0, 1):
        return [kind, data.number(0)]
    if kind in (2, 3):
        return [kind, data.number(0), point(data.child(0))]
    if kind == 4:
        return [kind, data.number(0), action(data)[1]]
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


def update_digest(stream, item):
    stream.update(json.dumps(item, separators=(",", ":")).encode() + b"\n")


def topology(text, width, height):
    edges, ends = {}, defaultdict(list)
    for line in text.splitlines():
        words = line.split("#", 1)[0].split()
        if not words or words[0] != "EDGE":
            continue
        edge_id, kind, pid = map(int, words[1:4])
        row, x = divmod(edge_id, width + 1)
        if row % 2 == 0:
            y = row // 2
            links = [((x % width, y % height), 0), ((x % width, (y - 1) % height), 2)]
        else:
            y = (row - 1) // 2
            links = [((x % width, y), 3), (((x - 1) % width, y), 1)]
        for cell, direction in links:
            edges[cell, direction] = kind, pid
        if kind == 2:
            # Canonical endpoints face E or S, matching the frozen source.
            cell, direction = links[1]
            endpoint = cell, direction
            if endpoint not in ends[pid]:
                ends[pid].append(endpoint)
    return edges, ends


def landing(cell, direction, width, height):
    dx, dy = DIRS[direction]
    return (cell[0] + dx) % width, (cell[1] + dy) % height


def pair_geometry(body, edges, ends, width, height):
    assert len(body) == 2
    head, neck = body
    ordinary, portals = [], []
    for direction in range(4):
        kind, pid = edges.get((neck, direction), (0, -1))
        q = landing(neck, direction, width, height)
        if kind == 0 and q == head:
            ordinary.append(LETTERS[direction])
        if kind != 2:
            continue
        source = (neck, direction) if direction in (1, 2) else (q, direction ^ 2)
        for cell, outgoing in ends[pid]:
            if (cell, outgoing) == source or outgoing not in (direction, direction ^ 2):
                continue
            destination = landing(cell, outgoing, width, height) if outgoing == direction else cell
            if destination == head:
                portals.append({"direction": LETTERS[direction], "portal_id": pid,
                                "paired_endpoint": [cell, LETTERS[outgoing]]})
    classification = "ambiguous_ordinary_and_portal" if ordinary and portals else \
        "ordinary_edge" if ordinary else "portal_link" if portals else "unmatched_topology"
    return {"head": head, "neck": neck,
            "geographic_adjacent": any(landing(neck, d, width, height) == head for d in range(4)),
            "ordinary_links": ordinary, "portal_links": portals, "classification": classification}


def scan(game):
    path = Path(game["replay"])
    assert sha(path) == game["replay_sha256"]
    root = Message(path.read_bytes()).structure(0, 0)
    width, height, bodies, teams = parse_map(root.text(0))
    edges, ends = topology(root.text(0), width, height)
    initial = {d: list(b) for d, b in bodies.items()}
    own = game["bot1_side"]
    counts = {s: Counter() for s in "AB"}
    actions, splits, round_zero, checkpoints = [], [], [], {}
    turn_starts, updates, deaths = defaultdict(list), defaultdict(list), {}
    physical_hash, actions_hash = hashlib.sha256(), hashlib.sha256()
    physical_count, round_index, active, turn_event = 0, -1, None, None
    logs, engine_logs, turn_action = [], [], None
    parentage = {}

    def standing():
        return {s: {"population": sum(teams[d] == s for d in bodies),
                    "total": sum(len(b) for d, b in bodies.items() if teams[d] == s),
                    "longest": max((len(b) for d, b in bodies.items() if teams[d] == s), default=0),
                    "queen": len(bodies.get("AB".index(s), []))} for s in "AB"}

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
            if round_index in (20, 50, 100):
                checkpoints[round_index] = {"standings": standing(),
                    "counts": {s: dict(counts[s]) for s in "AB"}}
        elif kind == 1:
            active, turn_event = data.number(0), index
            logs, engine_logs, turn_action = [], [], None
            assert active in bodies
            turn_starts[active].append({"event": index, "round": round_index, "id": active})
        elif kind == 5:
            engine_logs.append({"event": index, "round": round_index, "active_turn": active,
                                "text": data.text(0)})
        elif kind == 6:
            logs.append({"event": index, "text": data.text(0)})
        elif kind == 4:
            did = data.number(0)
            assert did == active and did in bodies
            action_kind, requested, split_length = action(data)
            record = {"round": round_index, "id": did, "side": teams[did],
                "event": index, "turn_event": turn_event, "kind": action_kind,
                "action": requested, "split_length": split_length,
                "body_before": list(bodies[did]), "length_before": len(bodies[did]),
                "population_before": sum(teams[d] == teams[did] for d in bodies),
                "physical_prefix_sha256": physical_hash.hexdigest(),
                "physical_prefix_events": physical_count, "logs": list(logs)}
            actions.append(record)
            turn_action = record
            update_digest(actions_hash, [round_index, did, requested])
        elif kind == 9:
            did = data.number(0)
            head, tail = point(data.child(0)), point(data.child(1))
            if round_index < 0:
                assert bodies[did][0] == head and bodies[did][-1] == tail
            else:
                body = bodies[did]
                body.insert(0, head)
                while len(body) > 1 and body[-1] != tail:
                    body.pop()
                assert body[-1] == tail
                counts[teams[did]]["successful_steps"] += 1
                update = {"event": index, "round": round_index, "id": did,
                          "active_turn": active, "head": head, "tail": tail,
                          "length_after": len(body)}
                updates[did].append(update)
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            parent_body = [point(p) for p in data.structs(0)]
            child_body = [point(p) for p in data.structs(1)]
            true_prebody = parent_body + list(reversed(child_body))
            assert true_prebody == bodies[parent]
            assert parent == active and turn_action is not None and turn_action["kind"] == 1
            assert turn_action["split_length"] == len(child_body)
            assert turn_action["body_before"] == true_prebody
            side = teams[parent]
            category = "queen" if parent in (0, 1) else "nonqueen"
            counts[side]["births"] += 1
            counts[side][category + "_births"] += 1
            head = true_prebody[0]
            window = {((head[0] + dx) % width, (head[1] + dy) % height)
                      for dx in range(-3, 4) for dy in range(-3, 4)}
            enemy_ids = [d for d, b in bodies.items() if teams[d] != side and set(b) & window]
            split = {"round": round_index, "event": index, "action_event": turn_action["event"],
                "turn_event": turn_event, "parent": parent, "child": child, "side": side,
                "queen_parent": parent in (0, 1), "length_before": len(true_prebody),
                "population_before": turn_action["population_before"], "body_before": true_prebody,
                "parent_body": parent_body, "child_body": child_body,
                "parent_length": len(parent_body), "child_length": len(child_body),
                "whole_body_current_visible": set(true_prebody) <= window,
                "parent_neck_current_visible": parent_body[1] in window if len(parent_body) >= 2 else None,
                "enemy_part_ids_current_visible": enemy_ids, "logs": list(logs)}
            turn_action["actual_split"] = split
            if len(parent_body) == 2:
                split["parent_LEN2_geometry"] = pair_geometry(parent_body, edges, ends, width, height)
            if round_index == 0:
                round_zero.append(split)
            if round_index < 350 and parent not in (0, 1) and len(true_prebody) >= 5:
                splits.append(split)
            bodies[parent], bodies[child] = list(parent_body), list(child_body)
            teams[child] = "AB"[data.number(8, "H")]
            assert teams[child] == side
            parentage[child] = {"parent": parent, "round": round_index, "event": index}
        elif kind == 11:
            did = data.number(0)
            reason = REASONS[data.number(4, "H")]
            assert did in bodies
            category = "queen" if did in (0, 1) else "nonqueen"
            counts[teams[did]]["deaths"] += 1
            counts[teams[did]][category + "_deaths"] += 1
            deaths[did] = {"event": index, "round": round_index, "id": did,
                "active_turn": active, "reason": reason, "body_before": list(bodies[did]),
                "turn_engine_logs": list(engine_logs)}
            del bodies[did]
        item = canonical(event)
        if item is not None:
            update_digest(physical_hash, item)
            physical_count += 1
    final = standing()
    assert final == game["standings"]
    by_id = defaultdict(list)
    for record in actions:
        by_id[record["id"]].append(record)

    def later(items, after):
        cursor = bisect_right([item["event"] for item in items], after)
        return items[cursor] if cursor < len(items) else None

    for split in {s["event"]: s for s in splits + round_zero}.values():
        for category in ("parent", "child"):
            did = split[category]
            next_action = later(by_id[did], split["event"])
            # Keep the attachment small and avoid recursive split references.
            next_action = {k: v for k, v in next_action.items() if k != "actual_split"} if next_action else None
            death = deaths.get(did)
            split[category + "_next_turn"] = later(turn_starts[did], split["event"])
            split[category + "_next_action"] = next_action
            split[category + "_next_update"] = later(updates[did], split["event"])
            split[category + "_death"] = death
            split[category + "_death_before_next_action"] = bool(death and
                (next_action is None or death["event"] < next_action["event"]))
            split[category + "_death_same_round"] = bool(death and death["round"] == split["round"])
            split[category + "_death_before_following_round_end"] = bool(death and death["round"] <= split["round"] + 1)
    return {"initial": initial, "actions": actions, "actions_sha256": actions_hash.hexdigest(),
        "physical_sha256": physical_hash.hexdigest(), "physical_events": physical_count,
        "early_nonqueen_LEN5plus_split_pool": [s for s in splits if s["side"] == own],
        "round_zero_splits": round_zero, "parentage": parentage, "checkpoints": checkpoints,
        "counts": {s: dict(counts[s]) for s in "AB"}, "final": final}


def first_difference(left, right):
    index = next((i for i, (a, b) in enumerate(zip(left, right)) if
                  (a["round"], a["id"], a["action"]) != (b["round"], b["id"], b["action"])), None)
    if index is None and len(left) != len(right):
        index = min(len(left), len(right))
    if index is None:
        return None
    a = left[index] if index < len(left) else None
    b = right[index] if index < len(right) else None
    strict = bool(a and b and a["physical_prefix_sha256"] == b["physical_prefix_sha256"] and
                  a["physical_prefix_events"] == b["physical_prefix_events"] and
                  a["body_before"] == b["body_before"] and
                  (a["round"], a["id"], a["population_before"]) ==
                  (b["round"], b["id"], b["population_before"]))
    allocation = bool(strict and a.get("actual_split") and b.get("actual_split") and
        a["id"] not in (0, 1) and a["round"] < 350 and a["length_before"] >= 5 and
        a["split_length"] == 2 and b["split_length"] == b["length_before"] - 2 and
        b["actual_split"]["parent_length"] == 2)
    return {"action_index": index, "strict_physical_sonar_prefix_equal": strict,
            "verified_first_ordinary_allocation_effect": allocation, "H14": a, "R21": b}


def cascade(source, first):
    if not first or first["round"] != 0:
        return None
    descendants = {first["child"]}
    linked = []
    for split in source["round_zero_splits"]:
        if split["event"] > first["event"] and split["parent"] in descendants:
            linked.append(split)
            descendants.add(split["child"])
    return {"first_split_event": first["event"], "initial_parent": first["parent"],
            "initial_child": first["child"], "initial_child_length": first["child_length"],
            "same_round_newborn_descendant_splits": linked,
            "same_round_descendant_births": len(linked),
            "terminal_child_actual_first_action": (linked[-1] if linked else first)["child_next_action"],
            "terminal_child_actual_first_update": (linked[-1] if linked else first)["child_next_update"]}


def checkpoint_delta(left, right, side, round_index):
    a, b = left["checkpoints"].get(round_index), right["checkpoints"].get(round_index)
    if a is None or b is None:
        return {"available": False, "H14_reached": a is not None, "R21_reached": b is not None}
    fields = ("population", "total", "longest", "queen")
    return {"available": True, "H14": a, "R21": b,
            "own_R21_minus_H14": {f: b["standings"][side][f] - a["standings"][side][f] for f in fields},
            "own_births_R21_minus_H14": b["counts"][side].get("births", 0) - a["counts"][side].get("births", 0)}


def main():
    out_json, out_md = HERE / "r21_firstscreen_mechanism.json", HERE / "r21_firstscreen_mechanism.md"
    assert not out_json.exists() and not out_md.exists()
    candidate_config, candidate, candidate_provenance = verified_run(R21, R21_MAIN)
    control_config, control, control_provenance = verified_run(H14, H14_MAIN)
    assert set(candidate) == set(control)
    assert {m["name"]: m["sha256"] for m in candidate_config["maps"]} == {
        m["name"]: m["sha256"] for m in control_config["maps"]}
    assert candidate_config["engine_sha256"] == control_config["engine_sha256"]
    assert [s["prepared_sha256"] for s in candidate_config["sources"][1:]] == [
        s["prepared_sha256"] for s in control_config["sources"][1:]]
    games = []
    for pair_key in sorted(candidate, key=lambda k: int(candidate[k]["game"])):
        game, baseline = candidate[pair_key], control[pair_key]
        left, right = scan(baseline), scan(game)
        first = first_difference(left["actions"], right["actions"])
        first_a = first["H14"].get("actual_split") if first and first["H14"] else None
        first_b = first["R21"].get("actual_split") if first and first["R21"] else None
        record = {"opponent": game["opponent"], "map": game["map"], "seed": int(game["seed"]),
            "side": game["bot1_side"], "H14_game": baseline["game"], "R21_game": game["game"],
            "H14_replay": baseline["replay"], "R21_replay": game["replay"],
            "H14_replay_sha256": baseline["replay_sha256"], "R21_replay_sha256": game["replay_sha256"],
            "H14_outcome": baseline["outcome"], "R21_outcome": game["outcome"],
            "H14_final": baseline["standings"], "R21_final": game["standings"],
            "H14_completed_rounds": baseline["completed_rounds"], "R21_completed_rounds": game["completed_rounds"],
            "same_actions": left["actions_sha256"] == right["actions_sha256"],
            "same_physical_sonar": left["physical_sha256"] == right["physical_sha256"],
            "H14_actions_sha256": left["actions_sha256"], "R21_actions_sha256": right["actions_sha256"],
            "H14_physical_sha256": left["physical_sha256"], "R21_physical_sha256": right["physical_sha256"],
            "first_action_divergence": first,
            "H14_first_round_zero_cascade": cascade(left, first_a),
            "R21_first_round_zero_cascade": cascade(right, first_b),
            "checkpoints": {r: checkpoint_delta(left, right, game["bot1_side"], r) for r in (20, 50, 100)},
            "H14_early_nonqueen_LEN5plus_split_pool": left["early_nonqueen_LEN5plus_split_pool"],
            "R21_early_nonqueen_LEN5plus_split_pool": right["early_nonqueen_LEN5plus_split_pool"]}
        games.append(record)
        print(f"paired mechanism {len(games)}/68 first-effect={bool(first and first['verified_first_ordinary_allocation_effect'])}", flush=True)
    effects = [g for g in games if g["first_action_divergence"] and
               g["first_action_divergence"]["verified_first_ordinary_allocation_effect"]]
    metrics = {"games": len(games), "identical_action_games": sum(g["same_actions"] for g in games),
        "identical_physical_sonar_games": sum(g["same_physical_sonar"] for g in games),
        "first_action_divergence_games": sum(g["first_action_divergence"] is not None for g in games),
        "strict_prefix_first_ordinary_allocation_effect_games": len(effects),
        "other_first_action_divergence_games": sum(g["first_action_divergence"] is not None for g in games) - len(effects),
        "round_zero_verified_first_allocation_games": sum(g["first_action_divergence"]["R21"]["round"] == 0 for g in effects),
        "round_zero_verified_first_with_newborn_cascade_games": sum(bool(g["R21_first_round_zero_cascade"] and
            g["R21_first_round_zero_cascade"]["same_round_descendant_births"]) for g in effects)}
    pool_metrics, geometry_metrics = {}, {}
    for label in ("H14", "R21"):
        pool = [s for g in games for s in g[label + "_early_nonqueen_LEN5plus_split_pool"]]
        pool_metrics[label] = {"early_LEN5plus_nonqueen_actual_splits": len(pool),
            "child2": sum(s["child_length"] == 2 for s in pool),
            "child_LENminus2_parent2": sum(s["parent_length"] == 2 for s in pool),
            "not_an_ordinary_branch_activation_count": True}
        geometry = [s for s in pool if s["parent_length"] == 2]
        geometry_metrics[label] = {"parent_LEN2_allocations": len(geometry),
            "geometry": dict(Counter(s["parent_LEN2_geometry"]["classification"] for s in geometry)),
            "portal_link_witnesses": [dict(opponent=g["opponent"], map=g["map"], seed=g["seed"], side=g["side"], split=s)
                for g in games for s in g[label + "_early_nonqueen_LEN5plus_split_pool"]
                if s["parent_length"] == 2 and s["parent_LEN2_geometry"]["portal_links"]],
            "next_round_or_earlier_death_reasons": dict(Counter(s["parent_death"]["reason"] for s in geometry
                if s["parent_death_before_following_round_end"]))}
    promotions = [g for g in games if g["R21_outcome"] == "win" and g["H14_outcome"] != "win"]
    regressions = [g for g in games if g["H14_outcome"] == "win" and g["R21_outcome"] != "win"]
    limitations = [
        "One development screen only; no held-out seed or generalization claim.",
        "True exposure is the strict-prefix first actual allocation effect in each matched game; this is a lower bound, not all ordinary branch activations.",
        "Later raw LEN-2 allocations include other branches; no ordinary activation count is inferred from them.",
        "No committed trace difference does not establish no rejected/proposed branch activation.",
        "Post-divergence numerical child IDs are not treated as paired causal identities.",
        "Full bodies are offline diagnosis; radius3 current visibility is not reconstructed private trail, sonar memory, or source input.",
        "Portal-link geometry and actual death timing/reason are separate evidence; invalid_action is never relabeled as portal death.",
        "Round20/50/100 checkpoints are before that round; early-ended games are not padded.",
        "Round-zero descendant splits establish actual allocation timing, not a causal win explanation."]
    payload = {"passed": True, "scope": "Native1.2.7 base82001 ranked17 both seats two unchanged opponents",
        "script_sha256": sha(__file__), "packed_parser_sha256": sha(RUNS.parent / "protect_local_20260927/analyze_replays.py"),
        "provenance": [control_provenance, candidate_provenance], "metrics": metrics,
        "raw_pool_metrics": pool_metrics, "parent_LEN2_geometry_metrics": geometry_metrics,
        "promotions": [{f: g[f] for f in ("opponent", "map", "seed", "side", "H14_outcome", "R21_outcome")} for g in promotions],
        "regressions": [{f: g[f] for f in ("opponent", "map", "seed", "side", "H14_outcome", "R21_outcome")} for g in regressions],
        "limitations": limitations, "games": games}
    lines = ["# R21 First-Screen Mechanism", "", "All68 R21 and matched H14 replays have complete passed official outcome audits. This read-only analysis rechecks frozen source/binary/map/engine manifests, each replay SHA256, actual split arrays, and reconstructed final standings. No game was launched and no frozen source was modified.", "", "| Mechanism Evidence | Games |", "| --- | ---: |"]
    lines.extend(f"| {field} | {value} |" for field, value in metrics.items())
    lines += ["", "The strict-prefix first actual rear allocation is an observed ordinary-factor effect. It is not a complete activation count: the source emits no ordinary-branch telemetry, and later reverse allocations can also come from emergency logic.", "", "| Raw Early Nonqueen LEN>=5 Split Pool | H14 | R21 |", "| --- | ---: | ---: |"]
    for field in ("early_LEN5plus_nonqueen_actual_splits", "child2", "child_LENminus2_parent2"):
        lines.append(f"| {field} | {pool_metrics['H14'][field]} | {pool_metrics['R21'][field]} |")
    lines += ["", "## Round20", "", "| Opponent / Seat | Paired Games Reaching20 | Mean Population Delta | Mean Total Delta | Mean Birth Delta |", "| --- | ---: | ---: | ---: | ---: |"]
    for opponent in ("V3_portal", "xxqztest"):
        for side in "AB":
            selected = [g["checkpoints"][20] for g in games if g["opponent"] == opponent and g["side"] == side and g["checkpoints"][20]["available"]]
            n = len(selected)
            means = [sum(s["own_R21_minus_H14"][f] for s in selected) / n if n else 0 for f in ("population", "total")]
            births = sum(s["own_births_R21_minus_H14"] for s in selected) / n if n else 0
            lines.append(f"| {opponent} / {side} | {n} | {means[0]:+.2f} | {means[1]:+.2f} | {births:+.2f} |")
    lines += ["", "## Actual Parent-LEN2 Geometry", ""]
    for label in ("H14", "R21"):
        m = geometry_metrics[label]
        lines.append(f"{label}: {m['parent_LEN2_allocations']} observed early pool parent-LEN2 allocations; geometry {m['geometry']}; actual same/next-round death reasons {m['next_round_or_earlier_death_reasons']}. Portal-link witnesses {len(m['portal_link_witnesses'])}. Geometry is not a death-cause inference; full event details and next-turn/action/update/death timing are in JSON.")
        lines.append("")
    lines += ["## Paired Examples", ""]
    for heading, selected in (("Promotions", promotions), ("Regressions", regressions)):
        lines.append(f"{heading}: {len(selected)} observed paired win-status changes.")
        lines.append("")
        for g in selected[:4]:
            f = g["first_action_divergence"]
            detail = f"round{f['R21']['round']} id{f['R21']['id']} {f['H14']['action']} -> {f['R21']['action']} strict-prefix={f['strict_physical_sonar_prefix_equal']}" if f and f["H14"] and f["R21"] else "no paired first action"
            cp = g["checkpoints"][20]
            delta = cp.get("own_R21_minus_H14", {})
            lines.append(f"- {g['opponent']} / {g['map']} / seed{g['seed']} / {g['side']}: {g['H14_outcome']} -> {g['R21_outcome']}; {detail}; round20 own deltas {delta}.")
        lines.append("")
    lines += ["## Limits", ""] + ["- " + text for text in limitations]
    lines += ["", "This report describes observed mechanisms and paired outcomes only. The independent experiment decision owns expansion or production promotion. Complete event witnesses, hashes, and all matched tuples are in `r21_firstscreen_mechanism.json`."]
    with out_json.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    with out_md.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"passed": True, "metrics": metrics, "pool_metrics": pool_metrics,
                      "json": str(out_json), "md": str(out_md)}), flush=True)


if __name__ == "__main__":
    main()
