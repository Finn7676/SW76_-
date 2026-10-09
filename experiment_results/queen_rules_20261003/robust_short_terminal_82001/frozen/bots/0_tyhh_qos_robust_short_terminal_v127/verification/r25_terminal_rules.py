#!/usr/bin/env python3
"""Official scripted R25 terminal/escape arms, not bots or competition."""
import hashlib
import importlib.metadata
import json
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / "experiment_results/xxqz_robust_20261004"
RUNS = ROOT / "experiment_results/queen_rules_20261003"
sys.path.insert(0, str(EVIDENCE))
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, REASONS, sha
from r25_terminal_fixture import read_source, read_source_from_bytes
from r21_firstscreen_mechanism import action, update_digest
from queen_immediate_engine import observation, ENGINE_SHA
from unswbc.engine import EngineModule, WASM_PATH

OUT = Path(__file__).resolve().parent


def actor_history(packed, actor, start, stop):
    root = Message(packed).structure(0, 0)
    _, _, bodies, _ = parse_map(root.text(0))
    r, active, action_event = -1, None, None
    records, pickups, deaths = [], [], []
    removed = set()
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            r = data.number(0)
        elif kind == 1:
            active = data.number(0)
            removed = set()
            action_event = None
        elif kind == 3:
            if not data.number(0, "B") & 1:
                removed.add(point(data.child(0)))
        elif kind == 4:
            did = data.number(0)
            if did == actor and start <= r <= stop:
                action_event = {"round": r, "event": index, "action": action(data)[1],
                                "body_before": list(bodies[did]), "successful_updates": []}
                records.append(action_event)
        elif kind == 9:
            did, head, tail = data.number(0), point(data.child(0)), point(data.child(1))
            old = list(bodies[did])
            if r < 0:
                assert old[0] == head and old[-1] == tail
                continue
            body = [head] + old
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            assert body[-1] == tail
            bodies[did] = body
            if did == actor and start <= r <= stop:
                assert action_event is not None
                update = {"round": r, "event": index, "body_after": list(body),
                          "head": head, "tail": tail, "length": len(body),
                          "pearl_removed_on_this_step": head in removed}
                action_event["successful_updates"].append(update)
                if head in removed:
                    pickups.append(update)
            removed.clear()
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
        elif kind == 11:
            did = data.number(0)
            if did == actor and start <= r <= stop:
                deaths.append({"round": r, "event": index,
                               "reason": REASONS[data.number(4, "H")],
                               "active_turn": active, "body_before": list(bodies[did])})
            bodies.pop(did, None)
    return {"actions": records, "actual_pickups": pickups, "deaths_in_evidence_window": deaths}


def run_arm(source, full_tape, saved, label, first, next_action):
    start = time.perf_counter()
    actor, initial_round = saved["target_id"], saved["target_round"]
    finish_round = initial_round + 1
    engine = EngineModule()
    spawns, inputs, replies, cleanup, notices, deaths = {}, {}, [], [], [], []
    completed = False

    def spawn(did, block):
        spawns[did] = block.decode()

    def reply(did, block):
        nonlocal completed
        r = int(block.decode().splitlines()[0].split()[1])
        if completed:
            cleanup.append([r, did])
            return b"ENDTURN\n"
        entry = full_tape[r, did]
        selected = entry["action"]
        if did == actor and r in (initial_round, finish_round):
            inputs[str(r)] = observation(block)
            selected = "MOVE " + (first if r == initial_round else next_action)
            assert entry["action"] == (saved["source_action"] if r == initial_round else "MOVE N")
            if r == finish_round:
                completed = True
        replies.append({"round": r, "id": did, "action": selected,
                        "recorded_action": entry["action"], "sonar": list(entry["sonar"])})
        return ("\n".join(([selected] if selected else []) + entry["sonar"] + ["ENDTURN"]) + "\n").encode()

    engine.run(source["map"], reply, lambda d, r, why: deaths.append([d, r, why]),
               spawn, notices.append, seed=saved["seed"])
    assert completed and set(inputs) == {str(initial_round), str(finish_round)}
    assert inputs[str(initial_round)] == saved["target_turn_input"]
    assert spawns[actor] == saved["target_spawn_input"]
    packed = engine.replay(f"R25 {label} rule fixture A", f"R25 {label} rule fixture B")
    regenerated = read_source_from_bytes(packed, initial_round, actor)
    assert regenerated["prefix"] == source["prefix"]
    for field in ("round", "id", "bodies", "teams", "pearls"):
        assert regenerated["target"][field] == source["target"][field]
    prefix_digest = hashlib.sha256()
    for item in regenerated["prefix"]:
        update_digest(prefix_digest, item)
    assert prefix_digest.hexdigest() == saved["physical_sonar_action_prefix_sha256"]
    changes = [a for a in replies if a["action"] != a["recorded_action"]]
    if label == "original":
        assert not changes
    else:
        assert [(a["round"], a["id"], a["action"]) for a in changes] == [
            (initial_round, actor, "MOVE E"), (finish_round, actor, "MOVE S")]
    history = actor_history(packed, actor, initial_round, finish_round)
    assert len(history["actions"]) == 2
    assert [a["action"] for a in history["actions"]] == ["MOVE " + first, "MOVE " + next_action]
    if label == "original":
        assert history["actions"][0]["successful_updates"][0]["body_after"] == [(34, 22), (34, 21), (33, 21)]
        assert history["actions"][0]["successful_updates"][0]["pearl_removed_on_this_step"]
        assert history["actions"][1]["successful_updates"] == []
        assert [(d["round"], d["reason"]) for d in history["deaths_in_evidence_window"]] == [(finish_round, "self")]
    else:
        assert history["actions"][0]["successful_updates"][0]["body_after"] == [(35, 21), (34, 21)]
        assert history["actions"][1]["successful_updates"][0]["body_after"] == [(35, 22), (35, 21)]
        assert not history["deaths_in_evidence_window"]
    return {"label": label, "elapsed_seconds": time.perf_counter() - start,
            "actions": ["MOVE " + first, "MOVE " + next_action],
            "target_turn_inputs": inputs,
            "target_spawn_input_equal": True,
            "physical_sonar_action_prefix_equal": True,
            "physical_sonar_action_prefix_events": len(regenerated["prefix"]),
            "physical_sonar_action_prefix_sha256": prefix_digest.hexdigest(),
            "recorded_other_actor_actions_and_sonar_held_fixed": True,
            "reply_count_before_cleanup": len(replies), "changed_replies": changes,
            "actor_history": history,
            "target_death_callbacks_in_evidence_window": [d for d in deaths if d[0] == actor and initial_round <= d[1] <= finish_round],
            "cleanup_turn_count": len(cleanup), "notices": notices,
            "fixture_replay_sha256": hashlib.sha256(packed).hexdigest()}, packed


def main():
    output = OUT / "r25_terminal_rules.json"
    paths = [OUT / "r25_terminal_rules_original.replay", OUT / "r25_terminal_rules_alternate.replay"]
    assert not any(p.exists() for p in [output] + paths)
    saved = json.loads((EVIDENCE / "r25_terminal_actual_state.json").read_text())
    assert saved["passed"] and (saved["target_round"], saved["target_id"]) == (188, 75)
    assert importlib.metadata.version("unswbc") == "1.2.7" and sha(WASM_PATH) == ENGINE_SHA
    replay = Path(saved["replay"])
    assert sha(replay) == saved["replay_sha256"]
    source = read_source(replay, saved["target_round"], saved["target_id"])
    full = read_source(replay, saved["target_round"] + 1, saved["target_id"])
    assert len(source["prefix"]) == saved["physical_sonar_action_prefix_events"] == 27385
    arms, packed_arms = [], []
    for label, first, second in [("original", "S", "N"), ("alternate", "E", "S")]:
        arm, packed = run_arm(source, full["tape"], saved, label, first, second)
        arms.append(arm)
        packed_arms.append(packed)
        print(f"{label} official1.2.7 fixture passed in {arm['elapsed_seconds']:.3f}s", flush=True)
    assert arms[0]["target_turn_inputs"]["188"] == arms[1]["target_turn_inputs"]["188"]
    for arm, packed, path in zip(arms, packed_arms, paths):
        with path.open("xb") as handle:
            handle.write(packed)
        arm["fixture_replay"] = str(path)
    payload = {"passed": True, "scope": "Official1.2.7 scripted two-arm rule fixture; no bot call or competitive result",
               "script_sha256": sha(__file__), "engine_sha256": ENGINE_SHA,
               "source_replay": str(replay), "source_replay_sha256": saved["replay_sha256"],
               "source_actual_state": str(EVIDENCE / "r25_terminal_actual_state.json"),
               "source_actual_state_sha256": sha(EVIDENCE / "r25_terminal_actual_state.json"),
               "map_sha256": saved["map_sha256"], "seed": saved["seed"], "side": saved["side"],
               "target": {"round": 188, "id": 75, "body": saved["local_body"]},
               "identical_target_raw_input_sha256": saved["target_turn_input"]["sha256"],
               "source_physical_sonar_action_prefix": source["prefix"], "arms": arms,
               "limitations": ["The native source-bound fixture separately proves H14S/R25E on the saved input; no live bot is called here.",
                                "Next-round alternateS is scripted. This proves engine legality and local survival only, not that the live candidate choosesS.",
                                "Other actors retain their recorded actions/SONAR despite the alternate body; they could react differently in a competitive game.",
                                "ENDTURN replies after actor75 round189 are deliberate cleanup, and their deaths are not strategy evidence.",
                                "No cross-map win benefit, runtime bound or global hidden-body/future-actor safety is established."]}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"passed": True, "output": str(output),
                      "prefix_events": len(source["prefix"]),
                      "target_raw_sha256": payload["identical_target_raw_input_sha256"],
                      "original": arms[0]["target_death_callbacks_in_evidence_window"],
                      "alternate": arms[1]["target_death_callbacks_in_evidence_window"]}))


if __name__ == "__main__":
    main()
