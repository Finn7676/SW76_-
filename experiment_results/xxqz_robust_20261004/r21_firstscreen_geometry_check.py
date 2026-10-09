#!/usr/bin/env python3
"""Independent read-only replay geometry/schema checks; never run games."""
from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import zipfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
RUN = ROOT / "experiment_results/queen_rules_20261003/robust_h14_82001"
PARSER = HERE / "r21_firstscreen_mechanism.py"
VIEWER = ROOT / ".venv-battlecode/lib/python3.13/site-packages/unswbc/replay-viewer.vsix"
FIXTURE = RUN / "0052_xxqztest_slithery_fight_seed82009_B.replay"
DIRS = ((0, -1), (1, 0), (0, 1), (-1, 0))
LETTERS = "NESW"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def viewer_evidence():
    with zipfile.ZipFile(VIEWER) as archive:
        source = archive.read("extension/dist/webview/webview.js").decode()
    markers = {
        "kind2_countdown": "displayName:`EventPearlCountdown`",
        "kind3_tile_change": "displayName:`EventTileChange`",
        "kind4_optional_action": "case yf.DRAGON_ACTION:",
        "kind5_engine_log": "displayName:`EventEngineLog`",
        "kind6_dragon_log": "displayName:`EventDragonLog`",
        "kind9_update": "displayName:`EventDragonUpdate`",
        "kind10_split": "displayName:`EventDragonSplit`",
        "kind11_death": "displayName:`EventDragonDeath`",
        "topology_transition": "function qa(",
        "portal_landing": "function Ja(",
        "initial_event_separation": "initEvents=[];constructor(e,t=[])",
    }
    snippets = {}
    for name, marker in markers.items():
        start = source.index(marker)
        snippets[name] = source[start:start + (1400 if name == "initial_event_separation" else 1000)]
    assert "get countdown(){return Z.getInt32(0,this)}" in snippets["kind2_countdown"]
    assert "get hasPearl(){return Z.getBit(0,this)}" in snippets["kind3_tile_change"]
    assert "t._hasAction()?If(t.action):null" in snippets["kind4_optional_action"]
    for name in ("kind5_engine_log", "kind6_dragon_log"):
        assert "get id(){return Z.getInt32(0,this)}" in snippets[name]
        assert "get text(){return Z.getText(0,this)}" in snippets[name]
    assert "get reason(){return Z.getUint16(4,this)}" in snippets["kind11_death"]
    assert "new to(e.initialDragons)" in snippets["initial_event_separation"]
    assert "this.initEvents.push(e)" in snippets["initial_event_separation"]
    return {"path": str(VIEWER), "sha256": sha(VIEWER), "snippets": snippets}


def independent_map(text):
    width = height = None
    edges, ends = {}, defaultdict(set)
    rows = 0
    for line in text.splitlines():
        words = line.split("#", 1)[0].split()
        if not words:
            continue
        if words[0] == "MAP":
            width, height = map(int, words[1:3])
        if words[0] != "EDGE":
            continue
        assert width and height
        edge_id, kind, pid = map(int, words[1:4])
        row, x = divmod(edge_id, width + 1)
        assert 0 <= row <= 2 * height and kind in (0, 1, 2)
        if row % 2 == 0:
            assert x < width
            endpoint = "H", x, row // 2 % height
        else:
            assert row < 2 * height
            endpoint = "V", x % width, (row - 1) // 2
        value = kind, pid
        assert endpoint not in edges or edges[endpoint] == value
        edges[endpoint] = value
        if kind == 2:
            assert pid >= 0
            ends[pid].add(endpoint)
        rows += 1
    assert len(edges) == 2 * width * height
    for endpoints in ends.values():
        assert len(endpoints) == 2 and len({edge[0] for edge in endpoints}) == 1
    return width, height, edges, ends, rows


def compare_geometry(module, item):
    path = Path(item["snapshot"])
    assert sha(path) == item["sha256"]
    text = path.read_text()
    width, height, edges, ends, rows = independent_map(text)
    parsed_edges, parsed_ends = module.topology(text, width, height)
    checked_pairs = checked_transitions = 0
    for y in range(height):
        for x in range(width):
            neck = x, y
            expected = {}
            for direction, (dx, dy) in enumerate(DIRS):
                # Official viewer uses N/W edge endpoints, independent of the
                # mechanism parser's E/S canonical endpoint representation.
                endpoint = ("H", x, y) if direction == 0 else \
                    ("V", (x + 1) % width, y) if direction == 1 else \
                    ("H", x, (y + 1) % height) if direction == 2 else ("V", x, y)
                kind, pid = edges[endpoint]
                if kind == 1:
                    continue
                if kind == 0:
                    destination = (x + dx) % width, (y + dy) % height
                else:
                    paired = next(edge for edge in ends[pid] if edge != endpoint)
                    axis, px, py = paired
                    assert axis == endpoint[0]
                    destination = (px, py if direction == 2 else (py - 1) % height) \
                        if axis == "H" else (px if direction == 1 else (px - 1) % width, py)
                entry = expected.setdefault(destination, {"ordinary": [], "portals": []})
                if kind == 0:
                    entry["ordinary"].append(LETTERS[direction])
                else:
                    entry["portals"].append((LETTERS[direction], pid))
                checked_transitions += 1
            for head, expected_links in expected.items():
                actual = module.pair_geometry([head, neck], parsed_edges, parsed_ends, width, height)
                assert actual["ordinary_links"] == expected_links["ordinary"], (item["name"], neck, head)
                assert [(link["direction"], link["portal_id"]) for link in actual["portal_links"]] == \
                    expected_links["portals"], (item["name"], neck, head)
                ordinary, portals = expected_links["ordinary"], expected_links["portals"]
                classification = "ambiguous_ordinary_and_portal" if ordinary and portals else \
                    "ordinary_edge" if ordinary else "portal_link"
                assert actual["classification"] == classification
                checked_pairs += 1
    return {"map": item["name"], "path": str(path), "sha256": item["sha256"],
        "width": width, "height": height, "edge_rows": rows, "canonical_edges": len(edges),
        "portal_pairs": len(ends), "checked_transitions": checked_transitions,
        "checked_topology_reachable_head_neck_pairs": checked_pairs, "passed": True}


def check_early_semantics(module):
    root = module.Message(FIXTURE.read_bytes()).structure(0, 0)
    _, _, bodies, teams = module.parse_map(root.text(0))
    round_index, active, initial_updates, splits = -1, None, 0, 0
    actions = {}
    for index, event in enumerate(root.structs(3)):
        kind, data = event.number(0, "H"), event.child(0)
        if kind == 0:
            round_index = data.number(0)
            if round_index >= 3:
                break
        elif kind == 1:
            active = data.number(0)
        elif kind == 4:
            actions[data.number(0)] = round_index, module.action(data)
        elif kind == 9:
            did = data.number(0)
            head, tail = module.point(data.child(0)), module.point(data.child(1))
            if round_index < 0:
                assert bodies[did][0] == head and bodies[did][-1] == tail
                initial_updates += 1
                continue
            bodies[did].insert(0, head)
            while len(bodies[did]) > 1 and bodies[did][-1] != tail:
                bodies[did].pop()
            assert bodies[did][-1] == tail
        elif kind == 10:
            parent, child = data.number(0), data.number(4)
            parent_body = [module.point(value) for value in data.structs(0)]
            child_body = [module.point(value) for value in data.structs(1)]
            assert bodies[parent] == parent_body + list(reversed(child_body)), (index, parent)
            assert active == parent and actions[parent][0] == round_index
            assert actions[parent][1][0] == 1 and actions[parent][1][2] == len(child_body)
            bodies[parent], bodies[child] = parent_body, child_body
            teams[child] = "AB"[data.number(8, "H")]
            splits += 1
        elif kind == 11:
            del bodies[data.number(0)]
    assert initial_updates == 14 and splits == 23
    return {"path": str(FIXTURE), "sha256": sha(FIXTURE), "rounds_read": [0, 1, 2],
        "initial_updates_matching_map": initial_updates,
        "split_arrays_matching_exact_prebody_and_same_turn_action": splits,
        "outcomes_or_terminal_result_read": False, "passed": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=HERE / "r21_firstscreen_geometry_check.json")
    args = parser.parse_args()
    assert not args.output.exists(), "Output exists; choose a new exclusive output path"
    parser_hash = sha(PARSER)
    specification = importlib.util.spec_from_file_location("r21_semantic_geometry_subject", PARSER)
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    config_path = RUN / "run.json"
    config = json.loads(config_path.read_text())
    assert config["unswbc"] == "1.2.7" and len(config["maps"]) == 17
    assert sha(config["engine_original"]) == config["engine_sha256"]
    evidence = viewer_evidence()
    maps = [compare_geometry(module, item) for item in config["maps"]]
    early = check_early_semantics(module)
    assert sha(PARSER) == parser_hash, "Subject parser changed during this check"
    output = {"passed": True, "scope": "Read-only schema and map geometry; no matches or outcome analysis",
        "script_sha256": sha(__file__), "subject_parser": str(PARSER),
        "subject_parser_sha256": parser_hash, "run_json": str(config_path),
        "run_json_sha256": sha(config_path), "engine_sha256": config["engine_sha256"],
        "viewer_evidence": evidence, "maps": maps, "early_semantics": early,
        "checked_maps": len(maps), "checked_topology_reachable_head_neck_pairs":
            sum(item["checked_topology_reachable_head_neck_pairs"] for item in maps),
        "checked_transitions": sum(item["checked_transitions"] for item in maps),
        "limitations": ["Topology-reachable pairs are not proof that a live dragon occupied them.",
            "Geometry classification is not a death-cause inference.",
            "No outcomes, generalization or experiment selection verdict is provided."]}
    with args.output.open("x", encoding="utf-8") as handle:
        json.dump(output, handle, indent=2)
        handle.write("\n")
    print(json.dumps({"passed": True, "maps": output["checked_maps"],
        "head_neck_pairs": output["checked_topology_reachable_head_neck_pairs"],
        "transitions": output["checked_transitions"], "output": str(args.output)}, indent=2))


if __name__ == "__main__":
    main()
