# Default Development Portal Diagnosis

Read-only diagnosis, 2026-10-05. Sources: frozen H14 versus xxqztest, bases
82001/83001/84001, default actual seeds 82003/83003/84003, both seats. All
six games are retained, including H14's 83003 side-B win. No holdout seed,
new competitive match, engine/map change or bot change was used.

Machine evidence is `default_portal_probe_v3.json`; generator is
`default_portal_probe.py`. Older v1/v2 JSON is preserved. Replay hashes are
embedded per game. Packed replay events reconstruct exact current bodies and
pearls. Current 7x7 observable cells are selected geometrically with the
documented radius3; historical self-known cells are a conservative lower
bound. Sonar packets, hidden enemy intentions, and exact bot memory are not
reconstructed. Full-state room membership is diagnosis only, not bot input.

## Resource Slice

There are two 25-cell rich rooms, coordinates 2..6 by 2..6 and 25..29 by
25..29. The table counts their pearl-removal events attributed to the actor's
team and head-turn occupancy over the full replay; it does not equate map
presence or a generated pearl with a won resource. Checkpoints are before
the numbered round starts, therefore after rounds0..99 and0..199 respectively.

| Seed | H14 Seat | Result | Rich Pearl Removals H14 / xx | Rich Head Turns H14 / xx | Total Length At100 H14 / xx | Total Length At200 H14 / xx |
| --- | --- | --- | --- | --- | --- | --- |
| 82003 | A | loss | 4 / 152 | 56 / 1546 | 25 / 43 | 26 / 83 |
| 82003 | B | loss | 3 / 149 | 39 / 1981 | 24 / 42 | 38 / 83 |
| 83003 | A | loss | 89 / 59 | 590 / 957 | 25 / 38 | 28 / 63 |
| 83003 | B | win | 159 / 1 | 1712 / 13 | 48 / 23 | 88 / 25 |
| 84003 | A | loss | 71 / 85 | 902 / 938 | 27 / 56 | 36 / 70 |
| 84003 | B | loss | 82 / 90 | 751 / 730 | 34 / 38 | 25 / 48 |

This is not a universal failure to enter rooms: H14 enters rich room0 before
xx on 82003 sideB (r23 and r26 versus xx r34), but its three scouts all leave
by r43. The 83003 sideB win maintains rich-room dominance. Rich-room removals
are greater for H14 in the 83003 sideA loss, so this mechanism cannot explain
all losses or replace queen/endgame analysis. No statistical superiority
claim is made from this six-game descriptive slice.

## Action Evidence

1. Seed82003, H14 sideB, id5, round39, turn event5123, action event5124,
   crossing event5125: body [(4,4),(3,4)], LEN2. Final action MOVE N crosses
   portal0 to (24,7), abandoning the rich room. Current view has pearl(5,3),
   no enemy part or head, own two segments and ally id10 at (5,5),(4,5),(4,6).
   Entire route E(5,4) -> N pearl(5,3) -> N continuation(5,2) is ordinary,
   currently visible and empty. One MOVE E is immediately living; later
   steps must be recomputed and are not a commitment or a free sprint.
   This actor had crossed portal0 at r26/event3772, from(24,7) S to(4,4).
   The entry edge's learnedExit is therefore set at the next turn by H14's
   pendingPortalKey logic; the reverse edge's learnedExit need not be set.

2. Seed84003, H14 sideB, id5, round55, turn event7361, action event7362,
   crossing event7363: same body [(4,4),(3,4)], LEN2. Final MOVE N crosses
   portal0 to(24,7). Current view contains only the actor's two segments,
   no enemy parts, and pearls(4,5),(6,4),(6,6). Ordinary MOVE S immediately
   collects(4,5) to LEN3 with clear ordinary E continuation(5,5). Alternative
   E(5,4) -> E pearl(6,4) -> N(6,3) also has visible empty geometry.
   The actor previously crossed portal0 at r46/event6168 from(24,8) N to
   (4,3), so this is a repeat of the same portal ID even though its exit
   edge differs from the earlier entry edge. Tracking a current edge's
   learnedExit alone would miss this case.

3. Seed83003, H14 sideA, child27, round77, turn event9900, action event9901,
   crossing event9902: body [(4,3),(5,3)], LEN2. Final MOVE S crosses portal0
   to(24,8) with seven current room pearls and no enemy parts. W(3,3) -> S
   pearl(3,4) -> E pearl(4,4) is currently visible/ordinary/empty. However,
   child27 has never itself crossed portal0. A policy limited to portals
   previously crossed by this same actor must leave this case unchanged.
   It cannot silently treat parent's travel or inferred sonar as its own.

The scan found no same-actor, consecutive-round portal recross without a
pickup before round80 in these six games. These witnesses concern premature
departure several rounds after entry, not a demonstrated immediate bounce.

## Mechanism And Hypothesis

H14 main's earlyPortalDir branch runs before routine split and pearl scoring
for every round<80, including portals already crossed by this dragon. LEN2
freeHarvest has only one free step, so it cannot replace these one-step
proposals. The replays expose the emitted action but not the planner proposal;
the freeHarvest source rule rules out its rewrite for these LEN2 cases.

Room claims cannot rescue these witnesses: provenResourceRoom rejects an
ordinary-open component on discovering its25th cell because the generic
bound is24. Both actual rich rooms have25 cells. Changing24 to25 would be
map-tuned and would conflate claiming with the repeated exploration problem.
This diagnosis does not propose that change.

R18 candidate hypothesis: make **early exploration privilege single-use per
portal ID per dragon**. On an actually confirmed local crossing, record that
portal ID. The early forced branch uses only IDs this dragon has not crossed;
repeat traversals remain legal and fall through to the unchanged resource/
survival scorer. First discovery, all long/queen portal checks, claims, split,
feeding and freeHarvest remain unchanged. A new child begins with its own
empty crossing history unless a separately tested rule says otherwise.

This differs from R1 approach routing to an unexplored remote portal, R3/R11
late donor rendezvous and R12 pearl reservation. Its prediction is fewer
resource-room departures by already-exploring scouts, not faster first entry.
The relevant state is local observable travel, not full-state room membership.

Important falsifier: the unchanged scorer still awards round<80 portal +360
and rewards larger controlled space. Merely removing forced priority does
not prove the scorer will take either visible pearl route. Before a match,
source-bound exact fixtures must show the final alternative after all guards;
if the original scorer still recrosses, this minimal R18 mechanism is not
validated, and a separately registered resource-vs-repeat arbitration factor
would be required. No bot implementation has been made by this diagnosis.
