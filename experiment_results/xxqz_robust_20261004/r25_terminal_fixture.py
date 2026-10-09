#!/usr/bin/env python3
"""Prepare official input for earliest R25 loss witness, never call a bot."""
import hashlib
import importlib.metadata
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(RUNS))
from analyze import Message, parse_map, point, sha
from queen_immediate_engine import observation, ENGINE_SHA
from queen_immediate_witness import topology, visible_cells
from r21_firstscreen_mechanism import action, canonical, update_digest
from unswbc.engine import EngineModule, WASM_PATH

LETTERS = "NESW"


def read_source(replay, target_round, target_id):
    root = Message(replay.read_bytes()).structure(0, 0)
    w, h, bodies, teams = parse_map(root.text(0))
    facing = {}
    for did, body in bodies.items():
        facing[did] = next(d for d, (dx, dy) in enumerate(((0, -1), (1, 0), (0, 1), (-1, 0)))
                           if ((body[1][0] + dx) % w, (body[1][1] + dy) % h) == body[0])
    tape, pearls, prefix, target = {}, set(), [], None
    r, active = -1, None
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            r = data.number(0)
        elif kind == 1:
            active = data.number(0)
            if r <= target_round:
                tape[r, active] = {"action": "", "sonar": []}
        elif kind == 3:
            c = point(data.child(0))
            if data.number(0, "B") & 1:
                pearls.add(c)
            else:
                pearls.discard(c)
        elif kind == 4:
            did = data.number(0)
            if (r, did) == (target_round, target_id):
                target = {"event": index, "round": r, "id": did,
                          "bodies": {i: list(b) for i, b in bodies.items()},
                          "teams": dict(teams), "pearls": sorted(pearls)}
            if r <= target_round:
                kind_action, requested, _ = action(data)
                tape[r, did]["action"] = requested if kind_action in (0, 1) else ""
        elif kind == 9:
            did, head, tail = data.number(0), point(data.child(0)), point(data.child(1))
            facing[did] = data.number(4, "H")
            if r < 0:
                assert bodies[did][0] == head and bodies[did][-1] == tail
            else:
                body = [head] + list(bodies[did])
                while len(body) > 1 and body[-1] != tail:
                    body.pop()
                assert body[-1] == tail
                bodies[did] = body
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            bodies[parent] = [point(p) for p in data.structs(0)]
            bodies[child] = [point(p) for p in data.structs(1)]
            teams[child] = "AB"[data.number(8, "H")]
            facing[child] = data.number(10, "H")
        elif kind == 11:
            bodies.pop(data.number(0), None)
        elif kind == 12 and r <= target_round:
            did, origin = data.number(0), point(data.child(0))
            d = data.number(4, "H")
            if origin != bodies[did][0]:
                assert origin == bodies[did][-1]
                d = (facing[did] + 2) % 4
            tape[r, did]["sonar"].append(f"SONAR {LETTERS[d]} {data.number(16, 'Q')}")
        item = canonical(event)
        if item is not None and target is None:
            prefix.append(item)
    assert target is not None
    return {"map": root.text(0).encode(), "width": w, "height": h,
            "target": target, "tape": tape, "prefix": prefix}


def main():
    output, packed_output = HERE / "r25_terminal_actual_state.json", HERE / "r25_terminal_input_fixture.replay"
    assert not output.exists() and not packed_output.exists()
    probe = json.loads((HERE / "h14_terminal_body_probe.json").read_text())
    eligible = [(g, f) for g in probe["games"] if g["outcome"] == "loss"
                for f in g["findings"] if f["quiet_with_alternate"]]
    game, finding = min(eligible, key=lambda p: (p[1]["round"], p[0]["base"], p[0]["map"], p[0]["side"], p[1]["action_event"]))
    assert (game["seed"], game["side"], finding["round"], finding["id"]) == (84010, "B", 188, 75)
    replay = Path(game["replay"])
    assert sha(replay) == game["replay_sha256"]
    assert importlib.metadata.version("unswbc") == "1.2.7" and sha(WASM_PATH) == ENGINE_SHA
    source = read_source(replay, finding["round"], finding["id"])
    assert source["target"]["event"] == finding["action_event"]
    assert source["target"]["bodies"][finding["id"]] == list(map(tuple, finding["body_before"]))
    digest = hashlib.sha256()
    for item in source["prefix"]:
        update_digest(digest, item)
    assert len(source["prefix"]) == finding["physical_sonar_action_prefix_events"]
    assert digest.hexdigest() == finding["physical_sonar_action_prefix_sha256"]
    engine = EngineModule()
    target_input, spawn_input, seen, cleanup, deaths, notices = None, None, False, [], [], []
    spawns = {}

    def spawn(did, block):
        spawns[did] = block.decode()

    def reply(did, block):
        nonlocal target_input, spawn_input, seen
        r = int(block.decode().splitlines()[0].split()[1])
        if seen:
            cleanup.append([r, did])
            return b"ENDTURN\n"
        item = source["tape"][r, did]
        if (r, did) == (finding["round"], finding["id"]):
            target_input = observation(block)
            spawn_input = spawns[did]
            assert item["action"] == finding["action"]
            seen = True
        return ("\n".join(([item["action"]] if item["action"] else []) + item["sonar"] + ["ENDTURN"]) + "\n").encode()

    engine.run(source["map"], reply, lambda d, r, why: deaths.append([d, r, why]),
               spawn, notices.append, seed=game["seed"])
    packed = engine.replay("R25 input fixture A", "R25 input fixture B")
    regenerated = read_source_from_bytes(packed, finding["round"], finding["id"])
    assert regenerated["prefix"] == source["prefix"]
    # Scripted callbacks omit bot logs, so raw event indices can differ even
    # when all physical/SONAR/action events and the target state are exact.
    for field in ("round", "id", "bodies", "teams", "pearls"):
        assert regenerated["target"][field] == source["target"][field], field
    assert target_input is not None and spawn_input is not None
    view = {(x, y) for x, y, _, _ in target_input["tiles"]}
    assert view == visible_cells(tuple(finding["body_before"][0]), source["width"], source["height"])
    expected = {(did, x, y) for did, body in source["target"]["bodies"].items() for x, y in body if (x, y) in view}
    observed = {(int(p[1]), int(p[2]), int(p[3])) for p in target_input["parts"]}
    assert observed == expected
    assert target_input["length"] == 2
    assert {(x, y) for x, y, has, _ in target_input["tiles"] if has} == set(source["target"]["pearls"]) & view
    assert not [p for p in target_input["parts"] if p[0] == "A"]
    w, h, edges = topology(source["map"].decode())
    topology_rows = [{"cell": c, "direction": LETTERS[d], "kind": edges.get((c, d), (0, -1))[0],
                      "portal_id": edges.get((c, d), (0, -1))[1]} for c in sorted(view) for d in range(4)]
    payload = {"passed": True, "scope": "R25 design/input preparation only; official1.2.7 action tape, no bot or competition",
               "script_sha256": sha(__file__), "engine_sha256": ENGINE_SHA,
               "probe_json_sha256": sha(HERE / "h14_terminal_body_probe.json"),
               "replay": str(replay), "replay_sha256": game["replay_sha256"],
               "map_sha256": hashlib.sha256(source["map"]).hexdigest(),
               "seed": game["seed"], "side": game["side"], "target_id": finding["id"],
               "target_round": finding["round"], "source_action_event": finding["action_event"],
               "source_action": finding["action"], "source_finding": finding,
               "physical_sonar_action_prefix_equal": True,
               "physical_sonar_action_prefix_events": len(source["prefix"]),
               "physical_sonar_action_prefix_sha256": digest.hexdigest(),
               "target_spawn_input": spawn_input, "target_turn_input": target_input,
               "current_visible_topology": topology_rows,
               "local_body": finding["body_before"],
               "current_candidate_alternatives": finding["candidate_ordinary_alternatives"],
               "fixture_replay": str(packed_output), "fixture_replay_sha256": hashlib.sha256(packed).hexdigest(),
               "target_fixture_deaths": [d for d in deaths if d[0] == finding["id"]],
               "cleanup_turn_count": len(cleanup), "notices": notices,
               "limitations": ["All source actions/SONAR are scripted, no persistent bot state is regenerated.",
                                "After the target action, ENDTURN callbacks are deliberate fixture cleanup, not strategy or survival evidence.",
                                "The actual spawn/turn input prepares a source-bound fixture but does not establish H14 or R25 action from a fresh process.",
                                "No alternate action arm was run; immediate escape and live policy remain unverified."]}
    with packed_output.open("xb") as handle:
        handle.write(packed)
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"passed": True, "target": [finding["round"], finding["id"]],
                      "prefix_events": len(source["prefix"]), "raw_input_sha256": target_input["sha256"],
                      "output": str(output)}))


def read_source_from_bytes(packed, target_round, target_id):
    # The exact same parser is used without touching the original replay.
    class ReplayBytes:
        def read_bytes(self):
            return packed
    return read_source(ReplayBytes(), target_round, target_id)


if __name__ == "__main__":
    main()
