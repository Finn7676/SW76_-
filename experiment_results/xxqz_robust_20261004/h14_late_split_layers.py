#!/usr/bin/env python3
"""Official late split stratification on the three fixed H14 development bases."""
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


def scan(game):
    packed = Path(game["replay"]).read_bytes()
    assert hashlib.sha256(packed).hexdigest() == game["replay_sha256"]
    root = Message(packed).structure(0, 0)
    width, height, dragons, teams = parse_map(root.text(0))
    side, board = game["bot1_side"], {}
    population = Counter(teams.values())
    first, last = {}, {}
    splits, counts = [], Counter()
    round_index, logs = -1, []

    def update(dragon, old, new):
        for c in old:
            if c in board:
                board[c].discard(dragon)
                if not board[c]:
                    board.pop(c)
        for c in new:
            board.setdefault(c, set()).add(dragon)

    for dragon, body in dragons.items():
        update(dragon, [], body)

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            logs = []
        elif kind == 6:
            logs.append(data.text(0))
        elif kind == 4:
            dragon, action = data.number(0), data.child(0)
            if teams[dragon] != side or dragon in (0, 1):
                continue
            head = dragons[dragon][0]
            local = {d for dx in range(-3, 4) for dy in range(-3, 4)
                for d in board.get(((head[0] + dx) % width, (head[1] + dy) % height), set())}
            enemy = sorted(d for d in local if teams[d] != side)
            queen_visible = "AB".index(side) in local
            first.setdefault(dragon, round_index)
            last.setdefault(dragon, round_index)
            if enemy:
                last[dragon] = round_index
            if not (350 <= round_index < 470 and population[side] >= 8
                    and action.number(0, "H") == 1 and action.number(4) == 2
                    and not any("PROTECT_DIRECT" in log for log in logs)):
                continue
            item = {"round": round_index, "id": dragon, "event_index": index,
                "length": len(dragons[dragon]), "head": head, "body": list(dragons[dragon]),
                "population": population[side], "enemy_parts": enemy,
                "queen_part_visible": queen_visible, "first_observe": first[dragon],
                "last_visible_enemy": last[dragon], "logs": list(logs),
                "champion": dragon % 8 == 0 or len(dragons[dragon]) >= 12}
            splits.append(item)
            counts["split2_before470_pop8_excludeprotect"] += 1
            counts["queen_visible" if queen_visible else "queen_not_visible"] += 1
            counts["current_enemy_visible" if enemy else "no_current_visible_enemy"] += 1
            if not enemy:
                counts["no_enemy_queen_visible" if queen_visible else "no_enemy_no_queen"] += 1
                counts["no_enemy_length" + str(item["length"])] += 1
                counts["no_enemy_champion" if item["champion"] else "no_enemy_nonchampion"] += 1
                if round_index - last[dragon] >= 8:
                    counts["no_enemy_last_visible_atleast8"] += 1
                if round_index - first[dragon] >= 8:
                    counts["no_enemy_age_atleast8"] += 1
                if (not queen_visible and round_index - last[dragon] >= 8
                        and round_index - first[dragon] >= 8):
                    counts["viewquiet8_noqueen"] += 1
        elif kind == 9:
            dragon = data.number(0)
            old = list(dragons[dragon])
            head, tail = point(data.child(0)), point(data.child(1))
            body = dragons[dragon]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            update(dragon, old, body)
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            old = list(dragons[parent])
            pb, cb = [point(p) for p in data.structs(0)], [point(p) for p in data.structs(1)]
            update(parent, old, pb)
            update(child, [], cb)
            dragons[parent], dragons[child] = pb, cb
            teams[child] = "AB"[data.number(8, "H")]
            population[teams[child]] += 1
        elif kind == 11:
            dragon = data.number(0)
            update(dragon, dragons[dragon], [])
            population[teams[dragon]] -= 1
            dragons.pop(dragon)
        elif kind > 12:
            raise ValueError(f"Unsupported event {kind}")
    actual = {s: {"population": sum(teams[d] == s for d in dragons),
        "longest": max((len(b) for d, b in dragons.items() if teams[d] == s), default=0),
        "total": sum(len(b) for d, b in dragons.items() if teams[d] == s),
        "queen": len(dragons.get("AB".index(s), []))} for s in "AB"}
    assert actual == game["standings"]
    return {"game": game["game"], "map": game["map"], "seed": game["seed"],
        "side": side, "outcome": game["outcome"], "replay": game["replay"],
        "replay_sha256": game["replay_sha256"], "counts": dict(counts), "splits": splits}


def total(games):
    c = Counter()
    for game in games:
        c.update(game["counts"])
    return dict(c)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    output = HERE / "h14_late_split_layers.json"
    assert not output.exists() and not output.with_suffix(".md").exists()
    games = []
    for seed in (82001, 83001, 84001):
        label = f"robust_h14_{seed}"
        config = json.loads((RUNS / label / "run.json").read_text())
        audit = json.loads((RUNS / label / "verified_audit.json").read_text())
        assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
        assert config["unswbc"] == "1.2.7" and config["base_seed"] == seed
        assert config["sources"][0]["files"]["main.cpp"] == "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
        for game in sorted(audit["games"], key=lambda g: int(g["game"])):
            if game["opponent"] != "xxqztest":
                continue
            games.append({"label": label, **scan(game)})
            if len(games) % 8 == 0:
                print(f"layers {len(games)}/102", flush=True)
    by_map = {m: total([g for g in games if g["map"] == m]) for m in sorted({g["map"] for g in games})}
    method = ("102 audited H14/xx development games; replaySHA/final standings verified. "
        "Recorded nonqueen split2 actions require350<=r<470, POP>=8 and no PROTECT_DIRECT log. "
        "This is not exact routine-branch instrumentation: emergency fallback split2 and champions "
        "are separately visible, not silently labeled growth. Wrapped7x7 visible-enemy history "
        "ignores sonar; quiet8 is an upper bound. No hidden info is supplied to a bot.")
    payload = {"method": method, "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "summary": total(games), "by_map": by_map, "games": games}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    lines = ["# H14 Late Split Layers", "", method, "", "Three development bases, descriptive counts only; no significance test.", "",
        "| Map | Split2 Pool | Current Enemy | No Enemy | No Enemy/Queen Visible | Quiet8/No Queen |",
        "|---|---:|---:|---:|---:|---:|"]
    for m, c in by_map.items():
        lines.append(f'| {m} | {c.get("split2_before470_pop8_excludeprotect", 0)} | '
            f'{c.get("current_enemy_visible", 0)} | {c.get("no_current_visible_enemy", 0)} | '
            f'{c.get("no_enemy_queen_visible", 0)} | {c.get("viewquiet8_noqueen", 0)} |')
    lines += ["", "## Portals Per Game", "",
        "| Base | Side | Outcome | Split2 Pool | Current Enemy | No Enemy | Quiet8/No Queen |",
        "|---|---|---|---:|---:|---:|---:|"]
    for g in games:
        if g["map"] != "portals":
            continue
        c = g["counts"]
        lines.append(f'| {g["label"]} | {g["side"]} | {g["outcome"]} | '
            f'{c.get("split2_before470_pop8_excludeprotect", 0)} | '
            f'{c.get("current_enemy_visible", 0)} | {c.get("no_current_visible_enemy", 0)} | '
            f'{c.get("viewquiet8_noqueen", 0)} |')
    with output.with_suffix(".md").open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps(payload["summary"]), flush=True)


if __name__ == "__main__":
    main()
