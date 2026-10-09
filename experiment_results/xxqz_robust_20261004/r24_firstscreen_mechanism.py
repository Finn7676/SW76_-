#!/usr/bin/env python3
"""Read-only paired R24/H14 room evidence; require complete audits first."""
from __future__ import annotations

from collections import Counter
from itertools import zip_longest
import hashlib
import json
from pathlib import Path

import r21_firstscreen_mechanism as base

HERE = Path(__file__).resolve().parent
R24 = "robust_room_bound_82001"
H14 = "robust_h14_82001"
R24_MAIN = "cea6542e3c5d8b462730456afa0289503f06e7d507098a5a39b38af9cf005857"
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
SCANNER_SHA = "b0c28698c4752c0991b9463a239d793f028b6080913f4abf3cca4e11399112dd"
GEOMETRY_SHA = "8ff95bcf38dc274b8e9d39da6c055c9ee2a6b2c0b76edfa61bfb079b94d1e6fa"
def decode_claim(value, side, width, height):
    if value >> 32 != (0x51505230 | (side == "B")):
        return None
    raw, low = value & 0xFFFFFFFF, value & 0x0FFFFFFF
    tag = (((low ^ 0x5A17C3E1) * 0x9E3779B1) & 0xFFFFFFFF) >> 28
    if raw >> 28 != tag or (low >> 25) & 7 != 3:
        return None
    x, y = low & 63, (low >> 6) & 63
    if x >= width or y >= height:
        return None
    return {"head": (x, y), "pid": (low >> 12) & 8191}


def confirmed_pickup(removals, actor, dragon, head, move_event):
    return next((item for item in removals if item["cell"] == head and
                 item["event"] + 1 == move_event and item["was_present"]), None) \
        if actor == dragon else None


def advance_body(body, head, tail, initial=False):
    if initial:
        assert body[0] == head and body[-1] == tail
        return list(body)
    after = [head] + list(body)
    while len(after) > 1 and after[-1] != tail:
        after.pop()
    assert after[-1] == tail
    return after


def room_context(head, component, pearls, width, height):
    window = {((head[0] + dx) % width, (head[1] + dy) % height)
              for dx in range(-3, 4) for dy in range(-3, 4)}
    cells = {tuple(c) for c in component["cells"]}
    return {"component": component["id"], "area": component["area"],
            "canonical_portal_boundaries": component["portal_boundary_count"],
            "new_area_cap_eligible_geometry": component["portal_boundary_count"] == 1 and
                24 < component["area"] <= 49,
            "whole_component_current_visible": cells <= window,
            "visible_current_pearl_cells": sorted(pearls & cells & window),
            "offline_geometry_not_private_room_proof": True}


def scan_rooms(game, geometry):
    path = Path(game["replay"])
    assert base.sha(path) == game["replay_sha256"]
    root = base.Message(path.read_bytes()).structure(0, 0)
    assert root.number(0, "I") == 2
    text = root.text(0)
    assert hashlib.sha256(text.encode()).hexdigest() == geometry["sha256"]
    width, height, bodies, teams = base.parse_map(text)
    edges, _ = base.topology(text, width, height)
    room_for = {tuple(c): item["id"] for item in geometry["components"] for c in item["cells"]}
    eligible = {item["id"]: item for item in geometry["components"]
                if item["portal_boundary_count"] == 1 and item["area"] <= 49}
    components = {item["id"]: item for item in geometry["components"]}
    stats = {rid: {"component": rid, "area": item["area"],
        "new_area_cap_eligible_geometry": 24 < item["area"] <= 49,
        "configured_resource_cells": item["spawn_source_count"],
        "counts": {s: Counter() for s in "AB"}, "pickups": [], "crossings": [],
        "deaths": [], "claims": [], "checkpoint_heads": {}} for rid, item in eligible.items()}
    counts = {s: Counter() for s in "AB"}
    pearls, removals, actions, last_cross, pickup_counts = set(), [], {}, {}, Counter()
    world_hash, world_count = hashlib.sha256(), 0
    claims_seen, crossings, claims, pickups = set(), [], [], []
    round_index, actor, active = -1, None, None

    def inc(side, field, cell=None, amount=1):
        counts[side][field] += amount
        rid = room_for.get(cell)
        if rid in stats:
            stats[rid]["counts"][side][field] += amount

    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index, actor, active, removals = data.number(0), None, None, []
            if round_index in (20, 50, 100, 200, 350):
                for rid, state in stats.items():
                    state["checkpoint_heads"][round_index] = [
                        {"id": did, "side": teams[did], "head": body[0], "length": len(body)}
                        for did, body in bodies.items() if room_for[body[0]] == rid]
        elif kind == 1:
            actor, active, removals = data.number(0), None, []
            assert actor in bodies
            head, side = bodies[actor][0], teams[actor]
            if room_for[head] in stats:
                inc(side, "room_turns", head)
        elif kind == 3:
            cell, present = base.point(data.child(0)), bool(data.number(0, "B") & 1)
            if present:
                pearls.add(cell)
                removals = [item for item in removals if item["cell"] != cell]
            else:
                was_present = cell in pearls
                pearls.discard(cell)
                removals.append({"event": event_index, "cell": cell, "was_present": was_present})
        elif kind == 4:
            did = data.number(0)
            assert did == actor and did in bodies
            action_kind, requested, split_length = base.action(data)
            head = bodies[did][0]
            active = {"event": event_index, "round": round_index, "id": did,
                "side": teams[did], "action": requested, "kind": action_kind,
                "head_before": head, "body_before": list(bodies[did]),
                "length_before": len(bodies[did]), "split_length": split_length,
                "population_before": sum(teams[d] == teams[did] for d in bodies),
                "world_action_prefix_sha256": world_hash.hexdigest(),
                "world_action_prefix_events": world_count,
                "room_context": room_context(head, components[room_for[head]], pearls, width, height),
                "moves": [], "pickups": [], "claims": []}
            actions[event_index] = active
        elif kind == 9:
            did = data.number(0)
            head, tail = base.point(data.child(0)), base.point(data.child(1))
            old = list(bodies[did])
            bodies[did] = advance_body(old, head, tail, round_index < 0)
            if round_index >= 0:
                assert did == actor and active is not None and active["kind"] == 0
                step = len(active["moves"])
                direction = base.LETTERS.index(active["action"].removeprefix("MOVE ")[step])
                edge_kind, pid = edges[old[0], direction]
                update = {"event": event_index, "from": old[0], "to": head,
                    "length_before": len(old), "length_after": len(bodies[did]),
                    "direction": base.LETTERS[direction], "edge_kind": edge_kind,
                    "from_component": room_for[old[0]], "to_component": room_for[head]}
                active["moves"].append(update)
                if room_for[head] in stats:
                    inc(teams[did], "room_move_landings", head)
                removal = confirmed_pickup(removals, actor, did, head, event_index)
                if removal is not None:
                    pickup = {"round": round_index, "id": did, "side": teams[did],
                        "cell": head, "component": room_for[head],
                        "action_event": active["event"], "removal_event": removal["event"],
                        "move_event": event_index, "actual_move_step": step + 1,
                        "length_before": len(old), "length_after": len(bodies[did]),
                        "positive_net_gain": max(0, len(bodies[did]) - len(old))}
                    pickups.append(pickup)
                    active["pickups"].append(pickup)
                    pickup_counts[did] += 1
                    inc(teams[did], "confirmed_pickups", head)
                    inc(teams[did], "pickup_positive_net_gain", head, pickup["positive_net_gain"])
                    if room_for[head] in stats:
                        stats[room_for[head]]["pickups"].append(pickup)
                removals = []
                if room_for[old[0]] != room_for[head]:
                    if room_for[old[0]] in stats:
                        inc(teams[did], "room_exits", old[0])
                    if room_for[head] in stats:
                        inc(teams[did], "room_entries", head)
                if edge_kind == 2:
                    prior = last_cross.get(did)
                    cross = {"round": round_index, "event": event_index, "id": did,
                        "side": teams[did], "pid": pid, "from": old[0], "to": head,
                        "from_component": room_for[old[0]], "to_component": room_for[head],
                        "length_before": len(old), "length_after": len(bodies[did]),
                        "action": active["action"], "pickups_to_date": pickup_counts[did],
                        "same_pid_next_round_recross_no_pickup": bool(prior and prior["pid"] == pid
                            and prior["round"] + 1 == round_index and
                            prior["pickups_to_date"] == pickup_counts[did])}
                    crossings.append(cross)
                    inc(teams[did], "portal_crossings", head)
                    if cross["same_pid_next_round_recross_no_pickup"]:
                        inc(teams[did], "same_pid_next_round_recross_no_pickup", head)
                    for rid in {room_for[old[0]], room_for[head]} & stats.keys():
                        stats[rid]["crossings"].append(cross)
                    last_cross[did] = cross
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            parent_body, child_body = [base.point(p) for p in data.structs(0)], [base.point(p) for p in data.structs(1)]
            assert parent_body + list(reversed(child_body)) == bodies[parent]
            bodies[parent], bodies[child] = parent_body, child_body
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            did, reason = data.number(0), base.REASONS[data.number(4, "H")]
            rid = room_for[bodies[did][0]]
            if rid in stats:
                death = {"round": round_index, "event": event_index, "id": did,
                    "side": teams[did], "reason": reason, "head": bodies[did][0],
                    "length": len(bodies[did]), "active_actor": actor}
                stats[rid]["deaths"].append(death)
                inc(teams[did], "room_deaths", bodies[did][0])
            del bodies[did]
        elif kind == 12:
            sender = data.number(0)
            value = data.number(16, "Q")
            packet = decode_claim(value, teams[sender], width, height)
            if packet is not None:
                claim = {"event": event_index, "round": round_index, "sender": sender,
                    "side": teams[sender], "direction": base.LETTERS[data.number(4, "H")],
                    "value": value, **packet,
                    "hit": data.number(6, "H") == 1,
                    "hit_id": data.number(12) if data.number(6, "H") == 1 else None,
                    "hit_kind": data.number(24, "H"),
                    "beam_origin": base.point(data.child(0)), "beam_end": base.point(data.child(1))}
                claims.append(claim)
                unique = sender, round_index, packet["pid"], packet["head"]
                inc(teams[sender], "claim_beams", packet["head"])
                if claim["hit"]:
                    inc(teams[sender], "claim_hit_beams", packet["head"])
                if unique not in claims_seen:
                    inc(teams[sender], "claim_turns", packet["head"])
                    claims_seen.add(unique)
                if active is not None and active["id"] == sender:
                    active["claims"].append(claim)
                rid = room_for.get(packet["head"])
                if rid in stats:
                    stats[rid]["claims"].append(claim)
        item = base.canonical(event)
        if item is not None and kind != 12:
            base.update_digest(world_hash, item)
            world_count += 1
    final = {s: {"population": sum(teams[d] == s for d in bodies),
        "total": sum(len(b) for d, b in bodies.items() if teams[d] == s),
        "longest": max((len(b) for d, b in bodies.items() if teams[d] == s), default=0),
        "queen": len(bodies.get("AB".index(s), []))} for s in "AB"}
    assert final == game["standings"]
    for state in stats.values():
        state["counts"] = {s: dict(c) for s, c in state["counts"].items()}
    return {"actions": actions, "rooms": list(stats.values()),
        "counts": {s: dict(c) for s, c in counts.items()},
        "world_action_sha256": world_hash.hexdigest(), "world_action_events": world_count,
        "claims": claims, "crossings": crossings, "pickups": pickups, "final": final}


def first_physical(left_game, right_game):
    def stream(game):
        root = base.Message(Path(game["replay"]).read_bytes()).structure(0, 0)
        assert root.number(0, "I") == 2
        round_index = -1
        for event_index, event in enumerate(root.structs(3)):
            kind, data = event.number(0, "H"), event.child(0)
            if kind == 0:
                round_index = data.number(0)
            item = base.canonical(event)
            if item is not None:
                yield {"round": round_index, "event": event_index, "kind": kind, "canonical": item}
    for index, (a, b) in enumerate(zip_longest(stream(left_game), stream(right_game))):
        if a is None or b is None or a["canonical"] != b["canonical"]:
            return {"canonical_index": index, "H14": a, "R24": b,
                    "first_difference_is_sonar": bool(a and b and a["kind"] == b["kind"] == 12)}
    return None


def renamed_first(left, right, left_rooms, right_rooms):
    first = base.first_difference(left["actions"], right["actions"])
    if first is None:
        return None
    first.pop("verified_first_ordinary_allocation_effect")
    first["R24"] = first.pop("R21")
    for label, source in (("H14", left_rooms), ("R24", right_rooms)):
        a = first[label]
        if a is not None:
            first[label + "_room_evidence"] = source["actions"][a["event"]]
    a, b = first.get("H14_room_evidence"), first.get("R24_room_evidence")
    first["same_world_action_prefix_excluding_sonar"] = bool(a and b and
        a["world_action_prefix_sha256"] == b["world_action_prefix_sha256"] and
        a["world_action_prefix_events"] == b["world_action_prefix_events"] and
        (a["round"], a["id"], a["body_before"], a["population_before"]) ==
        (b["round"], b["id"], b["body_before"], b["population_before"]))
    return first


def summary(games, label, opponent, side=None, map_name=None):
    selected = [g for g in games if g["opponent"] == opponent and
        (side is None or g["side"] == side) and (map_name is None or g["map"] == map_name)]
    counts = Counter()
    for game in selected:
        counts.update(game[label + "_room_summary"]["counts"][game["side"]])
    return {"games": len(selected),
        **{field: sum(g[label + "_outcome"] == status for g in selected)
           for field, status in (("wins", "win"), ("losses", "loss"), ("draws", "draw"))},
        "own_observed_counts": dict(counts)}


def main():
    output, md = HERE / "r24_firstscreen_mechanism.json", HERE / "r24_firstscreen_mechanism.md"
    assert not output.exists() and not md.exists()
    assert base.sha(Path(base.__file__)) == SCANNER_SHA
    geometry_path = HERE / "room_cap_diagnosis.json"
    assert base.sha(geometry_path) == GEOMETRY_SHA
    geometry = {m["map"]: m for m in json.loads(geometry_path.read_text())["maps"]}
    cc, candidate, cp = base.verified_run(R24, R24_MAIN)
    bc, baseline, bp = base.verified_run(H14, H14_MAIN)
    assert {g["format_version"] for g in list(candidate.values()) + list(baseline.values())} == {2}
    assert set(candidate) == set(baseline)
    assert cc["base_seed"] == bc["base_seed"] == 82001
    assert cc["engine_sha256"] == bc["engine_sha256"]
    assert cc["engine_snapshot_files"] == bc["engine_snapshot_files"]
    assert {(m["name"], m["sha256"], m["seed"]) for m in cc["maps"]} == {(m["name"], m["sha256"], m["seed"]) for m in bc["maps"]}
    assert [s["prepared_sha256"] for s in cc["sources"][1:]] == [s["prepared_sha256"] for s in bc["sources"][1:]]
    assert [s["files"] for s in cc["sources"][1:]] == [s["files"] for s in bc["sources"][1:]]
    for name in ("helper.hpp", "queen_feed.hpp", "consolidate.hpp", "bot.toml"):
        assert cc["sources"][0]["files"][name] == bc["sources"][0]["files"][name]
    candidate_main = (Path(cc["sources"][0]["snapshot"]) / "main.cpp").read_text()
    baseline_main = (Path(bc["sources"][0]["snapshot"]) / "main.cpp").read_text()
    assert candidate_main.replace("more than 49 cells", "more than 24 cells").replace("if (cells.size() >= 49)", "if (cells.size() >= 24)") == baseline_main
    games = []
    for pair_key in sorted(candidate, key=lambda k: int(candidate[k]["game"])):
        game, control = candidate[pair_key], baseline[pair_key]
        left, right = base.scan(control), base.scan(game)
        a, b = scan_rooms(control, geometry[game["map"]]), scan_rooms(game, geometry[game["map"]])
        first = renamed_first(left, right, a, b)
        record = {"opponent": game["opponent"], "map": game["map"], "seed": int(game["seed"]),
            "side": game["bot1_side"], "H14_game": control["game"], "R24_game": game["game"],
            "H14_replay": control["replay"], "R24_replay": game["replay"],
            "H14_replay_sha256": control["replay_sha256"], "R24_replay_sha256": game["replay_sha256"],
            "H14_outcome": control["outcome"], "R24_outcome": game["outcome"],
            "H14_final": control["standings"], "R24_final": game["standings"],
            "same_actions": left["actions_sha256"] == right["actions_sha256"],
            "same_physical_sonar": left["physical_sha256"] == right["physical_sha256"],
            "same_world_action_excluding_sonar": a["world_action_sha256"] == b["world_action_sha256"],
            "first_action_divergence": first, "first_physical_sonar_divergence": first_physical(control, game),
            "H14_actions_sha256": left["actions_sha256"], "R24_actions_sha256": right["actions_sha256"],
            "H14_physical_sha256": left["physical_sha256"], "R24_physical_sha256": right["physical_sha256"]}
        for label, scanned in (("H14", a), ("R24", b)):
            record[label + "_room_summary"] = {k: scanned[k] for k in
                ("counts", "world_action_sha256", "world_action_events")}
            if game["map"] in ("default", "portals"):
                record[label + "_rooms"] = scanned["rooms"]
            record[label + "_claim_sample"] = scanned["claims"][:12]
        games.append(record)
        print(f"R24 paired mechanism {len(games)}/68", flush=True)
    metrics = {"games": len(games), "identical_action_games": sum(g["same_actions"] for g in games),
        "identical_physical_sonar_games": sum(g["same_physical_sonar"] for g in games),
        "identical_world_action_excluding_sonar_games": sum(g["same_world_action_excluding_sonar"] for g in games),
        "first_action_divergence_games": sum(g["first_action_divergence"] is not None for g in games),
        "strict_physical_sonar_prefix_first_action_games": sum(bool(g["first_action_divergence"] and
            g["first_action_divergence"]["strict_physical_sonar_prefix_equal"]) for g in games),
        "same_world_action_prefix_first_action_games": sum(bool(g["first_action_divergence"] and
            g["first_action_divergence"]["same_world_action_prefix_excluding_sonar"]) for g in games),
        "changed_action_maps": sorted({g["map"] for g in games if not g["same_actions"]}),
        "changed_physical_sonar_maps": sorted({g["map"] for g in games if not g["same_physical_sonar"]}),
        "changed_world_action_maps": sorted({g["map"] for g in games if not g["same_world_action_excluding_sonar"]})}
    summaries = {label: {op: {"all": summary(games, label, op),
        "A": summary(games, label, op, "A"), "B": summary(games, label, op, "B"),
        "default": summary(games, label, op, map_name="default"),
        "portals": summary(games, label, op, map_name="portals")}
        for op in ("V3_portal", "xxqztest")} for label in ("H14", "R24")}
    paired = {status: [] for status in ("promotions", "regressions")}
    for game in games:
        status = "promotions" if game["R24_outcome"] == "win" and game["H14_outcome"] != "win" else \
            "regressions" if game["H14_outcome"] == "win" and game["R24_outcome"] != "win" else None
        if status is not None:
            paired[status].append({k: game[k] for k in
                ("opponent", "map", "seed", "side", "H14_outcome", "R24_outcome")})
    limits = ["One development base82001 only; descriptive statistics, no test/p-value, confirmation or cross-seed claim.",
        "Room component labels and full bodies are offline geometry, not reconstructed persistent bot memory or proof activation.",
        "A current whole-component view plus current pearls is a sufficient observable resource witness, not a certificate of future ownership/safety.",
        "Confirmed pickup requires an observed present-to-absent transition immediately before actor landing on that cell in the same turn; net growth is separate because move payment can offset growth.",
        "Claim packets establish emitted beams/union-qualified hits, not recipient acceptance, avoidance, occupancy exclusivity or win causality.",
        "First full physical/SONAR prefix is reported separately from board/action prefix excluding sonar; earlier changed packets can invalidate the former before any action changes.",
        "Initial round-minus-one position events are asserted against map endpoints, never prepended as body movement or counted as pickups.",
        "Later post-divergence numerical dragon IDs are never paired across versions as causal identities.",
        "All negative/default/portals and unaffected-map tuples remain in JSON; no opponent/map subset replaces the registered 68-game screen."]
    payload = {"passed": True, "scope": "Native1.2.7 ranked17 A/B vs unchanged V3_portal and xxqztest; base82001 development only",
        "script_sha256": base.sha(__file__), "reused_scanner_sha256": SCANNER_SHA,
        "geometry_json_sha256": GEOMETRY_SHA, "provenance": [bp, cp],
        "metrics": metrics, "summaries": summaries, "paired_changes": paired,
        "limitations": limits, "games": games}
    lines = ["# R24 Versus Actual H14 Room Mechanism", "",
        "Both 68-game suites passed full official audits before analysis. Frozen source/binary/map/opponent/engine manifests, replay hashes and final standings were rechecked. No match was launched and no frozen candidate changed.", "",
        "One matched development base82001 only; descriptive statistics, no significance test or p-value.", "",
        "| Version | Opponent | Seat | Wins | Confirmed Pickups | Claim Beams | Claim Turns |", "| --- | --- | --- | ---: | ---: | ---: | ---: |"]
    for label in ("H14", "R24"):
        for op in ("V3_portal", "xxqztest"):
            for side in ("all", "A", "B"):
                s = summaries[label][op][side]
                c = s["own_observed_counts"]
                lines.append(f"| {label} | {op} | {side} | {s['wins']}/{s['games']} | {c.get('confirmed_pickups',0)} | {c.get('claim_beams',0)} | {c.get('claim_turns',0)} |")
    lines += ["", "## Paired Trace", "", "| Evidence | Value |", "| --- | --- |"]
    lines += [f"| {k} | {v} |" for k, v in metrics.items()]
    lines += ["", f"Win-status promotions {len(paired['promotions'])}; regressions {len(paired['regressions'])}. All changes remain in JSON.", "", "## Default Room Pickups", "",
        "Counts below refer to the two offline 25-cell single-boundary components. They are actual removed-pearl/landing events, not configured respawn sources, inferred proof activation or guaranteed retained terminal length.", "",
        "| Opponent | Seat | H14 Outcome | R24 Outcome | H14 Own/Enemy Pickups | R24 Own/Enemy Pickups | Own Claim Turns H14/R24 |", "| --- | --- | --- | --- | ---: | ---: | ---: |"]
    for g in (g for g in games if g["map"] == "default"):
        own, enemy = g["side"], "B" if g["side"] == "A" else "A"
        room_counts = {}
        for label in ("H14", "R24"):
            selected = [r for r in g[label + "_rooms"] if r["new_area_cap_eligible_geometry"]]
            room_counts[label] = {s: Counter() for s in "AB"}
            for room in selected:
                for side in "AB":
                    room_counts[label][side].update(room["counts"][side])
        a, b = room_counts["H14"], room_counts["R24"]
        lines.append(f"| {g['opponent']} | {own} | {g['H14_outcome']} | {g['R24_outcome']} | {a[own]['confirmed_pickups']}/{a[enemy]['confirmed_pickups']} | {b[own]['confirmed_pickups']}/{b[enemy]['confirmed_pickups']} | {a[own]['claim_turns']}/{b[own]['claim_turns']} |")
        first = g["first_action_divergence"]
        if first:
            l, r = first.get("H14_room_evidence"), first.get("R24_room_evidence")
            if l and r:
                lines += ["", f"{g['opponent']} {own}: first action round{r['round']} id{r['id']} {l['action']} -> {r['action']}; full physical/SONAR prefix equal={first['strict_physical_sonar_prefix_equal']}, board/action prefix excluding SONAR equal={first['same_world_action_prefix_excluding_sonar']}; before-head {r['head_before']}, area{r['room_context']['area']}, whole component current view={r['room_context']['whole_component_current_visible']}. First-action confirmed pickups {len(l['pickups'])} -> {len(r['pickups'])}.", ""]
    lines += ["", "## Interpretation And Limits", "",
        "The cap change has no dedicated activation telemetry. Same-prefix first action changes and emitted valid claim packets are narrower observable effects; later harvest and final outcomes are downstream trajectories. More claims must not be presented as a causal explanation for winning. A default-room pickup increase alone cannot establish broad-map or cross-seed improvement.", ""]
    lines += ["- " + text for text in limits]
    lines += ["", "Next decision should use the registered per-opponent/seat and paired outcome gate, retaining any adverse room or redeployment effect; expansion/promotion belongs to the main experiment owner."]
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    with md.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"passed": True, "metrics": metrics, "paired_changes": paired,
                      "json": str(output), "md": str(md)}), flush=True)


if __name__ == "__main__":
    main()
