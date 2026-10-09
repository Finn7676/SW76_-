#!/usr/bin/env python3
"""Reconstruct successful portal crossings before round 80.

Map edge IDs use the official viewer boundary-grid format: each row contains
width+1 slots, alternating horizontal and vertical boundaries. Every ordinary
movement is checked against its requested direction and toroidal geometry.
Initial position notifications are not movements. A rapid repeat means the
same dragon crossed the same portal ID again within eight rounds; zero-growth
means no positive body-length change between those successful crossings. These
are descriptive proxies, not proof that a crossing was forced or unproductive.
"""
import argparse
import collections
import csv
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "protect_local_20260927"))
import analyze_replays as ar


def analyze(path, row, variant):
    root = ar.Message(path.read_bytes()).structure(0, 0)
    width, height, dragons, teams = ar.parse_map(root.text(0))
    portals, previous, directions = {}, {}, {}
    crossings, representative_moves = [], []
    growth = collections.Counter()
    round_num = -1
    for line in root.text(0).splitlines():
        words = line.split()
        if words and words[0] == "EDGE" and int(words[2]) == 2:
            edge, pid = int(words[1]), int(words[3])
            boundary_y, x = divmod(edge, width + 1)
            if boundary_y % 2 == 0:
                y = boundary_y // 2
                portals[((x, y % height), 0)] = pid
                portals[((x, (y - 1) % height), 2)] = pid
            else:
                y = (boundary_y - 1) // 2
                portals[((x % width, y), 3)] = pid
                portals[(((x - 1) % width, y), 1)] = pid
    for event in root.structs(3):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_num = data.number(0)
            if round_num >= 80:
                break
        elif kind == 4:
            ident, action = data.number(0), data.child(0)
            directions[ident] = list(action.uint16s(0)) if action.number(0, "H") == 0 else []
        elif kind == 9 and round_num >= 0:
            ident = data.number(0)
            head, tail = ar.point(data.child(0)), ar.point(data.child(1))
            body = dragons[ident]
            old_head, before = body[0], len(body)
            direction = directions[ident].pop(0)
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            growth[ident] += max(0, len(body) - before)
            pid = portals.get((old_head, direction))
            if pid is None:
                dx, dy = [(0, -1), (1, 0), (0, 1), (-1, 0)][direction]
                expected = ((old_head[0] + dx) % width, (old_head[1] + dy) % height)
                assert head == expected, (path, round_num, ident, old_head, head, direction)
            elif teams[ident] == row["bot1_side"]:
                prior = previous.get((ident, pid))
                crossings.append({"round": round_num, "id": ident, "pid": pid,
                                  "from": old_head, "to": head, "length": len(body),
                                  "rounds_since_same_portal": round_num - prior[0] if prior else None,
                                  "positive_growth_since_same_portal": growth[ident] - prior[1] if prior else None})
                previous[(ident, pid)] = (round_num, growth[ident])
            if (variant == "v2" and row["map"] == "default" and row["bot1_side"] == "B"
                    and ident == 25 and 30 <= round_num <= 45):
                representative_moves.append({"round": round_num, "id": ident,
                                             "direction": "NESW"[direction], "from": old_head,
                                             "to": head, "pid": pid, "length": len(body)})
        elif kind == 10:
            parent, child, team = data.number(0), data.number(4), "AB"[data.number(8, "H")]
            dragons[parent] = [ar.point(p) for p in data.structs(0)]
            dragons[child] = [ar.point(p) for p in data.structs(1)]
            teams[child] = team
        elif kind == 11:
            dragons.pop(data.number(0), None)
    repeats = [x for x in crossings if x["rounds_since_same_portal"] is not None]
    rapid = [x for x in repeats if x["rounds_since_same_portal"] <= 8]
    zero_growth = [x for x in rapid if x["positive_growth_since_same_portal"] == 0]
    return {"variant": variant, "map": row["map"], "side": row["bot1_side"],
            "seed": row["seed"], "outcome": row["outcome"], "replay": str(path.resolve()),
            "crossings": len(crossings), "repeats": len(repeats), "rapid8": len(rapid),
            "rapid8_zero_growth": len(zero_growth), "all_crossings": crossings,
            "representative_moves": representative_moves}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    games = []
    for variant in ["scout", "relay", "v2"]:
        folder = args.root / (variant + "_dev")
        for row in csv.DictReader((folder / "results.csv").open()):
            games.append(analyze(folder / row["replay"], row, variant))
        print(variant, "analyzed", flush=True)
    totals = {}
    for variant in ["scout", "relay", "v2"]:
        selected = [x for x in games if x["variant"] == variant]
        totals[variant] = {key: sum(x[key] for x in selected)
                           for key in ["crossings", "repeats", "rapid8", "rapid8_zero_growth"]}
    payload = {"definition": __doc__, "totals": totals, "games": games}
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps(totals), flush=True)


if __name__ == "__main__":
    main()
