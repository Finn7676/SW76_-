#!/usr/bin/env python3
"""Decode H5 official replay logs and directly observed donated-pearl pickup."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from analyze import Message, parse_map, point, REASONS


def scan(game):
    path = Path(game["replay"])
    root = Message(path.read_bytes()).structure(0, 0)
    _, _, dragons, teams = parse_map(root.text(0))
    entries, pending, sources = [], {}, {}
    round_index, current_turn, death_source = -1, None, None
    removed = []
    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            current_turn = data.number(0)
            death_source, removed = None, []
        elif kind == 6:
            text = data.text(0)
            words = text.split()
            if "CONSOLIDATE_FEED" not in words:
                continue
            donor = data.number(0)
            receiver = int(words[words.index("receiver") + 1])
            item = {
                "game": game["game"], "opponent": game["opponent"],
                "map": game["map"], "side": game["bot1_side"],
                "replay": str(path), "replay_sha256": game["replay_sha256"],
                "round_index": round_index, "log_event_index": event_index,
                "donor": donor, "receiver": receiver, "log": text,
                "donor_body": list(dragons.get(donor, [])),
                "donor_team": teams.get(donor), "receiver_team": teams.get(receiver),
                "receiver_length_at_log": len(dragons.get(receiver, [])),
                "donor_died": False, "pearl_cells_created": [], "collections": [],
            }
            entries.append(item)
            pending[donor] = item
        elif kind == 9:
            dragon_id = data.number(0)
            body = dragons[dragon_id]
            head, tail = point(data.child(0)), point(data.child(1))
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            for tile, source, removal_index in removed:
                if current_turn == dragon_id and tile == head:
                    source["collections"].append({
                        "round_index": round_index, "collector": dragon_id,
                        "collector_team": teams[dragon_id], "tile": tile,
                        "collector_length_after_update": len(body),
                        "is_intended_receiver": dragon_id == source["receiver"],
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
            dragon_id = data.number(0)
            if dragon_id in pending:
                death_source = pending[dragon_id]
                death_source["donor_died"] = True
                death_source["death_reason"] = REASONS[data.number(4, "H")]
                death_source["death_round_index"] = round_index
                death_source["death_event_index"] = event_index
                death_source["receiver_alive_at_donor_death"] = death_source["receiver"] in dragons
            dragons.pop(dragon_id, None)
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
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", type=Path, default=HERE / "queen_consolidate_v127")
    parser.add_argument("--output", type=Path, default=HERE / "consolidate_mechanism.json")
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Output exists; choose a new path to preserve evidence")
    games = json.loads((args.suite / "verified_audit.json").read_text())["games"]
    entries = []
    for index, game in enumerate(games):
        result = scan(game)
        entries.extend(result)
        print(f"{index + 1}/{len(games)} game {game['game']}: {len(result)} feed logs", flush=True)
    collections = [c for entry in entries for c in entry["collections"]]
    receiver = [c for c in collections if c["is_intended_receiver"]]
    summary = {
        "games_checked": len(games), "consolidate_feed_logs": len(entries),
        "games_with_feed": len({e["game"] for e in entries}),
        "donors_that_died": sum(e["donor_died"] for e in entries),
        "donor_death_reasons": dict(Counter(e.get("death_reason", "no_death") for e in entries)),
        "observed_created_donated_pearl_cells": sum(len(e["pearl_cells_created"]) for e in entries),
        "observed_created_donor_head_pearls": sum(any(c["is_donor_head"] for c in e["pearl_cells_created"]) for e in entries),
        "intended_receiver_pearl_collections": len(receiver),
        "intended_receiver_donor_head_collections": sum(c["is_donor_head"] for c in receiver),
        "feeds_with_intended_receiver_collection": sum(any(c["is_intended_receiver"] for c in e["collections"]) for e in entries),
        "games_with_intended_receiver_collection": len({e["game"] for e in entries if any(c["is_intended_receiver"] for c in e["collections"])}),
        "other_friendly_pearl_collections": sum(c["collector_team"] == e["donor_team"] and not c["is_intended_receiver"] for e in entries for c in e["collections"]),
        "enemy_pearl_collections": sum(c["collector_team"] != e["donor_team"] for e in entries for c in e["collections"]),
    }
    output = {
        "summary": summary, "entries": entries,
        "method": "CONSOLIDATE_FEED is decoded from official dragon-log events (kind 6). A donated pearl requires a present tile-change on the donor's predeath body within that death turn. Pickup requires a later absent tile-change and same-turn collector movement to that tile. This is a directly observed conservative subset; no whole-body collection is assumed.",
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    }
    with args.output.open("x") as handle:
        json.dump(output, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"output": str(args.output), **summary}, indent=2))


if __name__ == "__main__":
    main()
