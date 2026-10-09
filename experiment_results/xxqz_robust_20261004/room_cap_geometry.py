#!/usr/bin/env python3
"""Map-only room-cap diagnosis plus local source-bound fixture output."""
from __future__ import annotations

import argparse
from collections import Counter, deque
import hashlib
import importlib.util
from importlib.metadata import version
import json
from pathlib import Path
import subprocess
import sys
import zipfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT))
import benchmark

DIRS = ((0, -1), (1, 0), (0, 1), (-1, 0))
LETTERS = "NESW"
H14 = ROOT / "tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"
R21 = HERE / "r21_firstscreen_mechanism.py"
FIXTURE = HERE / "room_cap_local_fixture.cpp.txt"
VIEWER = ROOT / ".venv-battlecode/lib/python3.13/site-packages/unswbc/replay-viewer.vsix"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def geo(cell, direction, width, height):
    dx, dy = DIRS[direction]
    return ((cell[0] + dx) % width, (cell[1] + dy) % height)


def canonical(cell, direction, width, height):
    if direction in (1, 2):
        return cell, direction
    return geo(cell, direction, width, height), direction ^ 2


def interval_span(values, modulus):
    values = sorted(set(values))
    if len(values) == modulus:
        return modulus
    gaps = [(values[(i + 1) % len(values)] - value) % modulus for i, value in enumerate(values)]
    return modulus - max(gaps) + 1


def cheb(a, b, width, height):
    dx, dy = abs(a[0] - b[0]), abs(a[1] - b[1])
    return max(min(dx, width - dx), min(dy, height - dy))


def parse_map(path, r21):
    text = path.read_text()
    tiles, width, height = {}, None, None
    for line in text.splitlines():
        words = line.split("#", 1)[0].split()
        if not words:
            continue
        if words[0] == "MAP":
            width, height = map(int, words[1:3])
        elif words[0] == "TILE":
            x, y, low, high = map(int, words[1:5])
            assert (x, y) not in tiles and 0 <= low <= high
            tiles[x, y] = low, high
    assert len(tiles) == width * height
    edges, portal_ends = r21.topology(text, width, height)
    assert len(edges) == 4 * width * height
    assert all(len(ends) == 2 for ends in portal_ends.values())
    for (cell, direction), value in edges.items():
        assert value == edges[geo(cell, direction, width, height), direction ^ 2]
    remaining = set(tiles)
    components = []
    while remaining:
        start = min(remaining, key=lambda c: (c[1], c[0]))
        cells = {start}
        queue = deque([start])
        remaining.remove(start)
        while queue:
            cell = queue.popleft()
            for direction in range(4):
                if edges[cell, direction][0] != 0:
                    continue
                neighbor = geo(cell, direction, width, height)
                if neighbor not in cells:
                    cells.add(neighbor)
                    remaining.remove(neighbor)
                    queue.append(neighbor)
        boundaries = {}
        incident_count = Counter()
        for cell in cells:
            for direction in range(4):
                kind, pid = edges[cell, direction]
                if kind != 2:
                    continue
                key = canonical(cell, direction, width, height)
                assert key not in boundaries or boundaries[key] == pid
                boundaries[key] = pid
                incident_count[key] += 1
        sources = {c: tiles[c] for c in cells if tiles[c][1] > 0}
        full_view_centers = [c for c in cells if all(cheb(c, other, width, height) <= 3 for other in cells)] \
            if len(cells) <= 49 else []
        bounds = [min(c[0] for c in cells), min(c[1] for c in cells),
                  max(c[0] for c in cells), max(c[1] for c in cells)]
        components.append({
            "id": len(components), "area": len(cells), "bbox_xyxy": bounds,
            "wrapped_span_xy": [interval_span([c[0] for c in cells], width),
                                interval_span([c[1] for c in cells], height)],
            "portal_boundary_edges": [{"cell": list(cell), "direction": LETTERS[direction],
                "pid": pid, "incident_sides_in_component": incident_count[cell, direction]}
                for (cell, direction), pid in sorted(boundaries.items())],
            "portal_boundary_count": len(boundaries), "spawn_source_count": len(sources),
            "spawn_intervals": {f"{low}:{high}": count for (low, high), count in sorted(Counter(sources.values()).items())},
            "single_boundary_resource": len(boundaries) == 1 and bool(sources),
            "eligible_cap24_complete_knowledge": len(boundaries) == 1 and bool(sources) and len(cells) <= 24,
            "eligible_cap49_complete_knowledge": len(boundaries) == 1 and bool(sources) and len(cells) <= 49,
            "newly_eligible_24_to49": len(boundaries) == 1 and bool(sources) and 24 < len(cells) <= 49,
            "full_room_current_7x7_centers": [list(c) for c in sorted(full_view_centers)],
            "cells": [list(c) for c in sorted(cells, key=lambda c: (c[1], c[0]))],
        })
    assert sum(c["area"] for c in components) == width * height
    rooms = [c for c in components if c["single_boundary_resource"]]
    return {"map": path.stem, "path": str(path), "sha256": sha(path),
        "width": width, "height": height, "portal_pairs": len(portal_ends),
        "ordinary_component_count": len(components), "single_boundary_resource_count": len(rooms),
        "single_boundary_resource_areas": sorted(c["area"] for c in rooms),
        "eligible_cap24": sum(c["eligible_cap24_complete_knowledge"] for c in rooms),
        "eligible_cap49": sum(c["eligible_cap49_complete_knowledge"] for c in rooms),
        "newly_eligible_count": sum(c["newly_eligible_24_to49"] for c in rooms),
        "components": components}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture-binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=HERE / "room_cap_diagnosis.json")
    args = parser.parse_args()
    assert not args.output.exists()
    assert version("unswbc") == "1.2.7"
    assert sha(H14) == "ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc"
    spec = importlib.util.spec_from_file_location("r21_room_geometry", R21)
    r21 = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(r21)
    maps = [parse_map(path, r21) for path in benchmark.resolve_maps(["all"])]
    assert len(maps) == 17
    with zipfile.ZipFile(VIEWER) as archive:
        viewer = archive.read("extension/dist/webview/webview.js").decode()
    marker = "u[t*n+e]={minRounds:i,maxRounds:a},m.TILE++"
    assert marker in viewer
    fixture_run = subprocess.run([str(args.fixture_binary.resolve())], cwd=ROOT, check=True,
                                 capture_output=True, text=True)
    fixtures = []
    for line in fixture_run.stdout.splitlines():
        assert line.startswith("{")
        fixtures.append(json.loads(line))
    assert len(fixtures) == 8
    rooms = [(m["map"], c) for m in maps for c in m["components"] if c["single_boundary_resource"]]
    added = [(name, c) for name, c in rooms if c["newly_eligible_24_to49"]]
    output = {"scope": "17 official maps and local observable-state fixtures only; no competitive games, no holdout read",
        "rules": "unswbc 1.2.7", "h14_main_sha256": sha(H14),
        "script_sha256": sha(__file__), "topology_parser_sha256": sha(R21),
        "fixture_source_sha256": sha(FIXTURE), "fixture_binary_sha256": sha(args.fixture_binary),
        "official_viewer_sha256": sha(VIEWER), "tile_interval_schema_marker": marker,
        "resource_definition": "TILE maxRounds > 0 denotes configured respawn sources; runtime proof still requires known finite pearlDue",
        "geometry_method": "Wrapped 4-neighbor floodfill through kind=0 only; kelp and portal edges do not join ordinary components; portal boundaries deduplicated with H14 E/S canonical edge",
        "summary": {"maps": len(maps), "ordinary_components": sum(m["ordinary_component_count"] for m in maps),
            "single_boundary_resource_rooms": len(rooms),
            "room_area_histogram": dict(sorted(Counter(c["area"] for _, c in rooms).items())),
            "eligible_cap24": sum(m["eligible_cap24"] for m in maps),
            "eligible_cap49": sum(m["eligible_cap49"] for m in maps),
            "newly_eligible_rooms": len(added), "newly_eligible_cells": sum(c["area"] for _, c in added),
            "newly_eligible_spawn_sources": sum(c["spawn_source_count"] for _, c in added),
            "newly_eligible_maps": sorted({name for name, _ in added})},
        "maps": maps, "local_fixtures": fixtures,
        "limitations": ["Static configured respawn sources are not currently available pearls or measured yields.",
            "Cap49 is an area bound, not a shape, one-view, safety, or occupancy guarantee; all unknown edges remain disqualifying.",
            "Fixtures do not reconstruct decoded sonar history or all remembered pearl timers.",
            "No outcome, survival over subsequent turns, cross-seed improvement, or holdout claim is made."]}
    with args.output.open("x", encoding="utf-8") as handle:
        json.dump(output, handle, indent=2)
        handle.write("\n")
    print(json.dumps(output["summary"], indent=2))
    for item in maps:
        print(item["map"], item["ordinary_component_count"], item["single_boundary_resource_areas"],
              item["eligible_cap24"], item["eligible_cap49"])
    print(json.dumps(fixtures, indent=2))


if __name__ == "__main__":
    main()
