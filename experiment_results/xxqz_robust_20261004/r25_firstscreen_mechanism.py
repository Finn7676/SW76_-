#!/usr/bin/env python3
"""Read-only R25/H14 first committed changes; require complete audits first."""
from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path

import r21_firstscreen_mechanism as base
from h14_terminal_body_probe import own_complete, terminal_proof
from queen_immediate_witness import project, visible_cells

HERE = Path(__file__).resolve().parent
R25 = "robust_short_terminal_82001"
H14 = "robust_h14_82001"
R25_MAIN = "2f682b33094ce2ed644cefd730cbcaad2ed400999f99c813c3579185db10e0a9"
R25_MODULE = "74f24a1a78d63371543cdd985cd6a53b0bf9860badbe05234bf35205d86b6c52"
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
SCANNER_SHA = "b0c28698c4752c0991b9463a239d793f028b6080913f4abf3cca4e11399112dd"
PROJECTOR_SHA = "678d32e1fa78a09f4da5cb9e6043d2e6f7ec6002d477de430682591101e396a6"


def observable_context(did, requested, bodies, teams, pearls, edges, width, height):
    body, side = list(bodies[did]), teams[did]
    view = visible_cells(body[0], width, height)
    occupied = {cell for cells in bodies.values() for cell in cells if cell in view}
    other = {cell for actor, cells in bodies.items() if actor != did
             for cell in cells if cell in view}
    enemies = [{"id": actor, "cell": cell} for actor, cells in sorted(bodies.items())
               if teams[actor] != side for cell in cells if cell in view]
    dirs = [base.LETTERS.index(c) for c in requested.removeprefix("MOVE ")] \
        if requested.startswith("MOVE ") else []
    destination = base.landing(body[0], dirs[0], width, height) if len(dirs) == 1 else None
    gates = {
        "nonqueen": did not in (0, 1), "LEN2": len(body) == 2,
        "single_move": requested.startswith("MOVE ") and len(dirs) == 1,
        "complete_unique_ordinary_current_ownbody": own_complete(body, view, edges, width, height),
        "no_current_visible_enemy_part": not enemies,
        "ordinary_current_visible_unoccupied_destination": bool(len(dirs) == 1 and
            edges.get((body[0], dirs[0]), (0, -1))[0] == 0 and
            destination in view and destination not in occupied),
        "current_visible_pearl_destination": destination in pearls and destination in view,
    }
    projected = project(body, dirs, other, pearls & view, view, edges, width, height) if dirs else None
    proof = terminal_proof(projected["body"], view, edges, width, height) \
        if projected and projected["known"] and projected["survives"] else None
    gates["known_surviving_LEN3_wall_ownbody_terminal"] = bool(projected and
        projected["known"] and projected["survives"] and len(projected["body"]) == 3 and proof is not None)
    onward = []
    if projected and projected["known"] and projected["survives"]:
        remaining = (pearls & view) - set(map(tuple, projected["eaten"]))
        for direction in range(4):
            follow = project(projected["body"], [direction], other, remaining, view, edges, width, height)
            if follow["known"] and follow["survives"] and \
                    terminal_proof(follow["body"], view, edges, width, height) is None:
                onward.append({"next_turn_action": "MOVE " + base.LETTERS[direction],
                               "projection": follow})
    return {"body_before": body, "side": side, "visible_cells": sorted(view),
        "visible_other_cells": sorted(other), "visible_enemy_parts": enemies,
        "visible_pearls": sorted(pearls & view), "destination": destination,
        "gates": gates, "necessary_original_terminal_gates": all(gates.values()),
        "projection": projected, "terminal_proof": proof,
        "ordinary_next_turn_nonterminal_continuations": onward,
        "ordinary_living_nonterminal_with_continuation": bool(
            gates["ordinary_current_visible_unoccupied_destination"] and projected and
            projected["known"] and projected["survives"] and proof is None and onward),
        "offline_observables_not_private_call_proof": True}


def self_checks():
    bodies = {75: [(4, 4), (3, 4)]}
    teams, pearls = {75: "B"}, {(4, 5)}
    edges = {((4, 5), d): (1, -1) for d in (1, 2, 3)}
    original = observable_context(75, "MOVE S", bodies, teams, pearls, edges, 9, 9)
    alternate = observable_context(75, "MOVE E", bodies, teams, pearls, edges, 9, 9)
    assert original["necessary_original_terminal_gates"]
    assert alternate["ordinary_living_nonterminal_with_continuation"]
    assert original["projection"]["body"] == [(4, 5), (4, 4), (3, 4)]
    assert not observable_context(75, "MOVE SS", bodies, teams, pearls, edges, 9, 9)["necessary_original_terminal_gates"]
    assert not observable_context(1, "MOVE S", {1: bodies[75]}, {1: "B"}, pearls, edges, 9, 9)["necessary_original_terminal_gates"]
    portals = dict(edges); portals[((4, 5), 1)] = (2, 4)
    assert not observable_context(75, "MOVE S", bodies, teams, pearls, portals, 9, 9)["necessary_original_terminal_gates"]
    enemy_body = {**bodies, 2: [(6, 6), (6, 7)]}
    assert not observable_context(75, "MOVE S", enemy_body, {**teams, 2: "A"}, pearls, edges, 9, 9)["necessary_original_terminal_gates"]
    blocked = {**bodies, 80: [(5, 4), (6, 4)]}
    assert not observable_context(75, "MOVE E", blocked, {**teams, 80: "B"}, pearls, edges, 9, 9)["ordinary_living_nonterminal_with_continuation"]
    return {"passed": True, "assertions": 8, "no_replay_or_outcome_read": True}


def actor_evidence(game, selected):
    path = Path(game["replay"])
    assert base.sha(path) == game["replay_sha256"]
    root = base.Message(path.read_bytes()).structure(0, 0)
    assert root.number(0, "I") == 2
    width, height, bodies, teams = base.parse_map(root.text(0))
    edges, _ = base.topology(root.text(0), width, height)
    pearls, removals = set(), []
    counts = {side: Counter() for side in "AB"}
    stream, prefix_count = hashlib.sha256(), 0
    round_index, actor, active = -1, None, None
    selected_id = selected["id"] if selected else None
    selected_event = selected["event"] if selected else None
    target, later_actions, pickups, death = None, [], [], None
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index, active, removals = data.number(0), None, []
        elif kind == 1:
            actor, active, removals = data.number(0), None, []
            assert actor in bodies
        elif kind == 3:
            cell = base.point(data.child(0))
            if data.number(0, "B") & 1:
                pearls.add(cell)
                removals = [item for item in removals if item["cell"] != cell]
            else:
                removals.append({"event": index, "cell": cell, "was_present": cell in pearls})
                pearls.discard(cell)
        elif kind == 4:
            did = data.number(0)
            assert did == actor and did in bodies
            akind, requested, split_length = base.action(data)
            active = {"event": index, "round": round_index, "id": did,
                "action": requested, "kind": akind, "split_length": split_length,
                "body_before": list(bodies[did]), "actual_successful_updates": []}
            if selected_event == index:
                assert (round_index, did, requested, bodies[did]) == \
                    (selected["round"], selected["id"], selected["action"], selected["body_before"])
                assert stream.hexdigest() == selected["physical_prefix_sha256"]
                assert prefix_count == selected["physical_prefix_events"]
                target = {"action": active, "observable_context": observable_context(
                    did, requested, bodies, teams, pearls, edges, width, height)}
            if selected_event is not None and index > selected_event and did == selected_id:
                later_actions.append(active)
        elif kind == 9:
            did = data.number(0)
            head, tail = base.point(data.child(0)), base.point(data.child(1))
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
                update = {"event": index, "round": round_index, "body_after": list(after),
                          "length_before": len(old), "length_after": len(after)}
                active["actual_successful_updates"].append(update)
                removal = next((item for item in removals if item["cell"] == head and
                    item["event"] + 1 == index and item["was_present"]), None)
                if removal is not None:
                    pickup = {"event": index, "round": round_index, "id": did,
                        "cell": head, "removal_event": removal["event"],
                        "action_event": active["event"], "length_before": len(old),
                        "length_after": len(after), "positive_net_gain": max(0, len(after)-len(old))}
                    counts[teams[did]]["confirmed_pickups"] += 1
                    counts[teams[did]]["pickup_positive_net_gain"] += pickup["positive_net_gain"]
                    if selected_event is not None and index > selected_event and did == selected_id:
                        pickups.append(pickup)
                removals = []
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            parent_body = [base.point(p) for p in data.structs(0)]
            child_body = [base.point(p) for p in data.structs(1)]
            assert parent_body + list(reversed(child_body)) == bodies[parent]
            bodies[parent], bodies[child] = parent_body, child_body
            teams[child] = "AB"[data.number(8, "H")]
            counts[teams[parent]]["births"] += 1
        elif kind == 11:
            did = data.number(0)
            counts[teams[did]]["deaths"] += 1
            counts[teams[did]]["nonqueen_deaths" if did not in (0, 1) else "queen_deaths"] += 1
            if selected_event is not None and index > selected_event and did == selected_id:
                death = {"event": index, "round": round_index, "id": did,
                    "reason": base.REASONS[data.number(4, "H")],
                    "active_turn": actor, "body_before": list(bodies[did])}
            del bodies[did]
        item = base.canonical(event)
        if item is not None:
            base.update_digest(stream, item)
            prefix_count += 1
    final = {side: {"population": sum(teams[d] == side for d in bodies),
        "total": sum(len(b) for d, b in bodies.items() if teams[d] == side),
        "longest": max((len(b) for d, b in bodies.items() if teams[d] == side), default=0),
        "queen": len(bodies.get("AB".index(side), []))} for side in "AB"}
    assert final == game["standings"]
    if selected is not None:
        assert target is not None
        target.update({"later_own_actions": later_actions, "confirmed_pickups_from_selected_action": pickups,
            "later_only_confirmed_pickups": [p for p in pickups if p["action_event"] > selected_event],
            "actual_death": death, "alive_at_end": selected_id in bodies,
            "end_body_if_alive": list(bodies[selected_id]) if selected_id in bodies else None,
            "same_or_next_round_death": bool(death and death["round"] <= selected["round"]+1),
            "next_round_self_death": bool(death and death["round"] == selected["round"]+1 and death["reason"] == "self")})
        predicted = target["observable_context"]["projection"]
        actual = target["action"]["actual_successful_updates"]
        target["actual_projection_exact_match"] = bool(predicted and predicted["known"] and
            predicted["survives"] and len(actual) == 1 and actual[0]["body_after"] == predicted["body"])
    return {"target": target, "counts": {s: dict(c) for s,c in counts.items()},
        "physical_sha256": stream.hexdigest(), "physical_events": prefix_count,
        "final_standings_checked": True}


def main():
    output, md = HERE / "r25_firstscreen_mechanism.json", HERE / "r25_firstscreen_mechanism.md"
    assert not output.exists() and not md.exists()
    assert base.sha(Path(base.__file__)) == SCANNER_SHA
    assert base.sha(base.RUNS / "queen_immediate_witness.py") == PROJECTOR_SHA
    checks = self_checks()
    cc, candidate, cp = base.verified_run(R25, R25_MAIN)
    bc, baseline, bp = base.verified_run(H14, H14_MAIN)
    assert set(candidate) == set(baseline)
    assert cc["base_seed"] == bc["base_seed"] == 82001
    assert cc["engine_sha256"] == bc["engine_sha256"] and cc["engine_snapshot_files"] == bc["engine_snapshot_files"]
    assert {(m["name"],m["sha256"],m["seed"]) for m in cc["maps"]} == {(m["name"],m["sha256"],m["seed"]) for m in bc["maps"]}
    assert len(cc["maps"]) == 17
    assert [s["files"] for s in cc["sources"][1:]] == [s["files"] for s in bc["sources"][1:]]
    assert [s["prepared_sha256"] for s in cc["sources"][1:]] == [s["prepared_sha256"] for s in bc["sources"][1:]]
    for name in ("helper.hpp", "queen_feed.hpp", "consolidate.hpp", "bot.toml"):
        assert cc["sources"][0]["files"][name] == bc["sources"][0]["files"][name]
    assert cc["sources"][0]["files"]["short_terminal.hpp"] == R25_MODULE
    candidate_main = (Path(cc["sources"][0]["snapshot"])/"main.cpp").read_text()
    baseline_main = (Path(bc["sources"][0]["snapshot"])/"main.cpp").read_text()
    assert candidate_main.replace('#include "short_terminal.hpp"\n\n', "").replace('        reviewShortTerminal(ct);\n', "") == baseline_main
    assert {g["format_version"] for g in list(candidate.values())+list(baseline.values())} == {2}
    assert Counter((g["opponent"],g["bot1_side"]) for g in candidate.values()) == \
        Counter({(op,side):17 for op in ("V3_portal","xxqztest") for side in "AB"})
    games = []
    for pair_key in sorted(candidate, key=lambda k:int(candidate[k]["game"])):
        game, control = candidate[pair_key], baseline[pair_key]
        left, right = base.scan(control), base.scan(game)
        first = base.first_difference(left["actions"],right["actions"])
        if first:
            first.pop("verified_first_ordinary_allocation_effect")
            first["R25"] = first.pop("R21")
        a = actor_evidence(control, first["H14"] if first else None)
        b = actor_evidence(game, first["R25"] if first else None)
        for full, focused in ((left,a),(right,b)):
            assert (full["physical_sha256"],full["physical_events"]) == (focused["physical_sha256"],focused["physical_events"])
        consistent = bool(first and first["strict_physical_sonar_prefix_equal"] and
            first["H14"]["side"] == game["bot1_side"] and first["R25"]["side"] == game["bot1_side"] and
            a["target"]["observable_context"]["necessary_original_terminal_gates"] and
            b["target"]["observable_context"]["ordinary_living_nonterminal_with_continuation"] and
            a["target"]["actual_projection_exact_match"] and b["target"]["actual_projection_exact_match"])
        if first:
            first["mechanism_consistent_observable_first_action_change"] = consistent
            first["H14_actor_evidence"], first["R25_actor_evidence"] = a["target"], b["target"]
        record = {"opponent":game["opponent"],"map":game["map"],"seed":int(game["seed"]),"side":game["bot1_side"],
            "H14_game":control["game"],"R25_game":game["game"],
            "H14_replay":control["replay"],"R25_replay":game["replay"],
            "H14_replay_sha256":control["replay_sha256"],"R25_replay_sha256":game["replay_sha256"],
            "H14_outcome":control["outcome"],"R25_outcome":game["outcome"],
            "H14_final":control["standings"],"R25_final":game["standings"],
            "same_actions":left["actions_sha256"]==right["actions_sha256"],
            "same_physical_sonar":left["physical_sha256"]==right["physical_sha256"],
            "first_action_divergence":first,"H14_observed_counts":a["counts"],"R25_observed_counts":b["counts"]}
        games.append(record)
        print(f"R25 paired mechanism {len(games)}/68 consistent-first-change={consistent}",flush=True)
    effects = [g for g in games if g["first_action_divergence"] and g["first_action_divergence"]["mechanism_consistent_observable_first_action_change"]]
    metrics = {"games":len(games),"identical_action_games":sum(g["same_actions"] for g in games),
        "identical_physical_sonar_games":sum(g["same_physical_sonar"] for g in games),
        "first_action_divergence_games":sum(g["first_action_divergence"] is not None for g in games),
        "strict_prefix_first_action_changes":sum(bool(g["first_action_divergence"] and g["first_action_divergence"]["strict_physical_sonar_prefix_equal"]) for g in games),
        "mechanism_consistent_observable_first_changes":len(effects),
        "other_first_action_divergence_games":sum(g["first_action_divergence"] is not None for g in games)-len(effects)}
    for label in ("H14","R25"):
        selected = [g["first_action_divergence"][label+"_actor_evidence"] for g in effects]
        metrics[label+"_first_actor_next_round_self_deaths"] = sum(x["next_round_self_death"] for x in selected)
        metrics[label+"_first_actor_same_or_next_round_deaths"] = sum(x["same_or_next_round_death"] for x in selected)
        metrics[label+"_first_actor_later_only_pickups"] = sum(len(x["later_only_confirmed_pickups"]) for x in selected)
        metrics[label+"_first_actor_alive_at_end"] = sum(x["alive_at_end"] for x in selected)
    summaries = []
    for opponent in ("V3_portal","xxqztest"):
        for side in (None,"A","B"):
            selected = [g for g in games if g["opponent"]==opponent and (side is None or g["side"]==side)]
            row = {"opponent":opponent,"side":side or "all","games":len(selected)}
            for label in ("H14","R25"):
                observed = Counter()
                for g in selected: observed.update(g[label+"_observed_counts"][g["side"]])
                row[label] = {"wins":sum(g[label+"_outcome"]=="win" for g in selected),"own_observed_counts":dict(observed)}
            summaries.append(row)
    paired = {status:[] for status in ("promotions","regressions")}
    for g in games:
        status = "promotions" if g["R25_outcome"]=="win" and g["H14_outcome"]!="win" else \
            "regressions" if g["H14_outcome"]=="win" and g["R25_outcome"]!="win" else None
        if status: paired[status].append({k:g[k] for k in ("opponent","map","seed","side","H14_outcome","R25_outcome")})
    limits = [
        "One development base82001, all17maps, both seats and both unchanged opponents; no held-out, cross-seed, p-value or generalization claim.",
        "The first committed change requires byte-equivalent canonical physical/SONAR/action prefix hashes and counts plus actor/body/round/population agreement; later divergent prefixes are not causal pairs.",
        "Observable geometry uses true replay bodies restricted to the current radius3 view; it does not reconstruct private trail, current segment facing metadata, inherited score values, persistent memory, donation flag or accepted protection/SONAR status.",
        "No dedicated R25 telemetry exists. A strict-prefix change with necessary observable gates is mechanism-consistent, not proof that a private R25 call or every private admission gate activated; no complete activation count is inferred.",
        "Continuation is a current-view next-turn projection with consumed pearls removed and collision checked before tail removal. It is not a same-turn sprint or a promise of future hidden-body/actor safety.",
        "A pickup is counted only for a present-to-absent pearl transition immediately preceding the same actor landing; positive net length gain is separate because payment can offset growth.",
        "The selected actor exists identically at the first same-prefix action, but subsequent pickups/death/end length are downstream trajectory observations. Later child IDs are not paired across versions as causal identities.",
        "Fresh native and scripted local rule fixtures are separate source/legality evidence, not trajectory benefit or a causal explanation for wins.",
        "Initial round-minus-one position updates are checked against map endpoints and never prepended as movement or counted as pickups. All negative and unaffected pair tuples remain in JSON.",
    ]
    payload = {"passed":True,"scope":"Audited native1.2.7 ranked17 A/B both unchanged opponents, base82001 only",
        "script_sha256":base.sha(__file__),"reused_scanner_sha256":SCANNER_SHA,
        "reused_projector_sha256":PROJECTOR_SHA,"reused_terminal_probe_sha256":base.sha(HERE/"h14_terminal_body_probe.py"),
        "provenance":[bp,cp],"synthetic_geometry_checks":checks,"metrics":metrics,
        "summaries":summaries,"paired_changes":paired,"limitations":limits,"games":games}
    lines = ["# R25 First-Screen Mechanism","",
        "Both68-game suites passed official audits before analysis. Frozen source/binary/map/opponent/engine manifests, replay hashes, canonical prefixes and final standings were rechecked. No match or policy edit was performed.","",
        "| Evidence | Count |","| --- | ---: |"]
    lines += [f"| {k} | {v} |" for k,v in metrics.items()]
    lines += ["","The consistent first changes are observed committed action differences with the registered necessary visible geometry. No private activation telemetry exists. Local escape and later collector outcomes are reported separately from the game's win status.","",
        "| Opponent | Seat | H14 Wins | R25 Wins | H14/R25 Own Pickups | H14/R25 Nonqueen Deaths |","| --- | --- | ---: | ---: | ---: | ---: |"]
    for row in summaries:
        a,b = row["H14"],row["R25"]; ac,bcnt = a["own_observed_counts"],b["own_observed_counts"]
        lines.append(f"| {row['opponent']} | {row['side']} | {a['wins']}/{row['games']} | {b['wins']}/{row['games']} | {ac.get('confirmed_pickups',0)}/{bcnt.get('confirmed_pickups',0)} | {ac.get('nonqueen_deaths',0)}/{bcnt.get('nonqueen_deaths',0)} |")
    lines += ["","## Same-Prefix First Changes",""]
    for g in (x for x in games if x["first_action_divergence"]):
        first=g["first_action_divergence"]; a,b=first["H14"],first["R25"]
        if a and b:
            lines.append(f"- {g['opponent']} / {g['map']} / seed{g['seed']} / {g['side']}: r{a['round']} id{a['id']} {a['action']} -> {b['action']}; strict-prefix={first['strict_physical_sonar_prefix_equal']}; mechanism-consistent={first['mechanism_consistent_observable_first_action_change']}; outcome {g['H14_outcome']} -> {g['R25_outcome']}.")
            for label in ("H14","R25"):
                evidence=first[label+"_actor_evidence"]; death=evidence["actual_death"]
                lines.append(f"  {label}: first projection exact={evidence['actual_projection_exact_match']}; next-round self death={evidence['next_round_self_death']}; later-only pickups={len(evidence['later_only_confirmed_pickups'])}; death={death}; alive-at-end={evidence['alive_at_end']}.")
        else:
            lines.append(f"- {g['opponent']} / {g['map']} / {g['side']}: unmatched terminal action-list length divergence; no same-prefix mechanism effect inferred.")
    lines += ["",f"Paired win-status promotions {len(paired['promotions'])}; regressions {len(paired['regressions'])}. All68 pairs, full first-actor event witnesses and downstream histories remain in JSON.","","## Limits",""]
    lines += ["- "+item for item in limits]
    with output.open("x") as handle:
        json.dump(payload,handle,indent=2); handle.write("\n")
    with md.open("x") as handle: handle.write("\n".join(lines)+"\n")
    print(json.dumps({"passed":True,"metrics":metrics,"paired_changes":paired,"json":str(output),"md":str(md)}),flush=True)


if __name__ == "__main__":
    main()
