#!/usr/bin/env python3
"""Deterministic death/collection microfixtures; never a bot comparison."""
import hashlib
import json
from unswbc.engine import EngineModule, WASM_PATH

BODY = [(6, 6), (5, 6), (4, 6), (4, 7), (5, 7), (6, 7)]


def fixture(length, action):
    lines = ["MAP 16 16", "SYMMETRY xy", "MAP_NAME Crown Unit Fixture",
             "TILE_COUNT 256"]
    lines += [f"TILE {x} {y} 0 0" for y in range(16) for x in range(16)]
    lines += ["EDGE_COUNT 0", "DRAGON_COUNT 3",
              f"DRAGON 0 {length} " + " ".join(f"{x} {y}" for x, y in BODY[:length]),
              "DRAGON 0 8 6 5 7 5 8 5 9 5 10 5 11 5 12 5 13 5",
              "DRAGON 1 2 12 12 13 12", "END"]
    calls, observations, deaths = {}, [], []

    def reply(dragon_id, block):
        calls[dragon_id] = calls.get(dragon_id, 0) + 1
        rows = block.decode().splitlines()
        if dragon_id == 1:
            pearls = []
            for row in rows:
                words = row.split()
                if len(words) == 4 and all(w.lstrip("-").isdigit() for w in words):
                    x, y, pearl, _ = map(int, words)
                    if (x, y) in BODY and pearl == 1:
                        pearls.append([x, y])
            observations.append({"turn": calls[dragon_id], "pearls": sorted(pearls),
                                 "length": next(r for r in rows if r.startswith("LENGTH "))})
        if calls[dragon_id] > 1:
            return b"ENDTURN\n"
        if dragon_id == 0:
            return action
        return b"MOVE S\nENDTURN\n" if dragon_id == 1 else b"MOVE W\nENDTURN\n"

    engine = EngineModule()
    engine.run(("\n".join(lines) + "\n").encode(), reply,
               lambda dragon_id, r, reason: deaths.append([dragon_id, r, reason]),
               on_notice=lambda _: None, seed=7)
    expected = sorted([list(c) for c in BODY[:length:2]])
    assert observations[0]["pearls"] == expected, (length, observations, expected)
    assert observations[1]["length"] == "LENGTH 9", observations
    return {"donor_length": length, "action": action.decode(),
            "receiver_observations": observations, "deaths": deaths}


if __name__ == "__main__":
    cases = [fixture(length, b"ENDTURN\n") for length in (2, 3, 4, 5, 6)]
    cases.append(fixture(6, b"MOVE W\nENDTURN\n"))
    print(json.dumps({"engine_sha256": hashlib.sha256(WASM_PATH.read_bytes()).hexdigest(),
                      "cases": cases, "passed": True}, indent=2))
