# Resource-Room Cap Geometry Diagnosis

Independent map-only and local-action diagnosis, 2026-10-08. No competitive
games were started and no reserved-seed outcomes were read. H14 production
source remained unchanged at SHA-256
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
The R24 production source was created by root, not this diagnostic task.

## Definition And Method

The authoritative map pool is `benchmark.resolve_maps(["all"])`: all 17
ranked maps are verified byte-for-byte against the installed official 1.2.7
bundle. The existing R21 topology parser decodes every direction of every
tile; reverse-edge equality and exactly two endpoints per portal ID are
checked before connected-component analysis.

Components traverse only ordinary open edges (`kind=0`), including wrapped
map seams. Kelp and portal edges are excluded from traversal. Portal
incidences are deduplicated using the same E/S canonical edge as H14;
"single boundary" means one canonical edge, not one distinct portal ID or
one incident side. An internal portal edge can have both sides in the same
ordinary component. Both newly eligible default rooms have this property.

Configured resource tiles are `TILE maxRounds>0`. The official bundled
viewer's map parser labels the two `TILE` parameters `minRounds/maxRounds`;
they are respawn intervals, not initial pearl-presence flags. This static
resource definition is only a geometry filter. The production proof still
requires remembered or observed finite `pearlDue`; current availability,
corpse pearls and actual harvesting rates are not inferred from respawn
parameters.

## Raw Map Table

| Map | Ordinary components | Single-boundary resource areas | Eligible at 24 | Eligible at 49 |
| --- | ---: | --- | ---: | ---: |
| australia | 3 | 12 x 2 | 2 | 2 |
| autarky | 9 | none | 0 | 0 |
| default | 12 | 25 x 2 | 0 | 2 |
| devil | 1 | none | 0 | 0 |
| portals | 46 | 1 x 16; 4 x 12 | 28 | 28 |
| dilemma | 5 | none | 0 | 0 |
| queen_of_spades | 7 | 16 x 2 | 2 | 2 |
| schooltime | 44 | none | 0 | 0 |
| slithery_fight | 5 | 4 x 2 | 2 | 2 |
| trauma | 11 | none | 0 | 0 |
| trophy | 1 | none | 0 | 0 |
| islands | 23 | none | 0 | 0 |
| maze | 13 | 5 x 2 | 2 | 2 |
| stripes | 15 | 1 x 4; 3 x 4; 4 x 2 | 10 | 10 |
| tower_defense | 1 | none | 0 | 0 |
| unsw | 1 | none | 0 | 0 |
| weakhold | 1 | none | 0 | 0 |
| Total | 198 | 48 rooms | 46 | 48 |

Exactly two rooms become geometrically eligible: default's complete 5x5
regions `(2,2)..(6,6)` and `(25,25)..(29,29)`, with portal PID 0 and PID 1.
Each has 25 configured resource tiles: 23 intervals `[1,255]`, two intervals
`[1,3849]`. Their 50 configured sources are not 50 currently available
pearls. Each region has nine current-view centers that cover every room
tile: `(3..5,3..5)` and `(26..28,26..28)` respectively.

## Source-Bound Local Controls

The diagnostic fixture copies only the production proof function with a
cap argument and first asserts cap24 equality with H14's function for all
1,024 start cells. R18's source-bound r55/r39 observable states supply
current body/pearls and either current-plus-entry view or self-observed
cell-union topology. Every case also masks all remembered edges outside
the current 7x7 neighborhood: proof results remain identical, so proving
these rooms does not require the remote entry topology.

| Observable case | Cap24 room/action | Cap49 room/action | Cap49 projected gain |
| --- | --- | --- | ---: |
| r55 current-plus-entry view | -1 / N | 0 / S | 1 |
| r39 current-plus-entry view | -1 / N | 0 / N | 0 |
| r55 self-observed cell union | -1 / N | 0 / S | 1 |
| r39 self-observed cell union | -1 / N | 0 / N | 0 |

All eight final action projections survive locally. They do not reconstruct
complete native replay memory: decoded sonar, all past pearl timers and
production state are not known. They are counterfactual observable-state
fixtures, not reconstructed historical matches.

The root-created R24 candidate now also has permanent macro-controlled
`verification/room_observable_tests.cpp.txt` and a separate official native
fixture. The native fixture uses fresh processes at round 0, keeps default
topology, and supplies mirror-symmetric fixed pearl layouts. Its genuine
input has 49 tiles, zero sonar messages, id5 length2 at `(4,4)`. Native
source-bound proof is H14 `-1` versus R24 PID0. Native r55 is H14 N to
`(24,7)` versus R24 S to `(4,5)`, with next-round lengths 2 versus 3;
native r39 remains N for both. Four cases pass. Round-one deaths are
deliberate `ENDTURN` termination, not competitive outcomes. Exact raw
inputs, outputs, replay/map artifacts and hashes are recorded in
`tyhh_test/tyhh_qos_robust_room_bound_v127/verification/room_native.json`.

## Findings And Falsifier

1. The cap24 exclusion is real and excludes two compact default rooms
   whose complete boundaries can be known from current vision. The single
   cap change is a meaningful default-specific coverage hypothesis, not
   evidence of broad-map superiority; all other maps admit the same
   configured resource rooms under either cap.
2. Cap49 is an area limit, not a geometric shape or visibility guarantee.
   Every unknown edge, additional canonical portal boundary and oversized
   component still rejects. No configured qualifying room in these 17
   maps has area 26..49; a cap of 25 would cover the same static configured
   room set, though runtime resource evidence can include corpse pearls.
3. Room proof suppresses early forced departure and emits an occupancy
   claim; it does not prohibit recrossing or certify exclusive ownership.
   The unchanged ordinary early-portal bonus remains `+360`. r39's N
   recross is an explicit negative control, not an implementation failure.
4. Additional occupancy claims may suppress useful backup scouts or
   redeployment. A matched 68-game development screen must check real
   proof/action changes, default harvesting and loss transitions; a
   favorable local action alone is insufficient to advance or freeze.

The analysis skill supplied the raw-table and observation/interpretation
separation; no statistical inference or seed-based claim is made. Full
component cells, respawn counts, boundaries, view centers and local output
are preserved in `room_cap_diagnosis.json`. Reproduction uses
`room_cap_geometry.py` and `room_cap_local_fixture.cpp.txt`; production
strategy changes and competitive scheduling remain root's responsibility.
