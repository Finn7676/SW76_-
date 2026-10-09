# R20 First-Screen Mechanism

All68 R20 and paired H14 replays passed prior official outcome audits; this analysis rechecked frozen source/binary/map/engine manifests and every replay SHA256. Native unswbc1.2.7, base82001, ranked17 maps, both seats, unchanged opponents. No new match ran and no bot was modified.

| Evidence | Count |
| --- | ---: |
| same_outcome | 68 |
| same_standings | 67 |
| same_completed_rounds | 67 |
| same_queen_deaths | 67 |
| same_actions | 67 |
| same_queen_actions | 67 |
| same_physical_sonar_history | 67 |
| R20_witness_logs | 1 |
| R20_queen_actions | 11733 |

R20 first-screen wins are V3_portal15/34 and xxqztest16/34, identical to H14. Paired physical/SONAR histories compare round/turn markers, pearl changes, committed actions, moves, splits, deaths and sonar fields, excluding names, time, processor statistics and diagnostics.

Changed-action or committed-witness games: 1.

- xxqztest / islands / seed82012 / B: logs1; first change{'index': 449, 'H14': [30, 1, 'MOVE N'], 'R20': [30, 1, 'MOVE NN']}

Mechanism logs were read directly from packed replay kind6 and attached to the following official kind4 queen action; empty CLI logs do not establish nonactivation. The other67 games have identical committed action and physical/SONAR histories and zero witness logs. That establishes no replacement in those67 games, not the absence of rejected/eligible proposals, because rejection telemetry is not emitted.

## Islands B Change

The changed islands82012 B replay shares4240 canonical physical/SONAR events with H14 before queen1's round30 action. H14 action4267 is `MOVE N`, placing its length6 head at `(46,10)`; enemy16 at `[(47,10),(47,9)]` subsequently commits `MOVE W` at event4359, and both enemy16 and queen1 die `head` during that turn. In R20, the committed witness log4267 reports before1/after0, one complete enemy and6 nodes; action4268 is `MOVE NN`, placing the same-length queen head at `(46,9)`. Enemy16 instead chooses `MOVE E` and lives. This is not a replay of the development fixture's enemy-body-collision outcome.

R20 queen1 round31 moves W to `(45,9)`, then round32 moves S to `(45,10)`. Friendly id12 has body `[(31,16),(31,17),(32,17),(32,18)]` and subsequently commits `MOVE N` at event4716, crossing a portal to that same `(45,10)` landing. Queen1 and friendly12 both die `head`, events4717/4721. H14's id12 made the same portal action at event4694 after its queen was already dead and landed successfully. Thus R20 delayed the queen's death from30 to32, but the later loss is a friendly incoming-portal collision, not another demonstrated first-step enemy witness.

H14 loses by elimination after194 rounds, final B population/total0. R20 reaches500 rounds and loses on length with B two length2 dragons (total4), versus xx population28, longest54 and total281; both queens are dead. Those are observed downstream differences, not evidence that the local correction increases win probability. Before round30 both have B population5/total17/queen6. R20 retains queen6 before31/32, but after33 has B population4/total9/queen0. H14 before33 has B population5/total13/queen0. The incoming friendly collision also consumes a teammate, so local enemy avoidance is not a general team-survival guarantee.

`r20_islands_witness_slice.{py,json}` stores the hash-checked paired events, exact bodies, shared prefix and unpadded checkpoints. Replay SHA256 H14 is `fd127491fadc16bc832ad79c4427d918aca3f4785ef4508a4bfc094ef703375b`; R20 is `7106610fddda33580282fcb5eb69a94f011df1a43d6152d8aba8bd64f813ca2a`.

The source-bound official fixture uses dilemma84006 A round199, while this first screen includes dilemma82006. That fixture proves a real local N-to-NN correction under its specific input; it does not establish broad coverage or win-rate gain on82001. The first screen activates once among11733 recorded queen actions and produces no win conversion. Do not expand R20 based on that alone or generalize zero first-step witnesses into queen safety. All partial/hidden enemies, portals, longer routes and intervening turns remain unknown. The new friendly-portal collision deserves its own input-proven diagnosis before any broader guard is implemented.

Provenance and per-game hashes are in `r20_firstscreen_mechanism.json`; script is `r20_firstscreen_mechanism.py`. No reserved confirmation seed was read.
