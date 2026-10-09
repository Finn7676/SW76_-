#!/usr/bin/env python3
"""Analyze complete audited R26/H14 suites; never launch matches or edit bots."""
from __future__ import annotations

import argparse
from bisect import bisect_right
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path

import r21_firstscreen_mechanism as base
from h14_terminal_body_probe import own_complete
from queen_immediate_witness import ordinary_complete, project, visible_cells

HERE = Path(__file__).resolve().parent
R26 = "robust_headtrade_82001"
H14 = "robust_h14_82001"
R26_MAIN = "660363800c8cce16b1717d0f9dbbdf9feba6e02083380ccb1b3e1a5e92cc06d4"
R26_MODULE = "69bf74738cbf99f655eb9b715eec10f41b8946d184808b41fac28417c04ce4da"
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
BASE_SHA = "b0c28698c4752c0991b9463a239d793f028b6080913f4abf3cca4e11399112dd"
PROJECTOR_SHA = "678d32e1fa78a09f4da5cb9e6043d2e6f7ec6002d477de430682591101e396a6"
ENGINE_SHA = "26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546"
TAG = "SHORTER_HEAD_TRADE"


def parse_log(text):
    words = text.split()
    if words and words[0] == "LOG":
        words = words[1:]
    if not words or words[0] != TAG:
        return None
    assert len(words) == 11 and words[3::2] == ["before", "after", "complete_heads", "to"], text
    assert words[10] in base.LETTERS
    record = {"round": int(words[1]), "id": int(words[2]), "before": int(words[4]),
              "after": int(words[6]), "complete_heads": int(words[8]),
              "to": words[10], "text": text}
    assert record["before"] > 0 and record["after"] == 0 and record["complete_heads"] > 0
    return record


def head_mask(head, target, edges, width, height):
    mask = 0
    for direction in range(4):
        if edges.get((head, direction), (0, -1))[0] == 0 and \
                base.landing(head, direction, width, height) == target:
            mask |= 1 << direction
    return mask


def current_continuations(post, other, view, edges, width, height):
    found = []
    for direction in range(4):
        head = post["body"][0]
        cell = base.landing(head, direction, width, height)
        if edges.get((head, direction), (0, -1))[0] == 0 and cell in view and \
                cell not in post["body"] and cell not in other:
            found.append({"direction": base.LETTERS[direction], "cell": cell})
    return found


def observable_context(did, requested, bodies, teams, pearls, edges, width, height):
    body, side = list(bodies[did]), teams[did]
    view = visible_cells(body[0], width, height)
    other = {cell for actor, cells in bodies.items() if actor != did
             for cell in cells if cell in view}
    dirs = [base.LETTERS.index(c) for c in requested.removeprefix("MOVE ")] \
        if requested.startswith("MOVE ") else []
    ordinary = bool(dirs and len(dirs) <= 4)
    cell = body[0]
    for direction in dirs:
        ordinary &= cell in view and edges.get((cell, direction), (0, -1))[0] == 0
        cell = base.landing(cell, direction, width, height)
        ordinary &= cell in view
    complete_own = len(body) >= 2 and own_complete(body, view, edges, width, height)
    projected = project(body, dirs, other, pearls & view, view, edges, width, height) if dirs else None
    living = bool(projected and projected["known"] and projected["survives"])
    enemy_queen = 1 - "AB".index(side)
    enemies, exposures, witnesses = [], [], []
    for actor, cells in sorted(bodies.items()):
        if teams[actor] == side or cells[0] not in view:
            continue
        complete = len(cells) >= 2 and ordinary_complete(cells, view, edges, width, height)
        item = {"id": actor, "head": cells[0], "shown_body": [c for c in cells if c in view],
                "complete_ordinary_current_body": complete,
                "exact_length_if_complete": len(cells) if complete else None}
        enemies.append(item)
        mask = head_mask(cells[0], projected["body"][0], edges, width, height) if living else 0
        exposures.append({"id": actor, "mask": mask,
                          "directions": [base.LETTERS[d] for d in range(4) if mask & (1 << d)],
                          "complete_ordinary_current_body": complete})
        if living and complete and actor != enemy_queen and len(cells) < len(projected["body"]) and \
                projected["body"][0] not in cells and mask:
            occupancy = (other - set(cells)) | set(projected["body"])
            remaining = (pearls & view) - set(projected["eaten"])
            attack = next(d for d in range(4) if mask & (1 << d))
            proof = project(cells, [attack], occupancy, remaining, view, edges, width, height,
                            target=projected["body"][0])
            assert proof.get("reason") == "hit_target" and proof["hit_step"] == 1
            witnesses.append({"id": actor, "body": list(cells), "exact_length": len(cells),
                              "own_postlength": len(projected["body"]), "mask": mask,
                              "attack": "MOVE " + base.LETTERS[attack], "projection": proof})
    continuation = current_continuations(projected, other, view, edges, width, height) if living else []
    gates = {"nonqueen": did not in (0, 1), "complete_unique_ordinary_own_body": complete_own,
             "current_visible_ordinary_movement_at_most4": ordinary,
             "known_surviving_original_movement": living}
    return {"body_before": body, "side": side, "gates": gates,
            "necessary_original_entry_gates": all(gates.values()),
            "directions": dirs, "projection": projected, "visible_cells": sorted(view),
            "visible_other_cells": sorted(other), "visible_pearls": sorted(pearls & view),
            "visible_enemy_heads": enemies, "all_visible_enemy_head_exposures": exposures,
            "complete_visible_shorter_nonqueen_witnesses": witnesses,
            "complete_visible_enemy_heads": sum(e["complete_ordinary_current_body"] for e in enemies),
            "ordinary_current_continuations": continuation,
            "replacement_necessary_geometry": bool(all(gates.values()) and len(dirs) == 1 and continuation and not witnesses),
            "source_facing_and_private_admission_not_reconstructed": True}


def compare_geometry(original, replacement):
    old = {entry["id"]: entry["mask"] for entry in original["all_visible_enemy_head_exposures"]}
    new = {entry["id"]: entry["mask"] for entry in replacement["all_visible_enemy_head_exposures"]}
    additions = [{"id": did, "added_mask": mask & ~old.get(did, 0)}
                 for did, mask in new.items() if mask & ~old.get(did, 0)]
    return {"original_necessary_entry": original["necessary_original_entry_gates"],
            "original_shorter_firststep_trigger": bool(original["complete_visible_shorter_nonqueen_witnesses"]),
            "replacement_necessary_geometry": replacement["replacement_necessary_geometry"],
            "replacement_zero_complete_shorter_witnesses": not replacement["complete_visible_shorter_nonqueen_witnesses"],
            "no_added_any_visible_enemy_head_firststep_exposure": not additions,
            "added_head_exposure": additions}


def self_checks():
    edges, width, height = {}, 9, 9
    bodies, teams, pearls = {2: [(4, 4), (5, 4), (5, 5)], 3: [(3, 5), (3, 6)]}, \
                            {2: "A", 3: "B"}, {(3, 4)}
    original = observable_context(2, "MOVE W", bodies, teams, pearls, edges, width, height)
    alternate = observable_context(2, "MOVE N", bodies, teams, pearls, edges, width, height)
    assert len(original["complete_visible_shorter_nonqueen_witnesses"]) == 1
    assert all(v for k, v in compare_geometry(original, alternate).items() if k != "added_head_exposure")
    assert parse_log("LOG SHORTER_HEAD_TRADE 20 2 before 1 after 0 complete_heads 1 to N")["to"] == "N"
    assert parse_log("PROTECT_KEEP 20 2") is None
    queen = observable_context(0, "MOVE W", {0: bodies[2], 3: bodies[3]}, {0: "A", 3: "B"}, pearls,
                               edges, width, height)
    assert not queen["necessary_original_entry_gates"]
    enemy_queen = observable_context(2, "MOVE W", {2: bodies[2], 1: bodies[3]}, {2: "A", 1: "B"}, pearls,
                                     edges, width, height)
    assert not enemy_queen["complete_visible_shorter_nonqueen_witnesses"]
    portal = dict(edges); portal[((4, 4), 3)] = (2, 7)
    assert not observable_context(2, "MOVE W", bodies, teams, pearls, portal, width, height)["necessary_original_entry_gates"]
    partial = {**bodies, 3: [(3, 5), (3, 6), (3, 7), (3, 8)]}
    shown = observable_context(2, "MOVE W", partial, teams, pearls, edges, width, height)
    assert not shown["complete_visible_shorter_nonqueen_witnesses"] and shown["all_visible_enemy_head_exposures"][0]["mask"]
    additions = {**bodies, 4: [(4, 2), (3, 2), (2, 2), (1, 2), (0, 2)]}
    new_teams = {**teams, 4: "B"}
    orig2 = observable_context(2, "MOVE W", additions, new_teams, pearls, edges, width, height)
    alt2 = observable_context(2, "MOVE N", additions, new_teams, pearls, edges, width, height)
    assert not compare_geometry(orig2, alt2)["no_added_any_visible_enemy_head_firststep_exposure"]
    split = observable_context(2, "SPLIT 2", bodies, teams, pearls, edges, width, height)
    assert not split["necessary_original_entry_gates"]
    return {"passed": True, "assertions": 10, "no_replays_or_outcomes_read": True,
            "coverage": ["exact postgrowth shorter witness", "ordinary living continuation", "log parsing",
                         "queen and enemy-queen exclusion", "portal rejection", "partial enemy unknown",
                         "new partial-head exposure rejected", "split exclusion"]}


def next_after(rows, event):
    cursor = bisect_right([row["event"] for row in rows], event)
    return rows[cursor] if cursor < len(rows) else None


def summarize_actor(record, actions, pickups, deaths, final_bodies):
    event, did, round_index = record["action"]["event"], record["action"]["id"], record["action"]["round"]
    next_action = next_after(actions[did], event)
    death = deaths.get(did)
    later = [row for row in pickups[did] if row["action_event"] > event]
    first = [row for row in pickups[did] if row["action_event"] == event]
    post = record["observable_context"]["projection"]
    updates = record["action"]["actual_successful_updates"]
    record.update({"next_own_action": next_action, "actual_death": death,
        "death_before_next_own_action": bool(death and (next_action is None or death["event"] < next_action["event"])),
        "same_round_death": bool(death and death["round"] == round_index),
        "same_or_next_round_death": bool(death and death["round"] <= round_index + 1),
        "same_or_next_round_head_death": bool(death and death["round"] <= round_index + 1 and death["reason"] == "head"),
        "confirmed_pickups_from_selected_action": first, "later_only_confirmed_pickups": later,
        "alive_at_end": did in final_bodies, "end_body_if_alive": final_bodies.get(did),
        "actual_projection_exact_match": bool(post and post["known"] and post["survives"] and
            len(updates) == len(record["observable_context"]["directions"]) and updates and
            updates[-1]["body_after"] == post["body"])} )


def focused_scan(game, selected, candidate):
    path = Path(game["replay"])
    assert base.sha(path) == game["replay_sha256"]
    root = base.Message(path.read_bytes()).structure(0, 0)
    assert root.number(0, "I") == 2
    width, height, bodies, teams = base.parse_map(root.text(0))
    edges, _ = base.topology(root.text(0), width, height)
    selected_event = selected["event"] if selected else None
    pearls, removals, stream, prefix_count = set(), [], hashlib.sha256(), 0
    round_index, actor, active = -1, None, None
    actions, pickups, deaths = defaultdict(list), defaultdict(list), {}
    counts, selected_record, committed, pending = {s: Counter() for s in "AB"}, None, [], []
    logged = 0
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index, active, removals = data.number(0), None, []
        elif kind == 1:
            assert not pending, (path, "uncommitted replacement log")
            actor, active, removals = data.number(0), None, []
            assert actor in bodies
        elif kind == 3:
            cell = base.point(data.child(0))
            if data.number(0, "B") & 1:
                pearls.add(cell)
                removals = [row for row in removals if row["cell"] != cell]
            else:
                removals.append({"event": index, "cell": cell, "was_present": cell in pearls})
                pearls.discard(cell)
        elif kind == 6:
            entry = parse_log(data.text(0))
            if entry is not None:
                assert candidate and entry["round"] == round_index and entry["id"] == actor == data.number(0)
                pending.append({**entry, "log_event": index})
                logged += 1
        elif kind == 4:
            did = data.number(0)
            assert did == actor and did in bodies
            akind, requested, split_length = base.action(data)
            active = {"event": index, "round": round_index, "id": did, "side": teams[did],
                "action": requested, "kind": akind, "split_length": split_length,
                "body_before": list(bodies[did]), "actual_successful_updates": []}
            actions[did].append({key: active[key] for key in
                ("event", "round", "id", "side", "action", "kind", "split_length", "body_before")})
            record = None
            if selected_event == index or pending:
                record = {"action": active, "physical_prefix_sha256": stream.hexdigest(),
                    "physical_prefix_events": prefix_count, "observable_context": observable_context(
                        did, requested, bodies, teams, pearls, edges, width, height)}
            if selected_event == index:
                assert (round_index, did, requested, bodies[did]) == \
                    (selected["round"], selected["id"], selected["action"], selected["body_before"])
                assert (stream.hexdigest(), prefix_count) == \
                    (selected["physical_prefix_sha256"], selected["physical_prefix_events"])
                selected_record = record
            if pending:
                assert len(pending) == 1 and teams[did] == game["bot1_side"] and did not in (0, 1)
                log = pending[0]
                assert requested == "MOVE " + log["to"]
                record["committed_log"] = log
                record["original_private_proposal_not_logged"] = True
                record["logged_after_matches_observed_complete_shorter_count"] = \
                    log["after"] == len(record["observable_context"]["complete_visible_shorter_nonqueen_witnesses"])
                record["logged_complete_heads_matches_observed_count"] = \
                    log["complete_heads"] == record["observable_context"]["complete_visible_enemy_heads"]
                committed.append(record)
                pending = []
        elif kind == 9:
            did, head, tail = data.number(0), base.point(data.child(0)), base.point(data.child(1))
            old = list(bodies[did])
            if round_index < 0:
                assert old[0] == head and old[-1] == tail
            else:
                after = [head] + old
                while len(after) > 1 and after[-1] != tail:
                    after.pop()
                assert after[-1] == tail and did == actor and active is not None
                bodies[did] = after
                counts[teams[did]]["successful_updates"] += 1
                active["actual_successful_updates"].append({"event": index, "round": round_index,
                    "body_after": list(after), "length_before": len(old), "length_after": len(after)})
                removal = next((row for row in removals if row["cell"] == head and
                                row["event"] + 1 == index and row["was_present"]), None)
                if removal is not None:
                    pickup = {"event": index, "round": round_index, "id": did, "cell": head,
                        "removal_event": removal["event"], "action_event": active["event"],
                        "length_before": len(old), "length_after": len(after),
                        "positive_net_gain": max(0, len(after) - len(old))}
                    counts[teams[did]]["confirmed_pickups"] += 1
                    counts[teams[did]]["pickup_positive_net_gain"] += pickup["positive_net_gain"]
                    pickups[did].append(pickup)
                removals = []
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            parent_body, child_body = [base.point(p) for p in data.structs(0)], [base.point(p) for p in data.structs(1)]
            assert parent_body + list(reversed(child_body)) == bodies[parent]
            assert parent == actor and active is not None and active["split_length"] == len(child_body)
            bodies[parent], bodies[child] = parent_body, child_body
            teams[child] = "AB"[data.number(8, "H")]
            assert teams[child] == teams[parent]
            counts[teams[parent]]["births"] += 1
        elif kind == 11:
            did = data.number(0)
            counts[teams[did]]["deaths"] += 1
            counts[teams[did]]["nonqueen_deaths" if did not in (0, 1) else "queen_deaths"] += 1
            death = {"event": index, "round": round_index, "id": did,
                "reason": base.REASONS[data.number(4, "H")], "active_turn": actor,
                "acting_side": teams[actor] if actor is not None else None,
                "body_before": list(bodies[did]), "acting_action": active}
            deaths[did] = death
            if death["reason"] == "head" and death["acting_side"] != teams[did]:
                counts[teams[did]]["hostile_head_deaths"] += 1
            del bodies[did]
        item = base.canonical(event)
        if item is not None:
            base.update_digest(stream, item)
            prefix_count += 1
    assert not pending and logged == len(committed)
    final = {s: {"population": sum(teams[d] == s for d in bodies),
        "total": sum(len(b) for d, b in bodies.items() if teams[d] == s),
        "longest": max((len(b) for d, b in bodies.items() if teams[d] == s), default=0),
        "queen": len(bodies.get("AB".index(s), []))} for s in "AB"}
    assert final == game["standings"]
    if selected is not None:
        assert selected_record is not None
        summarize_actor(selected_record, actions, pickups, deaths, bodies)
    for record in committed:
        if record is not selected_record:
            summarize_actor(record, actions, pickups, deaths, bodies)
    return {"selected": selected_record, "committed": committed, "log_count": logged,
            "counts": {s: dict(c) for s, c in counts.items()}, "physical_sha256": stream.hexdigest(),
            "physical_events": prefix_count, "final_standings_checked": True}


def verify_packages():
    assert base.sha(Path(base.__file__)) == BASE_SHA
    assert base.sha(base.RUNS / "queen_immediate_witness.py") == PROJECTOR_SHA
    for label in (H14, R26):
        audit_path = base.RUNS / label / "verified_audit.json"
        assert audit_path.is_file(), f"Wait for the complete official audit: {audit_path}"
    cc, candidate, cp = base.verified_run(R26, R26_MAIN)
    bc, baseline, bp = base.verified_run(H14, H14_MAIN)
    assert set(candidate) == set(baseline) and cc["base_seed"] == bc["base_seed"] == 82001
    assert cc["engine_sha256"] == bc["engine_sha256"] == ENGINE_SHA
    assert cc["engine_snapshot_files"] == bc["engine_snapshot_files"]
    assert {(m["name"], m["sha256"], m["seed"]) for m in cc["maps"]} == \
        {(m["name"], m["sha256"], m["seed"]) for m in bc["maps"]}
    assert len(cc["maps"]) == 17
    assert [s["files"] for s in cc["sources"][1:]] == [s["files"] for s in bc["sources"][1:]]
    assert [s["prepared_sha256"] for s in cc["sources"][1:]] == [s["prepared_sha256"] for s in bc["sources"][1:]]
    for name in ("helper.hpp", "queen_feed.hpp", "consolidate.hpp", "bot.toml"):
        assert cc["sources"][0]["files"][name] == bc["sources"][0]["files"][name]
    assert cc["sources"][0]["files"]["headtrade.hpp"] == R26_MODULE
    candidate_main = (Path(cc["sources"][0]["snapshot"]) / "main.cpp").read_text()
    baseline_main = (Path(bc["sources"][0]["snapshot"]) / "main.cpp").read_text()
    assert candidate_main.replace('#include "headtrade.hpp"\n\n', "").replace('        reviewShorterHeadTrade(ct);\n', "") == baseline_main
    assert {g["format_version"] for g in list(candidate.values()) + list(baseline.values())} == {2}
    assert Counter((g["opponent"], g["bot1_side"]) for g in candidate.values()) == \
        Counter({(op, side): 17 for op in ("V3_portal", "xxqztest") for side in "AB"})
    return candidate, baseline, [bp, cp]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check-only", action="store_true")
    args = parser.parse_args()
    checks = self_checks()
    if args.self_check_only:
        print(json.dumps(checks))
        return
    output, report = HERE / "r26_firstscreen_mechanism.json", HERE / "r26_firstscreen_mechanism.md"
    assert not output.exists() and not report.exists(), "Preserve prior mechanism outputs"
    candidate, baseline, provenance = verify_packages()
    games = []
    for pair_key in sorted(candidate, key=lambda k: int(candidate[k]["game"])):
        game, control = candidate[pair_key], baseline[pair_key]
        left, right = base.scan(control), base.scan(game)
        first = base.first_difference(left["actions"], right["actions"])
        if first:
            first.pop("verified_first_ordinary_allocation_effect")
            first["R26"] = first.pop("R21")
        a = focused_scan(control, first["H14"] if first else None, False)
        b = focused_scan(game, first["R26"] if first else None, True)
        for full, focused in ((left, a), (right, b)):
            assert (full["physical_sha256"], full["physical_events"]) == \
                (focused["physical_sha256"], focused["physical_events"])
        consistent = False
        if first:
            geometry = compare_geometry(a["selected"]["observable_context"], b["selected"]["observable_context"]) \
                if a["selected"] and b["selected"] else None
            original, replacement = a["selected"], b["selected"]
            log = replacement.get("committed_log") if replacement else None
            consistent = bool(first["strict_physical_sonar_prefix_equal"] and geometry and
                all(value for name, value in geometry.items() if name != "added_head_exposure") and
                first["H14"]["side"] == first["R26"]["side"] == game["bot1_side"] and log and
                log["before"] == len(original["observable_context"]["complete_visible_shorter_nonqueen_witnesses"]) and
                replacement["logged_after_matches_observed_complete_shorter_count"] and
                replacement["logged_complete_heads_matches_observed_count"] and
                original["actual_projection_exact_match"] and replacement["actual_projection_exact_match"])
            first.update({"mechanism_consistent_strict_prefix_first_change": consistent,
                "visible_geometry_comparison": geometry, "H14_actor_evidence": original,
                "R26_actor_evidence": replacement})
        games.append({"opponent": game["opponent"], "map": game["map"], "seed": int(game["seed"]),
            "side": game["bot1_side"], "H14_game": control["game"], "R26_game": game["game"],
            "H14_replay": control["replay"], "R26_replay": game["replay"],
            "H14_replay_sha256": control["replay_sha256"], "R26_replay_sha256": game["replay_sha256"],
            "H14_outcome": control["outcome"], "R26_outcome": game["outcome"],
            "H14_final": control["standings"], "R26_final": game["standings"],
            "same_actions": left["actions_sha256"] == right["actions_sha256"],
            "same_physical_sonar": left["physical_sha256"] == right["physical_sha256"],
            "first_action_divergence": first, "R26_committed_changes": b["committed"],
            "R26_committed_log_count": b["log_count"], "H14_observed_counts": a["counts"],
            "R26_observed_counts": b["counts"]})
        print(f"R26 mechanism {len(games)}/68 logs={b['log_count']} strict-consistent={consistent}", flush=True)
    effects = [g for g in games if g["first_action_divergence"] and
               g["first_action_divergence"]["mechanism_consistent_strict_prefix_first_change"]]
    activations = [record for game in games for record in game["R26_committed_changes"]]
    metrics = {"games": len(games), "identical_action_games": sum(g["same_actions"] for g in games),
        "identical_physical_sonar_games": sum(g["same_physical_sonar"] for g in games),
        "committed_SHORTER_HEAD_TRADE_changes": len(activations),
        "games_with_committed_changes": sum(bool(g["R26_committed_changes"]) for g in games),
        "first_action_divergence_games": sum(g["first_action_divergence"] is not None for g in games),
        "strict_prefix_first_action_changes": sum(bool(g["first_action_divergence"] and
            g["first_action_divergence"]["strict_physical_sonar_prefix_equal"]) for g in games),
        "mechanism_consistent_strict_prefix_first_changes": len(effects),
        "other_first_action_divergences": sum(g["first_action_divergence"] is not None for g in games) - len(effects),
        "logged_replacements_observable_geometry_passed": sum(r["observable_context"]["replacement_necessary_geometry"] for r in activations),
        "logged_replacements_actual_projection_exact": sum(r["actual_projection_exact_match"] for r in activations),
        "logged_after_zero_matches_observed": sum(r["logged_after_matches_observed_complete_shorter_count"] for r in activations),
        "logged_complete_heads_matches_observed": sum(r["logged_complete_heads_matches_observed_count"] for r in activations)}
    for label in ("H14", "R26"):
        selected = [g["first_action_divergence"][label + "_actor_evidence"] for g in effects]
        metrics[label + "_first_actor_death_before_next_action"] = sum(r["death_before_next_own_action"] for r in selected)
        metrics[label + "_first_actor_same_or_next_round_head_deaths"] = sum(r["same_or_next_round_head_death"] for r in selected)
        metrics[label + "_first_actor_same_or_next_round_deaths"] = sum(r["same_or_next_round_death"] for r in selected)
        metrics[label + "_first_actor_later_only_pickups"] = sum(len(r["later_only_confirmed_pickups"]) for r in selected)
        metrics[label + "_first_actor_alive_at_end"] = sum(r["alive_at_end"] for r in selected)
    summaries = []
    groups = [(op, side, None) for op in ("V3_portal", "xxqztest") for side in (None, "A", "B")]
    groups += [(op, None, name) for op in ("V3_portal", "xxqztest") for name in sorted({g["map"] for g in games})]
    for opponent, side, map_name in groups:
        selected = [g for g in games if g["opponent"] == opponent and
                    (side is None or g["side"] == side) and (map_name is None or g["map"] == map_name)]
        row = {"opponent": opponent, "side": side or "all", "map": map_name or "all",
               "games": len(selected), "committed_changes": sum(g["R26_committed_log_count"] for g in selected),
               "strict_consistent_first_changes": sum(g in effects for g in selected)}
        for label in ("H14", "R26"):
            observed = Counter()
            for game in selected:
                observed.update(game[label + "_observed_counts"][game["side"]])
            row[label] = {"wins": sum(g[label + "_outcome"] == "win" for g in selected),
                          "own_observed_counts": dict(observed)}
        summaries.append(row)
    paired = {name: [] for name in ("promotions", "regressions")}
    for game in games:
        status = "promotions" if game["R26_outcome"] == "win" and game["H14_outcome"] != "win" else \
                 "regressions" if game["H14_outcome"] == "win" and game["R26_outcome"] != "win" else None
        if status:
            paired[status].append({name: game[name] for name in
                ("opponent", "map", "seed", "side", "H14_outcome", "R26_outcome")})
    limits = [
        "One audited development base82001, ranked17 A/B and two unchanged opponents; no confirmation, cross-seed or significance claim.",
        "Packed LOG records attach to the following official action of the same actor; every log must commit MOVE in its logged direction. Logging counts replacements, not rejected proposals.",
        "Only the first action change with equal physical/SONAR/action prefix digest/count, actor/body/round/population is a paired action intervention. Later divergent-prefix activations are descriptive trajectories.",
        "Original proposals are not in SHORTER_HEAD_TRADE logs. Original trigger and no-added-head exposure are independently verified only at strict-prefix H14/R26 first action changes.",
        "Replay geometry uses complete bodies restricted to radius3 current visibility and ordinary-tail boundary proof. It does not reconstruct segment-facing input metadata, private ownSeg/trail, score values, friend hard/soft SONAR protection or the donation flag; actual raw predecessor-chain fixtures are separate evidence.",
        "The candidate rejects newly exposed partial enemy heads; the first-change geometry compares all current visible head masks, including incomplete bodies. Partial bodies never authorize an exact shorter-length witness.",
        "A first ordinary free step onto a head is a local legal trade witness, not a promise about next actor order, intervening occupancy, hidden enemies or longer/paid/portal attacks.",
        "A current continuation is collision-before-tail-removal ordinary living space, not guaranteed next-turn behavior or survival.",
        "Pickups require a present-to-absent pearl event immediately followed by that actor landing; positive net length gain is separate from pickup when payment offsets growth.",
        "Actor deaths, later pickups and end length after the first same-prefix change are downstream observations; final outcome changes are paired results, not proof this one local event caused a win.",
        "Initial repeated position events are checked and skipped. Negative, unaffected, unmatched and geometry-inconsistent pairs remain in JSON; outputs are never overwritten."]
    payload = {"passed": True, "scope": "Audited frozen native1.2.7 R26/H14 base82001 mechanism evidence",
        "script_sha256": base.sha(__file__), "reused_scanner_sha256": BASE_SHA,
        "reused_projector_sha256": PROJECTOR_SHA, "own_body_probe_sha256": base.sha(HERE / "h14_terminal_body_probe.py"),
        "provenance": provenance, "synthetic_geometry_checks": checks, "metrics": metrics,
        "summaries": summaries, "paired_changes": paired, "limitations": limits, "games": games}
    lines = ["# R26 First-Screen Mechanism", "",
        "Both68-game suites passed complete official audits before analysis. Frozen source/binary/map/opponent/engine manifests, replay hashes, canonical prefixes and final standings were rechecked. No matches or policy edits were performed.", "",
        "| Evidence | Count |", "| --- | ---: |"]
    lines += [f"| {name} | {value} |" for name, value in metrics.items()]
    lines += ["", "Committed logs count executed replacement proposals. Strict-prefix first changes independently compare actual H14 proposals and R26 actions with necessary current visible geometry. Actor histories and paired wins are separate observations.", "",
        "| Opponent | Seat | H14 Wins | R26 Wins | Committed Changes | Strict First Changes |", "| --- | --- | ---: | ---: | ---: | ---: |"]
    lines += [f"| {r['opponent']} | {r['side']} | {r['H14']['wins']}/{r['games']} | {r['R26']['wins']}/{r['games']} | {r['committed_changes']} | {r['strict_consistent_first_changes']} |"
              for r in summaries if r["map"] == "all"]
    lines += ["", "## Strict-Prefix First Changes", ""]
    for game in effects:
        first = game["first_action_divergence"]
        a, b = first["H14"], first["R26"]
        lines.append(f"- {game['opponent']}/{game['map']}/seed{game['seed']}/{game['side']}: r{a['round']} id{a['id']} {a['action']} -> {b['action']}; H14/R26 immediate head deaths {first['H14_actor_evidence']['same_or_next_round_head_death']}/{first['R26_actor_evidence']['same_or_next_round_head_death']}; outcome {game['H14_outcome']} -> {game['R26_outcome']}.")
    lines += ["", f"Paired win-status promotions {len(paired['promotions'])}; regressions {len(paired['regressions'])}. Full committed activations by game, map/side/opponent summaries and actor histories remain in JSON.", "", "## Limits", ""]
    lines += ["- " + item for item in limits]
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    with report.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"passed": True, "metrics": metrics, "paired_changes": paired,
                      "json": str(output), "report": str(report)}), flush=True)


if __name__ == "__main__":
    main()
