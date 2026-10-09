#!/usr/bin/env python3
"""Diagnose existing short-dragon terminal pickups; never run or edit bots."""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS, sha
from queen_immediate_witness import topology, landing, visible_cells, project
from r21_firstscreen_mechanism import action, canonical, update_digest

BASES = (82001, 83001, 84001)
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
LETTERS = "NESW"


def own_complete(body, view, edges, w, h):
    return len(set(body)) == len(body) and set(body) <= view and all(
        any(edges.get((rear, d), (0, -1))[0] == 0 and
            landing(rear, d, w, h) == front for d in range(4))
        for front, rear in zip(body, body[1:]))


def terminal_proof(body, view, edges, w, h):
    head = body[0]
    if head not in view:
        return None
    proof = []
    for d in range(4):
        kind = edges.get((head, d), (0, -1))[0]
        q = landing(head, d, w, h)
        if kind == 1:
            proof.append({"direction": LETTERS[d], "wall": True})
        elif kind == 0 and q in view and q in body:
            proof.append({"direction": LETTERS[d], "own_postbody": q})
        else:
            return None
    return proof


def alternatives(body, actual_dir, other, pearls, view, edges, w, h):
    rows = []
    for d in range(4):
        if d == actual_dir:
            continue
        first = project(body, [d], other, pearls, view, edges, w, h)
        if not first["known"] or not first["survives"]:
            continue
        for e in range(4):
            second = project(body, [d, e], other, pearls, view, edges, w, h)
            # This is a next-turn continuation, not a two-step same-turn
            # sprint. A length-two dragon cannot pay its second same-turn step.
            next_pearl = pearls - set(map(tuple, first["eaten"]))
            follow = project(first["body"], [e], other, next_pearl, view, edges, w, h)
            if not follow["known"] or not follow["survives"]:
                continue
            if terminal_proof(follow["body"], view, edges, w, h) is not None:
                continue
            rows.append({"action": LETTERS[d], "projected_body": first["body"],
                         "projected_length": len(first["body"]),
                         "next_turn_continuation": LETTERS[e],
                         "continuation_body": follow["body"],
                         "same_turn_two_step_legal": second["known"] and second["survives"]})
            break
    return rows


def geometry_checks():
    w, h = 9, 9
    body = [(4, 4), (3, 4)]
    view = visible_cells(body[0], w, h)
    edges = {((4, 5), d): (1, -1) for d in (1, 2, 3)}
    pearl = {(4, 5)}
    post = project(body, [2], set(), pearl, view, edges, w, h)
    assert own_complete(body, view, edges, w, h)
    assert post["survives"] and len(post["body"]) == 3
    assert terminal_proof(post["body"], view, edges, w, h)
    assert alternatives(body, 2, set(), pearl, view, edges, w, h)
    portals = dict(edges)
    portals[((4, 5), 1)] = (2, 4)
    assert terminal_proof(post["body"], view, portals, w, h) is None
    assert not own_complete(body, {body[0]}, edges, w, h)
    assert not own_complete([body[0], body[0]], view, edges, w, h)
    assert not alternatives(body, 2, {(4, 3), (5, 4)}, pearl, view, edges, w, h)
    return {"passed": True, "assertions": 7,
            "covered": ["known wall-own-body terminal", "ordinary alternate continuation",
                        "portal not terminal proof", "partial and duplicate own body rejection",
                        "occupied alternate rejection"]}


def scan(game):
    replay = Path(game["replay"])
    assert sha(replay) == game["replay_sha256"]
    root = Message(replay.read_bytes()).structure(0, 0)
    w, h, bodies, teams = parse_map(root.text(0))
    _, _, edges = topology(root.text(0))
    own = game["bot1_side"]
    queen = "AB".index(own)
    pearls = set()
    board = defaultdict(set)
    for did, body in bodies.items():
        for c in body:
            board[c].add(did)
    stream = hashlib.sha256()
    prefix_events = 0
    counts = Counter()
    findings = []
    follow = defaultdict(list)
    r, acting, turn_event, turn_action = -1, None, None, None
    logs, engine_logs = [], []

    def replace(did, new):
        for c in bodies.get(did, []):
            board[c].discard(did)
            if not board[c]:
                del board[c]
        bodies[did] = list(new)
        for c in new:
            board[c].add(did)

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            r = data.number(0)
        elif kind == 1:
            acting, turn_event = data.number(0), index
            assert acting in bodies
            logs, engine_logs, turn_action = [], [], None
        elif kind == 3:
            c = point(data.child(0))
            if data.number(0, "B") & 1:
                pearls.add(c)
            else:
                pearls.discard(c)
        elif kind == 5:
            engine_logs.append({"event": index, "text": data.text(0)})
        elif kind == 6:
            logs.append({"event": index, "text": data.text(0)})
        elif kind == 4:
            did = data.number(0)
            assert did == acting
            akind, requested, split_length = action(data)
            dirs = data.child(0).uint16s(0) if akind == 0 else []
            body = list(bodies[did])
            turn_action = {"round": r, "id": did, "action_event": index,
                           "turn_event": turn_event, "action": requested,
                           "body_before": body, "kind": akind}
            for record in follow[did]:
                if record["action_event"] < index and "next_own_action" not in record:
                    record["next_own_action"] = dict(turn_action)
            if teams[did] == own and did != queen and len(body) == 2 and akind == 0 and len(dirs) == 1:
                counts["nonqueen_LEN2_single_move_pool"] += 1
                d = dirs[0]
                q = landing(body[0], d, w, h)
                if edges.get((body[0], d), (0, -1))[0] == 0 and q in pearls and q not in board:
                    counts["ordinary_current_pearl_LEN2_single_move_pool"] += 1
                    view = visible_cells(body[0], w, h)
                    if own_complete(body, view, edges, w, h):
                        other = {c for c, owners in board.items() if c in view and any(o != did for o in owners)}
                        shown_pearls = pearls & view
                        post = project(body, dirs, other, shown_pearls, view, edges, w, h)
                        proof = terminal_proof(post["body"], view, edges, w, h) if post["known"] and post["survives"] else None
                        if proof is not None:
                            counts["strict_terminal_pickup_pool"] += 1
                            enemy_parts = [{"id": o, "cell": c} for c, owners in board.items()
                                           if c in view for o in owners if teams[o] != own]
                            alts = alternatives(body, d, other, shown_pearls, view, edges, w, h)
                            counts["strict_terminal_with_alternate"] += bool(alts)
                            counts["strict_terminal_quiet_with_alternate"] += bool(alts) and not enemy_parts
                            record = {**turn_action, "directions": list(dirs),
                                "length_before": len(body), "complete_visible_own_body": True,
                                "unique_ordinary_own_body": True,
                                "physical_sonar_action_prefix_sha256": stream.hexdigest(),
                                "physical_sonar_action_prefix_events": prefix_events,
                                "prefix_excludes_this_action": True,
                                "visible_cells": sorted(view), "visible_other_cells": sorted(other),
                                "visible_enemy_parts": enemy_parts, "visible_pearls": sorted(shown_pearls),
                                "current_pearl": q, "original_projection": post,
                                "terminal_proof": proof, "candidate_ordinary_alternatives": alts,
                                "quiet_with_alternate": bool(alts) and not enemy_parts,
                                "logs_before_action": list(logs), "actual_successful_updates": []}
                            findings.append(record)
                            follow[did].append(record)
        elif kind == 9:
            did, head, tail = data.number(0), point(data.child(0)), point(data.child(1))
            if r < 0:
                assert bodies[did][0] == head and bodies[did][-1] == tail
                counts["initial_repeated_position_events_skipped"] += 1
            else:
                new = [head] + list(bodies[did])
                while len(new) > 1 and new[-1] != tail:
                    new.pop()
                assert new[-1] == tail
                replace(did, new)
                for record in follow[did]:
                    if turn_action and record["action_event"] == turn_action["action_event"]:
                        record["actual_successful_updates"].append({"event": index, "body_after": list(new)})
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            parent_body = [point(p) for p in data.structs(0)]
            child_body = [point(p) for p in data.structs(1)]
            assert parent_body + list(reversed(child_body)) == bodies[parent]
            replace(parent, parent_body)
            replace(child, child_body)
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            did = data.number(0)
            death = {"event": index, "round": r, "acting_id": acting,
                     "reason": REASONS[data.number(4, "H")],
                     "body_before": list(bodies[did]), "engine_logs": list(engine_logs)}
            for record in follow[did]:
                record["actual_death"] = death
            replace(did, [])
            del bodies[did]
        item = canonical(event)
        if item is not None:
            update_digest(stream, item)
            prefix_events += 1
    final = {s: {"population": sum(teams[d] == s for d in bodies),
                 "total": sum(len(b) for d, b in bodies.items() if teams[d] == s),
                 "longest": max((len(b) for d, b in bodies.items() if teams[d] == s), default=0),
                 "queen": len(bodies.get("AB".index(s), []))} for s in "AB"}
    assert final == game["standings"]
    for record in findings:
        actual = record["actual_successful_updates"]
        record["actual_projection_exact_match"] = len(actual) == 1 and actual[0]["body_after"] == record["original_projection"]["body"]
        assert record["actual_projection_exact_match"]
        death = record.get("actual_death")
        record["actual_self_death_next_round"] = bool(death and death["round"] == record["round"] + 1 and death["reason"] == "self")
    return {"game": game["game"], "map": game["map"], "seed": int(game["seed"]),
            "side": own, "outcome": game["outcome"], "replay": str(replay),
            "replay_sha256": game["replay_sha256"], "final_standings_checked": True,
            "full_physical_sonar_action_sha256": stream.hexdigest(),
            "full_physical_sonar_action_events": prefix_events,
            "counts": dict(counts), "findings": findings}


def main():
    output, report = HERE / "h14_terminal_body_probe.json", HERE / "h14_terminal_body_probe.md"
    assert not output.exists() and not report.exists()
    checks = geometry_checks()
    games, provenance = [], []
    for base in BASES:
        run = RUNS / f"robust_h14_{base}"
        config = json.loads((run / "run.json").read_text())
        audit = json.loads((run / "verified_audit.json").read_text())
        assert config["unswbc"] == "1.2.7" and not config["sandbox"]
        assert config["sources"][0]["files"]["main.cpp"] == H14_MAIN
        assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
        selected = [g for g in audit["games"] if g["opponent"] == "xxqztest"]
        assert len(selected) == 34
        provenance.append({"base": base, "run_json_sha256": sha(run / "run.json"),
                           "audit_json_sha256": sha(run / "verified_audit.json"),
                           "engine_sha256": config["engine_sha256"], "H14_main_sha256": H14_MAIN})
        for game in sorted(selected, key=lambda g: int(g["game"])):
            games.append({"base": base, **scan(game)})
            print(f"terminal body scan {len(games)}/102", flush=True)
    events = [(g, f) for g in games for f in g["findings"]]
    quiet = [(g, f) for g, f in events if f["quiet_with_alternate"]]
    counts = Counter()
    for game in games:
        counts.update(game["counts"])
    covered = [g for g in games if any(f["quiet_with_alternate"] for f in g["findings"])]
    payload = {"passed": True, "scope": "102 audited H14/unchanged xx native1.2.7 dev games; no competitive run or bot edit",
               "script_sha256": sha(__file__), "bases": BASES, "geometry_checks": checks,
               "provenance": provenance, "aggregate_counts": dict(counts),
               "strict_terminal_events": len(events), "quiet_alternate_events": len(quiet),
               "quiet_alternate_games": len(covered),
               "quiet_alternate_loss_games": sum(g["outcome"] == "loss" for g in covered),
               "quiet_alternate_next_round_self_deaths": sum(f["actual_self_death_next_round"] for _, f in quiet),
               "games": games,
               "limitations": ["Current radius3 geometry reconstructs an observable witness, not private bot state or branch telemetry.",
                                "Prefix digests bind exact preceding official physical, SONAR and committed-action events, not a counterfactual candidate.",
                                "No other actor body is used as proof of permanent terminal blockage; walls and own post-body only.",
                                "Alternatives prove current ordinary movement and one next-turn local continuation, not future enemy/friendly survival.",
                                "LEN3 cannot split, but later intervening actors/resources can still change the board.",
                                "No win causality, held-out performance or production promotion is established."]}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    lines = ["# H14 Terminal Body Pickup Diagnosis", "", payload["scope"], "",
             "All three audited bases82001/83001/84001, ranked17 maps, A/B seats. Replay SHA and final standings rechecked; initial repeated position events are skipped. Each finding retains the exact official action event and physical/SONAR/action prefix digest, complete unique ordinary own LEN2 body, radius3 visible cells/pearls/occupancy and actual successful postbody.", "",
             f"Strict terminal LEN2 pearl pickups: {len(events)}; quiet with a proved ordinary alternate: {len(quiet)} in {len(covered)} games, including {payload['quiet_alternate_loss_games']} losses. Actual next-round self deaths among quiet alternate events: {payload['quiet_alternate_next_round_self_deaths']}/{len(quiet)}.", "",
             "| Base | Map | Seat | Outcome | Strict Terminal | Quiet Alternate | Next-Round Self Death |", "| --- | --- | --- | --- | ---: | ---: | ---: |"]
    for g in games:
        if g["findings"]:
            fs = [f for f in g["findings"] if f["quiet_with_alternate"]]
            lines.append(f"| {g['base']} | {g['map']} | {g['side']} | {g['outcome']} | {len(g['findings'])} | {len(fs)} | {sum(f['actual_self_death_next_round'] for f in fs)} |")
    lines += ["", "## Recommended Single Factor", "",
              "H14-child final ordinary short-pickup review: only nonqueen LEN2, one-step known ordinary pearl proposal, complete ordinary own body, no currently visible enemy part, proved wall/own-postbody terminal LEN3, and a proved ordinary replacement with local continuation. Directly select among reviewed alternatives using unchanged scoring/tie order; never re-run a generic penalty/fallback. Preserve portal/unknown movement, splits, donation, head attacks, queen decisions, free harvesting and protection. Keep the original if no proved replacement exists.", "",
              "This differs from R15's LEN4/5 pre-split terminal-pocket salvage and from R5's ordinary-split parent-continuation gate. It also has stronger actual-loss coverage than the strict portal-endpoint queen hypothesis, but sparse coverage cannot by itself predict a win-rate gain.", "",
              "## Minimal Validation", "",
              "Source-bound actual-input fixture plus H14 control for the earliest quiet loss witness; require exact original MOVE/pickup/LEN3 next-round self-death and an ordinary candidate choice with same input. Official1.2.7 action-tape must preserve every physical/SONAR/action event before the decision and compare original vs alternate, holding the next actor actions fixed. Controls: portal exit, incomplete body, enemy present, donation, queen, nonpearl, no alternate, safe pearl and all unchanged split branches. Only after observable action change and independent review, register one68-game82001 development screen; CPU-only about3-6minutes/fourworkers and0.4GB, no GPU/API or reserved seeds. Compare H14 per opponent/A/B and paired wins/losses, and measure mechanism activation, collector pickup/death and retained length separately.", "",
              "## Limits", "", *payload["limitations"], "", "Full evidence is in `h14_terminal_body_probe.json`. No production/opponent source or match schedule changed."]
    with report.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({k: payload[k] for k in ["passed", "strict_terminal_events", "quiet_alternate_events", "quiet_alternate_games", "quiet_alternate_loss_games", "quiet_alternate_next_round_self_deaths"]}))


if __name__ == "__main__":
    main()
