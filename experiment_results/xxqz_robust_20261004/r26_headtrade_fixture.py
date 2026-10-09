#!/usr/bin/env python3
"""Reconstruct actual H14 input and scripted head-trade rule arms, no matches."""
import hashlib
import importlib.metadata
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
RUNS = HERE.parent / "queen_rules_20261003"
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(RUNS))
from analyze import sha
from r21_firstscreen_mechanism import update_digest
from r25_terminal_fixture import read_source, read_source_from_bytes
from queen_immediate_engine import observation, ENGINE_SHA
from queen_immediate_witness import topology, landing, visible_cells
from unswbc.bot import Bot, Pool
from unswbc.engine import EngineModule, WASM_PATH

H14_SHA = "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
LETTERS = "NESW"
CASES = [
    {"label": "default", "base": 82001, "seed": 82003, "game": "39", "id": 2,
     "round": 20, "attacker": 3, "alternate": "N", "event": 3226},
    {"label": "australia", "base": 83001, "seed": 83001, "game": "35", "id": 341,
     "round": 352, "attacker": 414, "alternate": "N", "event": 206654},
]


def prefix_hash(items):
    stream = hashlib.sha256()
    for item in items:
        update_digest(stream, item)
    return stream.hexdigest()


def input_body(raw, actor, expected_length, width, height, edges):
    view = {(x, y) for x, y, _, _ in raw["tiles"]}
    shown = {}
    for team, did, x, y, facing, head in raw["parts"]:
        if int(did) == actor:
            cell = int(x), int(y)
            assert cell not in shown
            shown[cell] = (LETTERS.index(facing), int(head))
    heads = [c for c, (_, is_head) in shown.items() if is_head]
    assert len(heads) == 1
    predecessors = {}
    for cell, (facing, is_head) in shown.items():
        if is_head:
            continue
        assert edges.get((cell, facing), (0, -1))[0] == 0
        front = landing(cell, facing, width, height)
        assert front in shown and front not in predecessors
        predecessors[front] = cell
    chain, cell = [], heads[0]
    while cell is not None:
        assert cell not in chain
        chain.append(cell)
        cell = predecessors.get(cell)
    assert set(chain) == set(shown) and len(chain) == expected_length
    assert set(chain) <= view
    tail_proof = []
    for direction in range(4):
        kind = edges.get((chain[-1], direction), (0, -1))[0]
        neighbor = landing(chain[-1], direction, width, height)
        assert kind == 1 or (kind == 0 and neighbor in view)
        tail_proof.append({"direction": LETTERS[direction], "kind": kind,
                           "neighbor": neighbor, "current_visible": neighbor in view})
    return {"body": chain, "unique_predecessor_chain": True,
            "all_possible_tail_predecessors_known_visible_or_wall": tail_proof}


def native(binary, spawn, raw):
    pool = Pool([str(binary.resolve())], size=0)
    bot = Bot(pool, init=spawn.encode(), name="r26_actual_h14_control")
    try:
        output = bot.ask(raw.encode()).decode()
        assert bot.error is None, bot.error
        stderr = bot.take_stderr().decode()
    finally:
        bot.stop()
        pool.close()
    actions = [row for row in output.splitlines() if row.startswith(("MOVE ", "SPLIT "))]
    assert len(actions) == 1
    return {"action": actions[0], "output": output, "stderr": stderr,
            "binary_sha256": sha(binary), "fresh_process_no_prior_memory": True}


def arm(case, source, move):
    engine = EngineModule()
    target_input, target_spawn = None, None
    spawns, deaths, inputs, replies = {}, [], [], []
    done = False

    def spawn(did, block):
        spawns[did] = block.decode()

    def reply(did, block):
        nonlocal done, target_input, target_spawn
        round_index = int(block.decode().splitlines()[0].split()[1])
        if done:
            return b"ENDTURN\n"
        row = source["tape"][round_index, did]
        action = row["action"]
        if (round_index, did) == (case["round"], case["id"]):
            target_input, target_spawn = observation(block), spawns[did]
            assert action == "MOVE W"
            action = "MOVE " + move
        if (round_index, did) == (case["round"], case["attacker"]):
            inputs.append({"actor": did, "raw": observation(block)})
            assert action == "MOVE N"
            done = True
        replies.append({"round": round_index, "id": did, "action": action,
                        "recorded_action": row["action"]})
        return ("\n".join(([action] if action else []) + row["sonar"] + ["ENDTURN"]) + "\n").encode()

    engine.run(source["map"], reply, lambda d, r, why: deaths.append([d, r, why]),
               spawn, lambda _: None, seed=case["seed"])
    assert done and target_input is not None
    packed = engine.replay("R26 scripted input A", "R26 scripted input B")
    check = read_source_from_bytes(packed, case["round"], case["id"])
    assert check["prefix"] == source["prefix"]
    for field in ("round", "id", "bodies", "teams", "pearls"):
        assert check["target"][field] == source["target"][field]
    before_attacker = read_source_from_bytes(packed, case["round"], case["attacker"])["target"]
    changes = [row for row in replies if row["action"] != row["recorded_action"]]
    assert len(changes) == (move != "W")
    window = [d for d in deaths if d[0] in (case["id"], case["attacker"]) and d[1] == case["round"]]
    if move == "W":
        assert {d[0] for d in window if d[2] == "H"} == {case["id"], case["attacker"]}, window
    else:
        assert not window, window
    return {"action": "MOVE " + move, "target_input": target_input,
            "target_spawn": target_spawn, "physical_sonar_action_prefix_equal": True,
            "prefix_events": len(check["prefix"]), "prefix_sha256": prefix_hash(check["prefix"]),
            "body_before_attacker": before_attacker["bodies"][case["id"]],
            "attacker_input": inputs, "target_and_attacker_deaths_same_round": window,
            "changed_replies": changes, "recorded_other_actor_actions_sonar_fixed": True,
            "cleanup_after_attacker_is_not_strategy_evidence": True}, packed


def main():
    output = HERE / "r26_headtrade_actual_state.json"
    assert not output.exists()
    assert importlib.metadata.version("unswbc") == "1.2.7" and sha(WASM_PATH) == ENGINE_SHA
    cases = []
    for case in CASES:
        run = RUNS / f"robust_h14_{case['base']}"
        config, audit = [json.loads((run / f).read_text()) for f in ("run.json", "verified_audit.json")]
        assert audit["summary"]["audit_passed"] and len(audit["games"]) == 68
        assert config["unswbc"] == "1.2.7" and not config["sandbox"]
        assert config["sources"][0]["files"]["main.cpp"] == H14_SHA
        game = next(g for g in audit["games"] if g["game"] == case["game"])
        assert game["opponent"] == "xxqztest" and game["outcome"] == "loss"
        path = Path(game["replay"])
        assert sha(path) == game["replay_sha256"]
        source = read_source(path, case["round"], case["id"])
        assert source["target"]["event"] == case["event"]
        width, height, edges = topology(source["map"].decode())
        arms, packed_arms = [], []
        for name, move in (("original", "W"), ("alternate", case["alternate"])):
            evidence, packed = arm(case, source, move)
            raw = evidence["target_input"]
            own_body = source["target"]["bodies"][case["id"]]
            enemy_body = source["target"]["bodies"][case["attacker"]]
            evidence["own_input_body_proof"] = input_body(raw, case["id"], len(own_body), width, height, edges)
            evidence["enemy_input_body_proof"] = input_body(raw, case["attacker"], len(enemy_body), width, height, edges)
            assert evidence["own_input_body_proof"]["body"] == own_body
            assert evidence["enemy_input_body_proof"]["body"] == enemy_body
            assert raw["length"] == len(own_body)
            assert {(x, y) for x, y, _, _ in raw["tiles"]} == visible_cells(own_body[0], width, height)
            arms.append({"label": name, **evidence})
            packed_arms.append(packed)
        assert arms[0]["target_input"] == arms[1]["target_input"]
        binary = Path(config["sources"][0]["prepared"])
        assert sha(binary) == config["sources"][0]["prepared_sha256"]
        decision = native(binary, arms[0]["target_spawn"], arms[0]["target_input"]["raw"])
        assert decision["action"] == "MOVE W", decision
        for evidence, packed in zip(arms, packed_arms):
            target = HERE / f"r26_{case['label']}_{evidence['label']}.replay"
            assert not target.exists()
            with target.open("xb") as handle:
                handle.write(packed)
            evidence["fixture_replay"] = str(target)
            evidence["fixture_replay_sha256"] = hashlib.sha256(packed).hexdigest()
        cases.append({**case, "source_replay": str(path), "source_replay_sha256": game["replay_sha256"],
                      "map_sha256": sha(run / "frozen/maps" / f"{case['label']}.map") if (run / "frozen/maps" / f"{case['label']}.map").exists() else hashlib.sha256(source["map"]).hexdigest(),
                      "source_run_json_sha256": sha(run / "run.json"),
                      "source_audit_json_sha256": sha(run / "verified_audit.json"),
                      "source_main_sha256": H14_SHA, "native_H14": decision, "arms": arms})
        print(f"{case['label']} actual input, predecessor body proof, fresh H14 W and official W/N arms passed", flush=True)
    payload = {"passed": True, "scope": "Actual-input scripted rule evidence; no candidate, matches or win claim",
               "script_sha256": sha(__file__), "engine_sha256": ENGINE_SHA, "cases": cases,
               "limitations": ["Fresh native controls have no prior-round private memory.",
                   "Exact source prefix contains official physical, SONAR and committed action events; bot logs are excluded.",
                   "Other actors keep recorded actions/SONAR after the alternate; live reactions may differ.",
                   "Incoming first-step geometry is possible, not a global next-actor or future survival prediction.",
                   "Local survival against the witnessed enemy does not establish better competitive wins."]}
    with output.open("x") as handle:
        json.dump(payload, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"passed": True, "output": str(output), "cases": len(cases)}))


if __name__ == "__main__":
    main()
