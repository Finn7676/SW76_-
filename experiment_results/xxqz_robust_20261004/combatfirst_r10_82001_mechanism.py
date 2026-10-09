#!/usr/bin/env python3
"""Read-only replay diagnosis of the audited 82001 H14/R4/R10 development batch."""
from collections import Counter, deque
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS

LABELS = ("robust_h14_82001", "robust_sprint4_82001", "robust_queen_hunt_82001_retry1")
EXPECTED_MAIN = (
    "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc",
    "455fbc4744a2634a9c529a61f6b4ee582d7d6da0ceb85d2854b81aba39b60675",
    "de27f3b2abb1440d75395c9676eb4d80967947bd7adb7e66f9570005f24ca8ba",
)


def key(game):
    return game["opponent"], game["map"], str(game["seed"]), game["side"]


def scan(game):
    packed = Path(game["replay"]).read_bytes()
    assert hashlib.sha256(packed).hexdigest() == game["replay_sha256"]
    root = Message(packed).structure(0, 0)
    _, _, dragons, teams = parse_map(root.text(0))
    side = game["bot1_side"]
    active = None
    round_index = -1
    history = deque(maxlen=10)
    own_actions = Counter()
    head_trades, queen_deaths, actions = [], [], []

    def finish():
        if active is None:
            return
        history.append(dict(active))
        if active["side"] != side:
            return
        own_actions[f'kind{active["kind"]}'] += 1
        if active["kind"] == 0:
            own_actions[f'steps{len(active["dirs"])}'] += 1
        actor_head_death = any(d["id"] == active["id"] and d["reason"] == "head"
                               for d in active["deaths"])
        victims = [d for d in active["deaths"] if d["side"] != side and d["reason"] == "head"]
        if actor_head_death and victims:
            own_death = next(d for d in active["deaths"] if d["id"] == active["id"])
            free_steps = (active["initial_length"] + 3) // 4
            legal = active["kind"] == 0 and active["successful_steps"] < len(active["dirs"]) and (
                active["successful_steps"] < free_steps or own_death["length_before_death"] > 2)
            assert legal
            head_trades.append({**active, "recorded_final_step_eligible": legal,
                "enemy_queen_hit": any(d["id"] in (0, 1) for d in victims)})

    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            finish()
            active = None
        elif kind == 4:
            dragon, action = data.number(0), data.child(0)
            action_kind = action.number(0, "H")
            dirs = action.uint16s(0) if action_kind == 0 else []
            active = {"round_index": round_index, "action_event_index": index,
                      "id": dragon, "side": teams[dragon], "kind": action_kind,
                      "dirs": dirs, "initial_length": len(dragons[dragon]),
                      "head_before_action": dragons[dragon][0], "successful_steps": 0,
                      "deaths": []}
            actions.append((round_index, dragon, action_kind, tuple(dirs), len(dragons[dragon])))
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
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            dragons[parent] = [point(p) for p in data.structs(0)]
            dragons[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
        elif kind == 11:
            dragon, reason = data.number(0), REASONS[data.number(4, "H")]
            death = {"id": dragon, "side": teams[dragon], "reason": reason,
                     "length_before_death": len(dragons[dragon]),
                     "head_before_death": dragons[dragon][0], "death_event_index": index}
            if active is not None:
                active["deaths"].append(death)
            if dragon in (0, 1):
                queen_deaths.append({**death, "round_index": round_index,
                    "active_actor": dict(active) if active is not None else None,
                    "last_ten_completed_actions": list(history)})
            dragons.pop(dragon)
        elif kind > 12:
            raise ValueError(f"Unknown replay kind {kind}")
    finish()
    for death in queen_deaths:
        matches = [t for t in head_trades if death["side"] != side and
                   any(d["id"] == death["id"] for d in t["deaths"])]
        death["our_recorded_queen_hit"] = bool(matches)
        if matches:
            death["recorded_hit"] = matches[0]
    return {"game": game["game"], "opponent": game["opponent"], "map": game["map"],
            "seed": game["seed"], "side": side, "outcome": game["outcome"],
            "replay": game["replay"], "replay_sha256": game["replay_sha256"],
            "standings": game["standings"], "queen_deaths": queen_deaths,
            "our_action_counts": dict(own_actions), "our_head_trades": head_trades,
            "_actions": actions}


def summary(games):
    trades = [t for g in games for t in g["our_head_trades"]]
    queens = [d for g in games for d in g["queen_deaths"] if d["side"] != g["side"]]
    counts = Counter()
    for g in games:
        counts.update(g["our_action_counts"])
    return {"games": len(games), "wins": sum(g["outcome"] == "win" for g in games),
            "our_action_counts": dict(counts), "our_recorded_head_trades": len(trades),
            "our_recorded_head_trades_by_requested_steps": dict(Counter(len(t["dirs"]) for t in trades)),
            "our_recorded_enemy_queen_hits": sum(t["enemy_queen_hit"] for t in trades),
            "our_recorded_enemy_queen_hits_by_requested_steps": dict(Counter(len(t["dirs"]) for t in trades if t["enemy_queen_hit"])),
            "enemy_queen_death_reasons": dict(Counter(d["reason"] for d in queens)),
            "enemy_queen_deaths_not_our_recorded_hit": dict(Counter(d["reason"] for d in queens if not d["our_recorded_queen_hit"]))}


def comparison(left, right):
    a = {key(g): g for g in left}
    b = {key(g): g for g in right}
    assert a.keys() == b.keys()
    changes = []
    for k in sorted(a):
        l, r = a[k], b[k]
        if l["outcome"] == r["outcome"]:
            continue
        divergence = next(((x, y) for x, y in zip(l["_actions"], r["_actions"]) if x != y), None)
        changes.append({"opponent": k[0], "map": k[1], "seed": k[2], "side": k[3],
            "baseline": l["outcome"], "candidate": r["outcome"],
            "baseline_standings": l["standings"], "candidate_standings": r["standings"],
            "baseline_queen_deaths": l["queen_deaths"], "candidate_queen_deaths": r["queen_deaths"],
            "first_action_signature_difference": divergence,
            "baseline_replay": l["replay"], "candidate_replay": r["replay"]})
    return changes


def main():
    output = HERE / "combatfirst_r10_82001_mechanism.json"
    markdown = output.with_suffix(".md")
    assert not output.exists() and not markdown.exists()
    runs, reference = {}, None
    for label, digest in zip(LABELS, EXPECTED_MAIN):
        directory = RUNS / label
        audit = json.loads((directory / "verified_audit.json").read_text())
        config = json.loads((directory / "run.json").read_text())
        assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
        assert config["base_seed"] == 82001 and config["unswbc"] == "1.2.7" and not config["sandbox"]
        assert config["sources"][0]["files"]["main.cpp"] == digest
        control = {"engine": config["engine_sha256"], "engine_modules": config["engine_snapshot_files"],
                   "schedule": config["schedule"],
                   "maps": [(m["name"], m["sha256"], m["seed"]) for m in config["maps"]],
                   "opponents": [{k: v for k, v in source["files"].items() if not k.startswith(".unswbc-build/")}
                                 for source in config["sources"][1:]]}
        if reference is None:
            reference = control
        else:
            assert reference == control
        games = []
        for game in sorted(audit["games"], key=lambda g: int(g["game"])):
            games.append(scan(game))
            if len(games) % 8 == 0:
                print(f"{label}: {len(games)}/68", flush=True)
        runs[label] = games
    comparisons = {"R10_vs_R4": comparison(runs[LABELS[1]], runs[LABELS[2]]),
                   "R10_vs_H14": comparison(runs[LABELS[0]], runs[LABELS[2]])}
    summaries = {label: {o: summary([g for g in games if g["opponent"] == o])
                         for o in ("V3_portal", "xxqztest")} for label, games in runs.items()}
    for games in runs.values():
        for game in games:
            game.pop("_actions")
    payload = {"scope": "Completed audited 82001 development only; no reserved seeds read",
               "method": "Official replay actions/deaths: our head trade requires our active actor and enemy both dying for head in that same turn. An enemy queen dying for head during its own move is not our initiated queen attack. Legality of a recorded hit is supported by successful prefix movement and official head death, not an inferred target or hidden-board input. Temporal association is not proof of winning causality.",
               "labels": LABELS, "source_main_sha256": dict(zip(LABELS, EXPECTED_MAIN)),
               "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "summaries": summaries, "comparisons": comparisons, "runs": runs}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    lines = ["# R10 82001 Queen-Hit Mechanism", "", payload["scope"], "", payload["method"], "",
             "| Version | Opponent | Wins | Our Recorded Head Trades | Our Initiated Queen Hits | Enemy Queen Death Reasons |",
             "|---|---|---:|---:|---:|---|"]
    for label, opponents in summaries.items():
        for opponent, item in opponents.items():
            lines.append(f'| {label} | {opponent} | {item["wins"]}/34 | {item["our_recorded_head_trades"]} | {item["our_recorded_enemy_queen_hits"]} | {json.dumps(item["enemy_queen_death_reasons"], sort_keys=True)} |')
    for name, changes in comparisons.items():
        lines += ["", "## " + name, ""]
        for change in changes:
            lines.append(f'- {change["opponent"]}, {change["map"]}, {change["side"]}, seed {change["seed"]}: {change["baseline"]} -> {change["candidate"]}; first action difference {change["first_action_signature_difference"]}.')
            for prefix in ("baseline", "candidate"):
                deaths = change[prefix + "_queen_deaths"]
                concise = [{"id": d["id"], "round": d["round_index"], "reason": d["reason"],
                            "our_hit": d["our_recorded_queen_hit"]} for d in deaths]
                lines.append(f'  {prefix}: {json.dumps(concise)}; standings {json.dumps(change[prefix + "_standings"])}.')
    with markdown.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"output": str(output), "summary": summaries}), flush=True)


if __name__ == "__main__":
    main()
