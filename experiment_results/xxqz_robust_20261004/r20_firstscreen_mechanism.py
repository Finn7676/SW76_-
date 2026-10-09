#!/usr/bin/env python3
"""Read frozen/audited R20 and paired H14 replays; no matches or bot edits."""
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, sha, tree_hashes
from queen_immediate_engine import canonical, action

R20 = "robust_queen_witness_82001"
H14 = "robust_h14_82001"
R20_MAIN = "c7682278e8f70609b8edf44423273ec65df4c4729ece4ba1d971eae3308bdcfb"
R20_MODULE = "c760dc62345dda734182787dde599657010364a677c651e72dc6f7ce27ae58ed"
H14_MAIN = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
TAG = "QUEEN_FIRST_STEP_WITNESS"


def key(game):
    return game["opponent"], game["map"], int(game["seed"]), game["bot1_side"]


def verified_run(label, main_hash):
    run = RUNS / label
    config_path, audit_path = run / "run.json", run / "verified_audit.json"
    config, audit = json.loads(config_path.read_text()), json.loads(audit_path.read_text())
    assert config["unswbc"] == "1.2.7" and not config["sandbox"]
    assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
    assert config["sources"][0]["files"]["main.cpp"] == main_hash
    if label == R20:
        assert config["sources"][0]["files"]["queen_witness.hpp"] == R20_MODULE
    for source in config["sources"]:
        assert tree_hashes(Path(source["snapshot"])) == source["files"]
        assert sha(source["prepared"]) == source["prepared_sha256"]
    for item in config["maps"]:
        assert sha(item["snapshot"]) == item["sha256"]
    assert sha(config["cli"]) == config["cli_sha256"]
    assert sha(config["engine_original"]) == config["engine_sha256"]
    assert tree_hashes(run / "frozen/engine") == config["engine_snapshot_files"]
    return config, audit, {"label": label, "run_json": str(config_path),
        "run_json_sha256": sha(config_path), "audit_json": str(audit_path),
        "audit_json_sha256": sha(audit_path), "manifest_rechecked": True,
        "engine_sha256": config["engine_sha256"],
        "source_main_sha256": main_hash,
        "binary_sha256": config["sources"][0]["prepared_sha256"],
        "by_opponent": audit["summary"]["by_opponent"]}


def scan(game):
    path = Path(game["replay"])
    assert sha(path) == game["replay_sha256"]
    root = Message(path.read_bytes()).structure(0, 0)
    physical, actions, queen_actions, activations, logs, proposal = [], [], [], [], [], []
    round_index, turn, queen = -1, -1, int(game["bot1_side"] == "B")
    for event_index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
        elif kind == 1:
            turn = data.number(0)
            proposal = []
        elif kind == 6:
            text = data.text(0)
            words = text.split()
            tag = words[1] if words and words[0] == "LOG" and len(words) > 1 else words[0] if words else ""
            if tag == TAG:
                entry = {"round": round_index, "turn": turn, "id": data.number(0),
                         "log_event": event_index, "text": text}
                logs.append(entry)
                proposal.append(entry)
        elif kind == 4:
            did, committed = data.number(0), action(data)
            entry = [round_index, did, committed]
            actions.append(entry)
            if did == queen:
                queen_actions.append(entry)
            for logged in proposal:
                assert logged["id"] == did == queen
                activations.append({**logged, "action_event": event_index,
                                    "committed_action": committed})
            proposal = []
        item = canonical(event)
        if item is not None:
            physical.append(item)
    assert len(activations) == len(logs), (path, "uncommitted witness log")
    return {"physical": physical, "actions": actions, "queen_actions": queen_actions,
            "activations": activations, "log_count": len(logs)}


def digest(sequence):
    return hashlib.sha256(json.dumps(sequence, separators=(",", ":")).encode()).hexdigest()


def first_difference(left, right):
    index = next((i for i, (a, b) in enumerate(zip(left, right)) if a != b), None)
    if index is None and len(left) != len(right):
        index = min(len(left), len(right))
    if index is None:
        return None
    return {"index": index, "H14": left[index] if index < len(left) else None,
            "R20": right[index] if index < len(right) else None}


def main():
    out_json, out_md = HERE / "r20_firstscreen_mechanism.json", HERE / "r20_firstscreen_mechanism.md"
    assert not out_json.exists() and not out_md.exists()
    candidate_config, candidate, candidate_provenance = verified_run(R20, R20_MAIN)
    control_config, control, control_provenance = verified_run(H14, H14_MAIN)
    assert candidate_config["maps"] == control_config["maps"] or {
        m["name"]: m["sha256"] for m in candidate_config["maps"]} == {
        m["name"]: m["sha256"] for m in control_config["maps"]}
    assert candidate_config["engine_sha256"] == control_config["engine_sha256"]
    assert [s["prepared_sha256"] for s in candidate_config["sources"][1:]] == [
        s["prepared_sha256"] for s in control_config["sources"][1:]]
    controls = {key(g): g for g in control["games"]}
    assert len(controls) == len(candidate["games"]) == 68
    games = []
    for game in sorted(candidate["games"], key=lambda g: int(g["game"])):
        baseline = controls[key(game)]
        left, right = scan(baseline), scan(game)
        record = {"opponent": game["opponent"], "map": game["map"], "seed": int(game["seed"]),
            "side": game["bot1_side"], "R20_game": game["game"], "H14_game": baseline["game"],
            "R20_replay": game["replay"], "H14_replay": baseline["replay"],
            "R20_replay_sha256": game["replay_sha256"], "H14_replay_sha256": baseline["replay_sha256"],
            "same_outcome": game["outcome"] == baseline["outcome"],
            "same_standings": game["standings"] == baseline["standings"],
            "same_completed_rounds": game["completed_rounds"] == baseline["completed_rounds"],
            "same_queen_deaths": game["queen_deaths"] == baseline["queen_deaths"],
            "same_actions": left["actions"] == right["actions"],
            "same_queen_actions": left["queen_actions"] == right["queen_actions"],
            "same_physical_sonar_history": left["physical"] == right["physical"],
            "H14_actions_sha256": digest(left["actions"]), "R20_actions_sha256": digest(right["actions"]),
            "H14_physical_sonar_sha256": digest(left["physical"]), "R20_physical_sonar_sha256": digest(right["physical"]),
            "H14_action_count": len(left["actions"]), "R20_action_count": len(right["actions"]),
            "R20_queen_actions": len(right["queen_actions"]), "R20_witness_logs": right["log_count"],
            "R20_committed_witness_changes": right["activations"],
            "first_action_difference": first_difference(left["actions"], right["actions"]),
            "first_physical_sonar_difference": first_difference(left["physical"], right["physical"])}
        games.append(record)
        print(f"paired replay audit {len(games)}/68 witness={right['log_count']} same_actions={record['same_actions']}", flush=True)
    metrics = {field: sum(g[field] for g in games) for field in (
        "same_outcome", "same_standings", "same_completed_rounds", "same_queen_deaths",
        "same_actions", "same_queen_actions", "same_physical_sonar_history", "R20_witness_logs", "R20_queen_actions")}
    payload = {"passed": True, "scope": "One audited native 82001 first screen, ranked17/both seats/two unchanged opponents",
        "source_sha256": sha(__file__), "parser_sha256": sha(RUNS.parent / "protect_local_20260927/analyze_replays.py"),
        "canonical_source_sha256": sha(RUNS / "queen_immediate_engine.py"),
        "provenance": [control_provenance, candidate_provenance], "metrics": metrics, "games": games,
        "limitations": ["Mechanism read packed replay LOG events, not CLI logs; no -v dependence.",
                         "Canonical physical/SONAR excludes bot names, wall time, processing statistics and diagnostics.",
                         "Committed witness logs are changes; zero logs on individual games does not rule out rejected/eligible proposals.",
                         "Developer fixture seed84006 is not part of base82001 first screen.",
                         "No stable cross-seed win-rate or global queen safety claim is supported."]}
    with out_json.open("x") as handle:
        json.dump(payload, handle, indent=2); handle.write("\n")
    changed = [g for g in games if not g["same_actions"] or g["R20_witness_logs"]]
    lines = ["# R20 First-Screen Mechanism", "", "All68 R20 and paired H14 replays passed prior official outcome audits; this analysis rechecked frozen source/binary/map/engine manifests and every replay SHA256. Native unswbc1.2.7, base82001, ranked17 maps, both seats, unchanged opponents. No new match ran and no bot was modified.", "", "| Evidence | Count |", "| --- | ---: |"]
    lines += [f"| {field} | {value} |" for field, value in metrics.items()]
    lines += ["", "R20 first-screen wins are V3_portal15/34 and xxqztest16/34, identical to H14. Paired physical/SONAR histories compare round/turn markers, pearl changes, committed actions, moves, splits, deaths and sonar fields, excluding names, time, processor statistics and diagnostics.", "", f"Changed-action or committed-witness games: {len(changed)}.", ""]
    if changed:
        lines.extend(f"- {g['opponent']} / {g['map']} / seed{g['seed']} / {g['side']}: logs{g['R20_witness_logs']}; first change{g['first_action_difference']}" for g in changed)
    lines += ["", "Mechanism logs were read directly from packed replay kind6 and attached to the following official kind4 queen action; empty CLI logs do not establish nonactivation. Identical action/physical/SONAR histories and zero committed witness logs on an individual game establish no replacement on that game, not the absence of rejected/eligible proposals, because rejection telemetry is not emitted.", "", "The source-bound official fixture uses dilemma84006 A round199, while this first screen includes dilemma82006. That fixture proves a real local N-to-NN correction under its specific input; it does not establish coverage or win-rate gain on82001. The first screen has no win conversions; do not expand R20 based on that alone or generalize zero first-step witnesses into queen safety. All partial/hidden enemies, portals, longer routes and intervening turns remain unknown.", "", "Provenance and per-game hashes are in `r20_firstscreen_mechanism.json`; script is `r20_firstscreen_mechanism.py`. No reserved confirmation seed was read."]
    with out_md.open("x") as handle:
        handle.write("\n".join(lines) + "\n")
    print(json.dumps({"passed": True, "metrics": metrics, "json": str(out_json), "md": str(out_md)}))


if __name__ == "__main__":
    main()
