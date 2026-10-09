#!/usr/bin/env python3
"""Replay-only sprint and directly observed donation mechanisms for H14/R3/R4."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS


def bucket(round_index):
    return f"{100 * (round_index // 100)}-{100 * (round_index // 100) + 99}"


def scan(game):
    replay = Path(game["replay"])
    packed = replay.read_bytes()
    assert hashlib.sha256(packed).hexdigest() == game["replay_sha256"]
    root = Message(packed).structure(0, 0)
    width, height, dragons, teams = parse_map(root.text(0))
    side = game["bot1_side"]
    actions, sprints, feeds = Counter(), [], []
    pending, sources, route_history = {}, {}, {}
    round_index, turn, active, death_source = -1, None, None, None
    removed, route_logs, route_donors = [], Counter(), set()

    def finish():
        if active is None or active["side"] != side:
            return
        actions["all_actions"] += 1
        actions[f'action_kind_{active["kind"]}'] += 1
        if active["kind"] == 0:
            actions[f'move_request_{active["requested_steps"]}'] += 1
            if active["requested_steps"] >= 2:
                active["enemy_head_hit"] = active["own_death"] == "head" and any(
                    d["side"] != side and d["reason"] == "head" for d in active["deaths"])
                active["enemy_queen_hit"] = active["enemy_head_hit"] and any(
                    d["id"] in (0, 1) and d["side"] != side for d in active["deaths"])
                active["actor_body_death"] = active["own_death"] == "body"
                sprints.append(active)

    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            finish()
            turn, active = data.number(0), None
            death_source, removed = None, []
        elif kind == 4:
            dragon = data.number(0)
            assert dragon == turn and active is None
            action = data.child(0)
            action_kind = action.number(0, "H")
            dirs = action.uint16s(0) if action_kind == 0 else []
            assert action_kind in (0, 1, 2)
            assert all(0 <= d < 4 for d in dirs)
            active = {
                "round_index": round_index, "action_event_index": event_index,
                "id": dragon, "side": teams[dragon], "kind": action_kind,
                "directions": dirs, "requested_steps": len(dirs),
                "length_before_action": len(dragons[dragon]),
                "queen": dragon in (0, 1), "successful_steps": 0,
                "own_death": None, "deaths": [],
            }
        elif kind == 6:
            text, donor = data.text(0), data.number(0)
            words = text.split()
            if teams.get(donor) != side:
                continue
            if "DONOR_ROUTE" in words:
                receiver = int(words[words.index("receiver") + 1])
                history = route_history.setdefault(donor, {"count": 0, "first_round": round_index})
                history.update(last_round=round_index, receiver=receiver)
                history["count"] += 1
                route_logs[bucket(round_index)] += 1
                route_donors.add(donor)
            tag = next((t for t in ("CONSOLIDATE_FEED", "QUEEN_FEED") if t in words), None)
            if tag is None:
                continue
            receiver = int(words[words.index("receiver" if tag == "CONSOLIDATE_FEED" else "queen") + 1])
            history = route_history.get(donor, {})
            item = {
                "round_index": round_index, "log_event_index": event_index,
                "tag": tag, "donor": donor, "receiver": receiver, "log": text,
                "donor_body": list(dragons.get(donor, [])),
                "donor_team": teams[donor], "receiver_team": teams.get(receiver),
                "donor_length_at_log": len(dragons.get(donor, [])),
                "receiver_length_at_log": len(dragons.get(receiver, [])),
                "donor_died": False, "pearl_cells_created": [], "collections": [],
                "prior_route": dict(history),
                "same_receiver_route_within_20_rounds": bool(history and
                    history["receiver"] == receiver and round_index - history["last_round"] <= 20),
            }
            assert donor not in pending
            feeds.append(item)
            pending[donor] = item
        elif kind == 9:
            dragon = data.number(0)
            body = dragons[dragon]
            head, tail = point(data.child(0)), point(data.child(1))
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            if active is not None and active["id"] == dragon:
                active["successful_steps"] += 1
            for tile, source, removal_index in removed:
                if turn == dragon and tile == head:
                    source["collections"].append({
                        "round_index": round_index, "collector": dragon,
                        "collector_team": teams[dragon], "tile": tile,
                        "collector_length_after_update": len(body),
                        "is_intended_receiver": dragon == source["receiver"],
                        "is_donor_head": tile == source["donor_body"][0],
                        "pearl_removal_event_index": removal_index,
                        "movement_event_index": event_index,
                    })
            removed = []
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            dragons[parent] = [point(p) for p in data.structs(0)]
            dragons[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            dragon, reason = data.number(0), REASONS[data.number(4, "H")]
            if active is not None:
                active["deaths"].append({"id": dragon, "side": teams[dragon],
                                         "reason": reason, "length": len(dragons[dragon])})
                if active["id"] == dragon:
                    active["own_death"] = reason
            death_source = pending.get(dragon)
            if death_source is not None:
                death_source["donor_died"] = True
                death_source["death_reason"] = reason
                death_source["death_round_index"] = round_index
                death_source["death_event_index"] = event_index
                death_source["receiver_alive_at_donor_death"] = death_source["receiver"] in dragons
            dragons.pop(dragon)
        elif kind == 3:
            tile, present = point(data.child(0)), bool(data.number(0, "B") & 1)
            if present:
                sources.pop(tile, None)
                if death_source is not None and tile in death_source["donor_body"]:
                    death_source["pearl_cells_created"].append({
                        "tile": tile, "event_index": event_index,
                        "is_donor_head": tile == death_source["donor_body"][0],
                    })
                    sources[tile] = death_source
            elif tile in sources:
                removed.append((tile, sources.pop(tile), event_index))
        elif kind > 12:
            raise ValueError(f"Unknown official event {kind}")
    finish()
    return {
        "game": game["game"], "opponent": game["opponent"], "map": game["map"],
        "seed": game["seed"], "side": side, "outcome": game["outcome"],
        "replay": str(replay), "replay_sha256": game["replay_sha256"],
        "map_size": [width, height], "actions": dict(actions), "sprints": sprints,
        "route_logs": dict(route_logs), "unique_route_donors": len(route_donors), "feeds": feeds,
    }


def feed_summary(feeds):
    collections = [c for f in feeds for c in f["collections"]]
    receiver = [c for c in collections if c["is_intended_receiver"]]
    return {
        "feed_logs": len(feeds), "donors_died": sum(f["donor_died"] for f in feeds),
        "death_reasons": dict(Counter(f.get("death_reason", "no_death") for f in feeds)),
        "created_pearl_cells": sum(len(f["pearl_cells_created"]) for f in feeds),
        "created_head_pearls": sum(any(c["is_donor_head"] for c in f["pearl_cells_created"]) for f in feeds),
        "intended_receiver_collections": len(receiver),
        "intended_receiver_head_collections": sum(c["is_donor_head"] for c in receiver),
        "feeds_with_receiver_collection": sum(any(c["is_intended_receiver"] for c in f["collections"]) for f in feeds),
        "other_friendly_collections": sum(c["collector_team"] == f["donor_team"] and
            not c["is_intended_receiver"] for f in feeds for c in f["collections"]),
        "enemy_collections": sum(c["collector_team"] != f["donor_team"]
            for f in feeds for c in f["collections"]),
        "feed_round_buckets": dict(Counter(bucket(f["round_index"]) for f in feeds)),
    }


def sprint_summary(sprints):
    return {
        "requests": len(sprints),
        "request_lengths": dict(Counter(s["requested_steps"] for s in sprints)),
        "initial_lengths": dict(Counter(s["length_before_action"] for s in sprints)),
        "queen_requests": sum(s["queen"] for s in sprints),
        "successful_steps": sum(s["successful_steps"] for s in sprints),
        "enemy_head_hits": sum(s["enemy_head_hit"] for s in sprints),
        "enemy_queen_hits": sum(s["enemy_queen_hit"] for s in sprints),
        "body_deaths": sum(s["actor_body_death"] for s in sprints),
        "death_reasons": dict(Counter(s["own_death"] or "survived" for s in sprints)),
        "round_buckets": dict(Counter(bucket(s["round_index"]) for s in sprints)),
        "head_hit_round_buckets": dict(Counter(bucket(s["round_index"]) for s in sprints if s["enemy_head_hit"])),
        "head_hit_request_lengths": dict(Counter(s["requested_steps"] for s in sprints if s["enemy_head_hit"])),
        "head_hit_initial_lengths": dict(Counter(s["length_before_action"] for s in sprints if s["enemy_head_hit"])),
    }


def summarize(games):
    sprints = [s for g in games for s in g["sprints"]]
    feeds = [f for g in games for f in g["feeds"]]
    return {
        "games": len(games), "sprints_ge2": sprint_summary(sprints),
        "sprints_ge3": sprint_summary([s for s in sprints if s["requested_steps"] >= 3]),
        "consolidate": feed_summary([f for f in feeds if f["tag"] == "CONSOLIDATE_FEED"]),
        "queen_feed": feed_summary([f for f in feeds if f["tag"] == "QUEEN_FEED"]),
        "route_associated_feeds": feed_summary([f for f in feeds if f["same_receiver_route_within_20_rounds"]]),
        "route_logs": sum(sum(g["route_logs"].values()) for g in games),
        "unique_route_donor_game_pairs": sum(g["unique_route_donors"] for g in games),
        "games_with_route": sum(bool(g["route_logs"]) for g in games),
        "games_with_consolidate": sum(any(f["tag"] == "CONSOLIDATE_FEED" for f in g["feeds"]) for g in games),
        "games_with_receiver_collection": sum(any(any(c["is_intended_receiver"] for c in f["collections"])
            for f in g["feeds"]) for g in games),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", action="append", required=True)
    parser.add_argument("--output-dir", type=Path, default=HERE / "mechanisms")
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for label in args.label:
        output = args.output_dir / f"{label}.json"
        if output.exists():
            parser.error(f"Preserve existing output: {output}")
        suite = RUNS / label
        audit = json.loads((suite / "verified_audit.json").read_text())
        config = json.loads((suite / "run.json").read_text())
        assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
        assert config["unswbc"] == "1.2.7" and not config["sandbox"]
        games = []
        for index, game in enumerate(sorted(audit["games"], key=lambda g: int(g["game"]))):
            games.append(scan(game))
            if index % 8 == 7:
                print(f"{label}: {index + 1}/68", flush=True)
        payload = {
            "label": label, "candidate": config["candidate"], "base_seed": config["base_seed"],
            "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "summary": summarize(games),
            "by_opponent": {o: summarize([g for g in games if g["opponent"] == o])
                            for o in ("V3_portal", "xxqztest")},
            "games": games,
            "method": "Official viewer schema: event 4 DragonAction, action move 0 contains the uint16 direction list; event 11 death reason 3 is head and 2 body. Enemy head hit is actor head death plus other-team head death in the same turn. Body death is the official reason during the actor's own move request; hit-body owner is not invented. Sprint request >=3 is not equivalent to proactive ram because freeHarvest and queen survival can also emit multi-step moves. Donated pearls require a present tile change on the donor's predeath body followed by absence and same-turn collector movement at that tile. Route association is the same donor/receiver with a DONOR_ROUTE log within 20 previous rounds; association is not causal attribution.",
        }
        with output.open("x") as handle:
            json.dump(payload, handle, indent=2)
            handle.write("\n")
        print(json.dumps({"output": str(output), "summary": payload["summary"]}), flush=True)


if __name__ == "__main__":
    main()
