#!/usr/bin/env python3
"""Compare only the audited H14/R12 82001 actions and pearl collections."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS

LABELS = ("robust_h14_82001", "robust_pearl_reserve_82001")
EXPECTED = (
    "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc",
    "2a49f95382df699038ecb267c8694b7beff69febb5d0fae4570766e0beedeb82",
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def key(game):
    return game["opponent"], game["map"], game["seed"], game["bot1_side"]


def scan(game):
    packed = Path(game["replay"]).read_bytes()
    assert hashlib.sha256(packed).hexdigest() == game["replay_sha256"]
    root = Message(packed).structure(0, 0)
    width, height, dragons, teams = parse_map(root.text(0))
    occupied = {cell: did for did, body in dragons.items() for cell in body}
    pearls = set()
    side, round_index, actor = game["bot1_side"], -1, None
    counts, gates, logs = Counter(), Counter(), Counter()
    first_round, last_visible_enemy = {}, {}
    actions, collections, world = [], [], []
    pending_removals, turn_logs, gate_examples = [], [], []

    def vision(head):
        return {((head[0] + x) % width, (head[1] + y) % height)
                for x in range(-3, 4) for y in range(-3, 4)}

    def remove_body(did):
        for cell in dragons[did]:
            if occupied.get(cell) == did:
                occupied.pop(cell)

    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            actor, pending_removals, turn_logs = data.number(0), [], []
        elif kind == 3:
            cell, present = point(data.child(0)), bool(data.number(0, "B") & 1)
            world.append((round_index, "pearl", cell, present))
            if present:
                pearls.add(cell)
            else:
                assert cell in pearls
                pearls.remove(cell)
                pending_removals.append(cell)
        elif kind == 4:
            did, action = data.number(0), data.child(0)
            assert actor == did
            action_kind = action.number(0, "H")
            detail = tuple(action.uint16s(0)) if action_kind == 0 else action.number(4) if action_kind == 1 else None
            signature = (round_index, did, teams[did], action_kind, detail,
                         len(dragons[did]), dragons[did][0])
            actions.append(signature)
            counts["all_actions"] += 1
            if teams[did] != side:
                continue
            counts["own_actions"] += 1
            counts[f"own_kind_{action_kind}"] += 1
            visible = vision(dragons[did][0])
            first_round.setdefault(did, round_index)
            last_visible_enemy.setdefault(did, round_index)
            enemy_visible = any(teams[occupied[cell]] != side for cell in visible if cell in occupied)
            if enemy_visible:
                last_visible_enemy[did] = round_index
            if round_index < 350:
                continue
            gates["late_own_actions"] += 1
            own_population = sum(teams[d] == side for d in dragons)
            length = len(dragons[did])
            if did in (0, 1) or not 2 <= length <= 6 or own_population < 4:
                continue
            gates["short_nonqueen_population_gate"] += 1
            if round_index - first_round[did] < 8:
                continue
            gates["age_at_least_eight_rounds"] += 1
            if enemy_visible:
                continue
            gates["no_current_visible_enemy"] += 1
            if round_index - last_visible_enemy[did] < 8:
                continue
            # This is a necessary-condition upper bound: sonar sightings can
            # extend the bot's last-enemy time beyond visible sightings.
            gates["eight_round_visible_quiet_upper_bound"] += 1
            visible_pearls = sorted(c for c in pearls & visible if c not in occupied)
            receivers = [(other, len(body), body[0]) for other, body in dragons.items()
                         if other != did and teams[other] == side and body[0] in visible
                         and (other in (0, 1) or len(body) >= max(8, length + 2))]
            if visible_pearls:
                gates["quiet_upper_bound_with_visible_pearl"] += 1
            if receivers:
                gates["quiet_upper_bound_with_actual_size_receiver"] += 1
            if not visible_pearls or not receivers:
                continue
            gates["quiet_upper_bound_with_pearl_and_receiver"] += 1
            preempted = any(t in ("QUEEN_FEED", "CONSOLIDATE_FEED") for t in turn_logs)
            gates["co_presence_feed_preempted" if preempted else "co_presence_not_feed_preempted"] += 1
            if len(gate_examples) < 24:
                gate_examples.append({"round_index": round_index, "actor": did,
                    "length": length, "population": own_population, "action": signature,
                    "visible_pearls": visible_pearls, "receivers_actual_lengths": receivers,
                    "feed_preempted": preempted, "logs": list(turn_logs)})
        elif kind == 6:
            did, text = data.number(0), data.text(0)
            if teams.get(did) == side:
                words = text.split()
                for tag in ("QUEEN_FEED", "CONSOLIDATE_FEED", "PROTECT", "PROTECT_KEEP", "PROTECT_DIRECT"):
                    if tag in words:
                        logs[tag] += 1
                        if did == actor:
                            turn_logs.append(tag)
        elif kind == 9:
            did, head, tail = data.number(0), point(data.child(0)), point(data.child(1))
            body = dragons[did]
            before_length = len(body)
            remove_body(did)
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            occupied.update({cell: did for cell in body})
            world.append((round_index, "move", did, head, tail))
            for cell in pending_removals:
                if did == actor and cell == head:
                    collections.append((round_index, did, teams[did], cell, before_length, len(body)))
                    counts["all_pearl_collections"] += 1
                    if teams[did] == side:
                        counts["own_pearl_collections"] += 1
                        if round_index >= 350:
                            counts["own_late_pearl_collections"] += 1
                            if did not in (0, 1) and before_length <= 6:
                                counts["own_late_short_pearl_collections"] += 1
            pending_removals = []
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            remove_body(parent)
            dragons[parent] = [point(p) for p in data.structs(0)]
            dragons[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            occupied.update({cell: did for did in (parent, child) for cell in dragons[did]})
            world.append((round_index, "split", parent, child, tuple(dragons[parent]), tuple(dragons[child])))
        elif kind == 11:
            did, reason = data.number(0), REASONS[data.number(4, "H")]
            world.append((round_index, "death", did, reason, tuple(dragons[did])))
            remove_body(did)
            dragons.pop(did)
        elif kind > 12:
            raise ValueError(f"Unknown replay event {kind}")
    return {"counts": dict(counts), "gate_upper_bound_counts": dict(gates),
            "logs": dict(logs), "gate_examples": gate_examples,
            "actions": actions, "collections": collections, "world": world}


def stream_hash(items):
    return hashlib.sha256(json.dumps(items, separators=(",", ":")).encode()).hexdigest()


def first_difference(left, right):
    for index, (a, b) in enumerate(zip(left, right)):
        if a != b:
            return {"index": index, "baseline": a, "candidate": b}
    if len(left) != len(right):
        index = min(len(left), len(right))
        return {"index": index, "baseline": left[index] if index < len(left) else None,
                "candidate": right[index] if index < len(right) else None}
    return None


def load(label, expected):
    directory = RUNS / label
    audit = json.loads((directory / "verified_audit.json").read_text())
    config = json.loads((directory / "run.json").read_text())
    assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
    assert config["base_seed"] == 82001 and config["unswbc"] == "1.2.7" and not config["sandbox"]
    assert config["sources"][0]["files"]["main.cpp"] == expected
    for source in config["sources"]:
        for relative, digest in source["files"].items():
            assert sha(Path(source["snapshot"]) / relative) == digest
    for item in config["maps"]:
        assert sha(item["snapshot"]) == item["sha256"]
    for relative, digest in config["engine_snapshot_files"].items():
        assert sha(directory / "frozen/engine" / relative) == digest
    common = {"engine": config["engine_sha256"], "engine_modules": config["engine_snapshot_files"],
              "schedule": config["schedule"],
              "maps": [(m["name"], m["sha256"], m["seed"]) for m in config["maps"]],
              "opponents": [source["files"] for source in config["sources"][1:]]}
    return audit, config, common


def main():
    output = HERE / "r12_h14_82001_action_compare.json"
    markdown = output.with_suffix(".md")
    assert not output.exists() and not markdown.exists()
    baseline, base_config, base_common = load(LABELS[0], EXPECTED[0])
    candidate, cand_config, cand_common = load(LABELS[1], EXPECTED[1])
    assert base_common == cand_common
    a = {key(g): g for g in baseline["games"]}
    b = {key(g): g for g in candidate["games"]}
    assert a.keys() == b.keys()
    totals = {label: Counter() for label in LABELS}
    gates = {label: Counter() for label in LABELS}
    games, summary = [], Counter()
    by_opponent = {opponent: Counter() for opponent in ("V3_portal", "xxqztest")}
    for index, k in enumerate(sorted(a), 1):
        x, y = scan(a[k]), scan(b[k])
        row = {"opponent": k[0], "map": k[1], "seed": k[2], "side": k[3],
               "baseline_outcome": a[k]["outcome"], "candidate_outcome": b[k]["outcome"],
               "same_standings": a[k]["standings"] == b[k]["standings"],
               "same_deaths": a[k]["deaths"] == b[k]["deaths"],
               "same_completed_rounds": a[k]["completed_rounds"] == b[k]["completed_rounds"]}
        for category in ("actions", "collections", "world"):
            row["same_" + category] = x[category] == y[category]
            row["first_" + category + "_difference"] = first_difference(x[category], y[category])
        for label, game, scanned in ((LABELS[0], a[k], x), (LABELS[1], b[k], y)):
            totals[label].update(scanned["counts"])
            gates[label].update(scanned["gate_upper_bound_counts"])
            row[label] = {"replay": game["replay"], "replay_sha256": game["replay_sha256"],
                          "counts": scanned["counts"], "gate_upper_bound_counts": scanned["gate_upper_bound_counts"],
                          "gate_examples": scanned["gate_examples"], "logs": scanned["logs"],
                          "stream_sha256": {c: stream_hash(scanned[c]) for c in ("actions", "collections", "world")}}
        summary["games"] += 1
        by_opponent[k[0]]["games"] += 1
        for category in ("actions", "collections", "world", "standings", "deaths", "completed_rounds"):
            summary["same_" + category] += row["same_" + category]
            by_opponent[k[0]]["same_" + category] += row["same_" + category]
        games.append(row)
        if index % 8 == 0:
            print(f"paired scan {index}/68", flush=True)
    payload = {"scope": "Only robust_h14_82001 and robust_pearl_reserve_82001 completed development suites; no reserved seeds read",
        "labels": LABELS, "source_main_sha256": dict(zip(LABELS, EXPECTED)),
        "audit_summary": {LABELS[0]: baseline["summary"], LABELS[1]: candidate["summary"]},
        "script_sha256": sha(__file__), "summary": dict(summary),
        "by_opponent": {k: dict(v) for k, v in by_opponent.items()},
        "counts": {k: dict(v) for k, v in totals.items()},
        "gate_upper_bound_counts": {k: dict(v) for k, v in gates.items()},
        "method": "Actions compare complete official move/split/suicide payloads plus round, actor, side, initial length and head. Collections require official pearl removal and same-turn movement to that cell. World compares pearl changes, moves, splits and deaths, excluding logs/sonar/header paths. Gate counts are necessary-condition upper bounds, not actual instrumentation: wrapped 7x7 current vision and visible-enemy history are reconstructed, sonar-enemy sightings are ignored, receiver full actual length replaces the bot's potentially lower visible/reported length, and topology/escape/path/payment checks are not imposed. Feed preemption is directly logged. Exact internal reservation activation cannot be recovered because R12 emits no reservation log.",
        "games": games}
    output.write_text(json.dumps(payload, indent=2) + "\n")
    lines = ["# R12/H14 82001 Action And Pearl Comparison", "", payload["scope"], "",
             "Both suites passed the stored audit; current frozen source, map and engine hashes and all 136 replay hashes were verified.", "",
             "| Opponent | Paired Games | Same Actions | Same Pearl Collections | Same World Changes | Same Final Standings |",
             "|---|---:|---:|---:|---:|---:|"]
    for opponent, item in payload["by_opponent"].items():
        lines.append(f'| {opponent} | {item["games"]} | {item["same_actions"]} | {item["same_collections"]} | {item["same_world"]} | {item["same_standings"]} |')
    lines += ["", "## Counts", "", "```json", json.dumps({"counts": payload["counts"], "gate_upper_bound_counts": payload["gate_upper_bound_counts"]}, indent=2), "```", "", "## Limits", "", payload["method"]]
    differences = [g for g in games if not g["same_actions"] or not g["same_collections"]]
    lines += ["", "## First Differences", "", json.dumps(differences, indent=2) if differences else "No action or pearl-collection divergence in any of the 68 paired games."]
    markdown.write_text("\n".join(lines) + "\n")
    print(json.dumps({"output": str(output), "summary": payload["summary"], "counts": payload["counts"], "gate_upper_bound_counts": payload["gate_upper_bound_counts"]}), flush=True)


if __name__ == "__main__":
    main()
