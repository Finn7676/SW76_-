# R24 Room-Bound Focused Evidence

This verification is source-bound and does not run competitive games. It
compares the frozen R24 candidate with the unchanged H14 source using the
same reconstructed observable states. It deliberately does not claim that
the R18 r55/r39 snapshots are native replay memory: remembered topology,
sonar and pearl timers are not fully reconstructible from those snapshots.

## Source-bound controls

`room_observable_tests.cpp.txt` was compiled with `g++-10 -std=c++20 -O2`.
The candidate binary passed four cases (r55/r39 times current-plus-entry
view and self-observed cell-union view). The H14 control was compiled with
`ROOM_BOUND_CANDIDATE=0` and passed the same four cases.

| source | bound | r55 action | r39 action | room PID |
| --- | ---: | --- | --- | ---: |
| H14 | 24 | `MOVE N` | `MOVE N` | `-1` |
| R24 | 49 | `MOVE S` (+1 length) | `MOVE N` | `0` |

For each candidate case, all actions had `split=0` and living projection.
After the current-only check, every edge outside the head's 7x7 visible
neighborhood was replaced with `UNKNOWN`; R24 still proved PID 0 and H14
still rejected the 25-cell room. This isolates the cap effect from the
historical entry-view edges.

## Native 1.2.7 round-zero controls

`room_native.py` constructs a fresh default-map input and runs the official
1.2.7 engine for one action turn. It preserves default topology and uses
mirror-symmetric fixed pearl layouts. A fresh probe process receives the
exact native initialization plus round-zero input and checks the production
proof result, so no hand-built memory is used for the proof assertion.

The native run has four cases (H14/R24 x r55/r39 layout), zero sonar
messages, 49 visible tiles, dragon 5 at `(4,4)`, length 2. H14 proves no
room and enters the paired endpoint `(24,7)` in both layouts. R24 proves
PID 0; it takes `(4,5)` and grows to length 3 in r55, while unchanged
ordinary scoring still enters `(24,7)` in r39. No dragon 5 death occurs
before the deliberate round-one `ENDTURN` termination. These are input/action
controls, not match outcomes.

Artifacts and exact hashes are recorded in `room_native.json`; replay/map/
input/output files are in `verification/room_native_artifacts/`. The
fixture intentionally does not read development or reserved competitive
results.
