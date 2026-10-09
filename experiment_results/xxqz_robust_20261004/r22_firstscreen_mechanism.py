#!/usr/bin/env python3
"""Read-only R22 versus actual frozen R10 first-screen mechanism analysis."""
from __future__ import annotations

from collections import Counter
import json
from pathlib import Path

import r21_firstscreen_mechanism as base

HERE = Path(__file__).resolve().parent
R22 = "robust_queen_rear_82001"
R10 = "robust_queen_hunt_82001_retry1"
R22_MAIN = "43ed3e523628fb34f74279943f80408f149353d5a3440a6fbadf73cfe4ff8138"
R10_MAIN = "de27f3b2abb1440d75395c9676eb4d80967947bd7adb7e66f9570005f24ca8ba"
SCANNER_SHA = "b0c28698c4752c0991b9463a239d793f028b6080913f4abf3cca4e11399112dd"
FACTOR = """        if (!isQueen() && ROUND < 350 && LEN >= 5 &&
            ct.can_split(LEN - 2) && splitSafe(LEN - 2)) {
            doSplit(LEN - 2);
            return;
        }
"""


def hunting(game, source):
    """Attach official head deaths to the currently acting requested move."""
    root = base.Message(Path(game["replay"]).read_bytes()).structure(0, 0)
    width, height, bodies, teams = base.parse_map(root.text(0))
    edges, _ = base.topology(root.text(0), width, height)
    indexed = {a["event"]: a for a in source["actions"]}
    own, active, body, round_index = game["bot1_side"], None, None, -1
    counts, trades, queen_deaths = Counter(), [], []

    def finish():
        if active is None or active["side"] != own:
            return
        counts["recorded_actions"] += 1
        if active["kind"] == 0:
            counts["move_requests"] += 1
            counts[f"requested_steps_{len(active['directions'])}"] += 1
            counts["successful_steps"] += len(active["updates"])
        elif active["kind"] == 1:
            counts["split_requests"] += 1
        elif active["kind"] == 2:
            counts["suicide_requests"] += 1
        else:
            counts["no_action_pointer"] += 1
        self_head = next((d for d in active["deaths"] if d["id"] == active["id"] and d["reason"] == "head"), None)
        victims = [d for d in active["deaths"] if d["side"] != own and d["reason"] == "head"]
        if not self_head or not victims:
            return
        step = len(active["updates"]) + 1
        free_steps = (active["length_before"] + 3) // 4
        assert active["kind"] == 0 and step <= len(active["directions"])
        eligible = step <= free_steps or self_head["actor_length_before_death"] > 2
        assert eligible
        collision_direction = base.LETTERS.index(active["directions"][step - 1])
        last_head = tuple(self_head["body_before"][0])
        expected_hit = base.landing(last_head, collision_direction, width, height)
        ordinary_collision = edges.get((last_head, collision_direction), (0, -1))[0] == 0 and any(
            tuple(d["body_before"][0]) == expected_hit for d in victims)
        ordinary_path = ordinary_collision and all(u["ordinary_step"] for u in active["updates"])
        sprint_hit = step >= 3 and active["id"] not in (0, 1) and ordinary_path
        entry = {**active, "actual_hit_step": step, "initial_free_steps": free_steps,
            "actual_hit_direction": active["directions"][step - 1],
            "actual_hit_step_eligible": eligible,
            "enemy_queen_hit": any(d["id"] in (0, 1) for d in victims),
            "actual_hit_after_two_steps": step >= 3,
            "ordinary_actual_path_and_collision": ordinary_path,
            "actual_nonqueen_ordinary_sprint_hit": sprint_hit}
        trades.append(entry)
        counts["our_initiated_head_trades"] += 1
        counts[f"head_trades_requested_steps_{len(active['directions'])}"] += 1
        counts[f"head_trades_actual_hit_step_{step}"] += 1
        counts["our_initiated_enemy_queen_hits"] += entry["enemy_queen_hit"]
        counts["head_hits_after_two_steps"] += sprint_hit
        counts["enemy_queen_hits_after_two_steps"] += entry["enemy_queen_hit"] and sprint_hit

    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind in (0, 1):
            finish()
            active, body = None, None
            if kind == 0:
                round_index = data.number(0)
        elif kind == 4:
            assert active is None
            a = indexed[event_index]
            active = {k: a[k] for k in ("round", "id", "side", "event", "turn_event", "kind", "action", "length_before", "population_before", "body_before")}
            active["directions"] = a["action"].removeprefix("MOVE ") if a["kind"] == 0 else ""
            active["updates"], active["deaths"] = [], []
            assert bodies[a["id"]] == a["body_before"]
            body = bodies[a["id"]]
        elif kind == 9 and round_index >= 0:
            did = data.number(0)
            head, tail = base.point(data.child(0)), base.point(data.child(1))
            current = bodies[did]
            previous = current[0]
            current.insert(0, head)
            while len(current) > 1 and current[-1] != tail:
                current.pop()
            assert current[-1] == tail
            if active is not None and did == active["id"]:
                step_index = len(active["updates"])
                direction = base.LETTERS.index(active["directions"][step_index])
                ordinary = edges.get((previous, direction), (0, -1))[0] == 0 and base.landing(previous, direction, width, height) == head
                active["updates"].append({"event": event_index, "head": head,
                    "tail": tail, "length_after": len(current), "ordinary_step": ordinary})
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [base.point(p) for p in data.structs(0)]
            bodies[child] = [base.point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            did, reason = data.number(0), base.REASONS[data.number(4, "H")]
            death = {"event": event_index, "round": round_index, "id": did,
                "side": teams[did], "reason": reason,
                "active_actor": active["id"] if active else None,
                "actor_length_before_death": len(bodies[did]) if active and did == active["id"] else None,
                "body_before": list(bodies[did])}
            if active is not None:
                active["deaths"].append(death)
            if did in (0, 1):
                queen_deaths.append(death)
            del bodies[did]
    finish()
    hit_deaths = {d["event"] for t in trades for d in t["deaths"] if d["side"] != own and d["reason"] == "head"}
    for death in queen_deaths:
        death["our_initiated_hit"] = death["event"] in hit_deaths
    return {"counts": dict(counts), "our_actual_initiated_head_trades": trades,
            "queen_deaths": queen_deaths}


def renamed_difference(left, right):
    first = base.first_difference(left["actions"], right["actions"])
    if first is None:
        return None
    first["R10"], first["R22"] = first.pop("H14"), first.pop("R21")
    return first


def cascade_all(source, first):
    if first is None:
        return None
    # Every actual split is attached to its recorded action by the shared scanner.
    splits = [a["actual_split"] for a in source["actions"] if "actual_split" in a]
    descendants, links = {first["child"]}, []
    for s in splits:
        if s["round"] == first["round"] and s["event"] > first["event"] and s["parent"] in descendants:
            links.append({k: s[k] for k in ("event", "action_event", "parent", "child", "parent_length", "child_length", "body_before", "parent_body", "child_body")})
            descendants.add(s["child"])
    return {"round": first["round"], "first_split_event": first["event"],
            "initial_parent": first["parent"], "initial_child": first["child"],
            "initial_child_length": first["child_length"],
            "same_round_descendant_births": len(links), "same_round_descendant_splits": links}


def geometry(source):
    pool = source["early_nonqueen_LEN5plus_split_pool"]
    parent2 = [s for s in pool if s["parent_length"] == 2]
    deaths = {s["parent_death"]["event"]: s["parent_death"]["reason"] for s in parent2 if s["parent_death_before_following_round_end"]}
    return {"raw_early_nonqueen_LEN5plus_split_pool_count": len(pool),
        "raw_pool_child2_count": sum(s["child_length"] == 2 for s in pool),
        "raw_pool_parent2_count": len(parent2),
        "parent2_geometry": dict(Counter(s["parent_LEN2_geometry"]["classification"] for s in parent2)),
        "parent2_portal_link_witnesses": [s for s in parent2 if s["parent_LEN2_geometry"]["portal_links"]],
        "parent2_actual_distinct_same_or_next_round_death_reasons": dict(Counter(deaths.values())),
        "raw_pool_is_not_ordinary_activation_count": True}


def summary(games, label, opponent, side=None):
    gs = [g for g in games if g["opponent"] == opponent and (side is None or g["side"] == side)]
    counts, geom, deaths = Counter(), Counter(), Counter()
    for g in gs:
        counts.update(g[label + "_hunting"]["counts"])
        geom.update(g[label + "_geometry"]["parent2_geometry"])
        deaths.update(g[label + "_geometry"]["parent2_actual_distinct_same_or_next_round_death_reasons"])
    return {"games": len(gs), "wins": sum(g[label + "_outcome"] == "win" for g in gs),
        "losses": sum(g[label + "_outcome"] == "loss" for g in gs),
        "draws": sum(g[label + "_outcome"] == "draw" for g in gs),
        "hunting_counts": dict(counts), "parent2_geometry": dict(geom),
        "parent2_distinct_same_or_next_round_death_reasons": dict(deaths)}


def main():
    out_json, out_md = HERE / "r22_firstscreen_mechanism.json", HERE / "r22_firstscreen_mechanism.md"
    assert not out_json.exists() and not out_md.exists()
    assert base.sha(Path(base.__file__)) == SCANNER_SHA
    cc, candidate, cp = base.verified_run(R22, R22_MAIN)
    bc, controls, bp = base.verified_run(R10, R10_MAIN)
    assert set(candidate) == set(controls)
    assert cc["base_seed"] == bc["base_seed"] == 82001
    assert cc["engine_sha256"] == bc["engine_sha256"]
    assert cc["engine_snapshot_files"] == bc["engine_snapshot_files"]
    assert {(m["name"], m["sha256"], m["seed"]) for m in cc["maps"]} == {(m["name"], m["sha256"], m["seed"]) for m in bc["maps"]}
    assert [s["prepared_sha256"] for s in cc["sources"][1:]] == [s["prepared_sha256"] for s in bc["sources"][1:]]
    assert [{k: v for k, v in s["files"].items() if not k.startswith(".unswbc-build/")} for s in cc["sources"][1:]] == [{k: v for k, v in s["files"].items() if not k.startswith(".unswbc-build/")} for s in bc["sources"][1:]]
    for name in ("helper.hpp", "queen_feed.hpp", "consolidate.hpp", "bot.toml"):
        assert cc["sources"][0]["files"][name] == bc["sources"][0]["files"][name]
    candidate_main = (Path(cc["sources"][0]["snapshot"]) / "main.cpp").read_text()
    baseline_main = (Path(bc["sources"][0]["snapshot"]) / "main.cpp").read_text()
    assert candidate_main.count(FACTOR) == 1 and candidate_main.replace(FACTOR, "") == baseline_main
    games = []
    for pair_key in sorted(candidate, key=lambda k: int(candidate[k]["game"])):
        game, control = candidate[pair_key], controls[pair_key]
        left, right = base.scan(control), base.scan(game)
        first = renamed_difference(left, right)
        record = {"opponent": game["opponent"], "map": game["map"], "seed": int(game["seed"]), "side": game["bot1_side"],
            "R10_game": control["game"], "R22_game": game["game"],
            "R10_replay": control["replay"], "R22_replay": game["replay"],
            "R10_replay_sha256": control["replay_sha256"], "R22_replay_sha256": game["replay_sha256"],
            "R10_outcome": control["outcome"], "R22_outcome": game["outcome"],
            "R10_final": control["standings"], "R22_final": game["standings"],
            "same_actions": left["actions_sha256"] == right["actions_sha256"],
            "same_physical_sonar": left["physical_sha256"] == right["physical_sha256"],
            "R10_actions_sha256": left["actions_sha256"], "R22_actions_sha256": right["actions_sha256"],
            "R10_physical_sha256": left["physical_sha256"], "R22_physical_sha256": right["physical_sha256"],
            "first_action_divergence": first, "R10_geometry": geometry(left), "R22_geometry": geometry(right),
            "R10_hunting": hunting(control, left), "R22_hunting": hunting(game, right)}
        for label, source in (("R10", left), ("R22", right)):
            initial_split = first[label].get("actual_split") if first and first[label] else None
            record[label + "_first_allocation_same_round_cascade"] = cascade_all(source, initial_split)
        record["checkpoints"] = {}
        for r in (20, 50, 100):
            delta = base.checkpoint_delta(left, right, game["bot1_side"], r)
            if delta["available"]:
                delta["R10"], delta["R22"] = delta.pop("H14"), delta.pop("R21")
                delta["own_R22_minus_R10"] = delta.pop("own_R21_minus_H14")
                delta["own_births_R22_minus_R10"] = delta.pop("own_births_R21_minus_H14")
            else:
                delta["R10_reached"], delta["R22_reached"] = delta.pop("H14_reached"), delta.pop("R21_reached")
            record["checkpoints"][r] = delta
        games.append(record)
        print(f"R22 paired mechanism {len(games)}/68", flush=True)
    effects = [g for g in games if g["first_action_divergence"] and g["first_action_divergence"]["verified_first_ordinary_allocation_effect"]]
    metrics = {"games": len(games), "identical_action_games": sum(g["same_actions"] for g in games),
        "identical_physical_sonar_games": sum(g["same_physical_sonar"] for g in games),
        "first_action_divergence_games": sum(g["first_action_divergence"] is not None for g in games),
        "strict_prefix_first_ordinary_allocation_effect_games": len(effects),
        "first_effect_round_zero_games": sum(g["first_action_divergence"]["R22"]["round"] == 0 for g in effects),
        "first_effect_actual_same_round_descendant_cascade_games": sum(g["R22_first_allocation_same_round_cascade"]["same_round_descendant_births"] > 0 for g in effects)}
    summaries = {label: {op: summary(games, label, op) for op in ("V3_portal", "xxqztest")} for label in ("R10", "R22")}
    paired = {op: {"promotions": [], "regressions": []} for op in ("V3_portal", "xxqztest")}
    for g in games:
        bucket = "promotions" if g["R22_outcome"] == "win" and g["R10_outcome"] != "win" else "regressions" if g["R10_outcome"] == "win" and g["R22_outcome"] != "win" else None
        if bucket:
            paired[g["opponent"]][bucket].append({k: g[k] for k in ("map", "seed", "side", "R10_outcome", "R22_outcome", "R10_final", "R22_final", "first_action_divergence", "R10_hunting", "R22_hunting", "R10_replay", "R22_replay")})
    limits = ["One matched development seed-base only; no statistical test or p-value, no cross-seed or held-out claim.",
        "Actual R10 retry1 is the baseline; neither H14 nor a mislabeled source tree is substituted.",
        "Strict-prefix first actual allocation is a game-level lower bound, not a complete ordinary-branch activation count or win causality.",
        "Raw early split pools include emergency/protection branches; later child IDs are only within-game lineages.",
        "Sprint hunting counts require a nonqueen own initiated ordinary-path head collision at actual step>=3 with both official head deaths in the actor's turn; generic multi-step movement is not hunt activation.",
        "Requested move length and actual collision step are reported separately; an unused later direction does not prove a longer-range hit.",
        "R22 changes no sprint-search source. Post-divergence attack counts are downstream trajectories, not direct rear-factor/hunt-branch attribution.",
        "No parent2 portal-link witness on this screen is not universal safety; actual death reasons are never relabeled from invalid_action.",
        "Inherited scanner NO_VALID_ACTION means absent action pointer, not inferred death reason; its ordered diagnostic logs omit raw IDs.",
        "Checkpoint snapshots precede each round; early-ended games are not padded."]
    payload = {"passed": True, "scope": "Native1.2.7 base82001 ranked17 both seats versus unchanged V3_portal/xxqztest",
        "script_sha256": base.sha(__file__), "reused_scanner_sha256": SCANNER_SHA,
        "provenance": [bp, cp], "source_binding": "Exact five-line ordinary rear factor; shared compiled files unchanged",
        "metrics": metrics, "summaries": summaries, "paired_changes": paired,
        "limitations": limits, "games": games}
    lines = ["# R22 Versus Actual R10 First-Screen Mechanism", "", "Both68-game suites passed complete official outcome audits. Frozen sources/binaries/opponents/maps/engine and every replay SHA were rechecked; final bodies were reconstructed by the unchanged independently audited R21 scanner. No game was launched and no frozen source was modified.", "", "One base82001 development screen; descriptive only, no significance test or p-value.", "", "| Version | Opponent | Wins | Initiated Head Trades | Hits After Two Steps | Initiated Queen Hits | Queen Hits After Two Steps |", "| --- | --- | ---: | ---: | ---: | ---: | ---: |"]
    for label in ("R10", "R22"):
        for op, s in summaries[label].items():
            c = s["hunting_counts"]
            lines.append(f"| {label} | {op} | {s['wins']}/34 | {c.get('our_initiated_head_trades',0)} | {c.get('head_hits_after_two_steps',0)} | {c.get('our_initiated_enemy_queen_hits',0)} | {c.get('enemy_queen_hits_after_two_steps',0)} |")
    lines += ["", "## First Actual Allocation", "", "| Evidence | Games |", "| --- | ---: |"] + [f"| {f} | {v} |" for f, v in metrics.items()]
    lines += ["", "Only five ordinary rear-allocation lines distinguish frozen R22 from frozen R10; shared helper/queen-feed/consolidation/config files are byte-identical. A strict same physical/SONAR prefix followed by actual SPLIT2 versus actual SPLIT(LEN-2) supports the first observed factor effect, not all later branch activations or terminal win causality.", "", "## Parent-LEN2 Geometry", ""]
    for label in ("R10", "R22"):
        gs = [g[label + "_geometry"] for g in games]
        geom, deaths = Counter(), Counter()
        for s in gs:
            geom.update(s["parent2_geometry"])
            deaths.update(s["parent2_actual_distinct_same_or_next_round_death_reasons"])
        lines.append(f"{label}: raw early LEN>=5 split pool{sum(s['raw_early_nonqueen_LEN5plus_split_pool_count'] for s in gs)}, parent2{sum(s['raw_pool_parent2_count'] for s in gs)}; actual geometry{dict(geom)}; distinct actual same/next-round death reasons{dict(deaths)}. Raw pools are not ordinary activation counts.")
        lines.append("")
    lines += ["## Round20 Matched Checkpoints", "", "| Opponent / Seat | Reaching20 | Mean Own Population Delta | Mean Own Total Delta | Mean Own Birth Delta |", "| --- | ---: | ---: | ---: | ---: |"]
    for op in ("V3_portal", "xxqztest"):
        for side in "AB":
            cps = [g["checkpoints"][20] for g in games if g["opponent"] == op and g["side"] == side and g["checkpoints"][20]["available"]]
            n = len(cps)
            means = [sum(c["own_R22_minus_R10"][f] for c in cps) / n if n else 0 for f in ("population", "total")]
            births = sum(c["own_births_R22_minus_R10"] for c in cps) / n if n else 0
            lines.append(f"| {op} / {side} | {n} | {means[0]:+.2f} | {means[1]:+.2f} | {births:+.2f} |")
    lines += ["", "## Paired Promotions And Regressions", ""]
    for op in ("V3_portal", "xxqztest"):
        for bucket in ("promotions", "regressions"):
            lines += [f"{op} {bucket}: {len(paired[op][bucket])}.", ""]
            for g in paired[op][bucket]:
                first = g["first_action_divergence"]
                f = f"round{first['R22']['round']} id{first['R22']['id']} {first['R10']['action']} -> {first['R22']['action']}, prefix={first['strict_physical_sonar_prefix_equal']}" if first and first["R10"] and first["R22"] else "no action difference"
                own = g["side"]
                l, r = g["R10_hunting"]["counts"], g["R22_hunting"]["counts"]
                lines.append(f"- {g['map']} / seed{g['seed']} / {own}: {g['R10_outcome']} -> {g['R22_outcome']}; {f}; own terminal queen/longest/total{tuple(g['R10_final'][own][k] for k in ('queen','longest','total'))} -> {tuple(g['R22_final'][own][k] for k in ('queen','longest','total'))}; initiated queen hits{l.get('our_initiated_enemy_queen_hits',0)} -> {r.get('our_initiated_enemy_queen_hits',0)}, actual >=3-step head hits{l.get('head_hits_after_two_steps',0)} -> {r.get('head_hits_after_two_steps',0)}.")
            lines.append("")
    lines += ["## Interpretation", "", "Observed head trades require actor and enemy actual head-death events in the same acting turn; the successful prefix and recorded final eligible collision step are preserved in JSON. The after-two-step columns restrict to nonqueens with actual ordinary-path collision at step>=3, not own-queen survival routes. Requested3/4-step paths are not automatically long-range hunts if the hit happens earlier. Generic free-harvest multi-step movements and enemy-initiated queen deaths remain separate. R22 leaves hunting source unchanged, so any count difference after the first allocation change is a downstream trajectory observation, not proof of a new hunting branch or direct winning mechanism.", "", "## Limits", ""] + ["- " + s for s in limits]
    with out_json.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    with out_md.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"passed": True, "metrics": metrics, "summaries": summaries,
                      "json": str(out_json), "md": str(out_md)}), flush=True)


if __name__ == "__main__":
    main()
