# R26 First-Screen Mechanism

Both68-game suites passed complete official audits before analysis. Frozen source/binary/map/opponent/engine manifests, replay hashes, canonical prefixes and final standings were rechecked. No matches or policy edits were performed.

| Evidence | Count |
| --- | ---: |
| games | 68 |
| identical_action_games | 22 |
| identical_physical_sonar_games | 22 |
| committed_SHORTER_HEAD_TRADE_changes | 705 |
| games_with_committed_changes | 46 |
| first_action_divergence_games | 46 |
| strict_prefix_first_action_changes | 46 |
| mechanism_consistent_strict_prefix_first_changes | 46 |
| other_first_action_divergences | 0 |
| logged_replacements_observable_geometry_passed | 705 |
| logged_replacements_actual_projection_exact | 705 |
| logged_after_zero_matches_observed | 705 |
| logged_complete_heads_matches_observed | 705 |
| H14_first_actor_death_before_next_action | 45 |
| H14_first_actor_same_or_next_round_head_deaths | 45 |
| H14_first_actor_same_or_next_round_deaths | 45 |
| H14_first_actor_later_only_pickups | 2 |
| H14_first_actor_alive_at_end | 0 |
| R26_first_actor_death_before_next_action | 0 |
| R26_first_actor_same_or_next_round_head_deaths | 3 |
| R26_first_actor_same_or_next_round_deaths | 3 |
| R26_first_actor_later_only_pickups | 122 |
| R26_first_actor_alive_at_end | 2 |

Committed logs count executed replacement proposals. Strict-prefix first changes independently compare actual H14 proposals and R26 actions with necessary current visible geometry. Actor histories and paired wins are separate observations.

| Opponent | Seat | H14 Wins | R26 Wins | Committed Changes | Strict First Changes |
| --- | --- | ---: | ---: | ---: | ---: |
| V3_portal | all | 15/34 | 16/34 | 295 | 23 |
| V3_portal | A | 7/17 | 9/17 | 181 | 11 |
| V3_portal | B | 8/17 | 7/17 | 114 | 12 |
| xxqztest | all | 16/34 | 17/34 | 410 | 23 |
| xxqztest | A | 10/17 | 10/17 | 200 | 11 |
| xxqztest | B | 6/17 | 7/17 | 210 | 12 |

## Strict-Prefix First Changes

- V3_portal/australia/seed82001/A: r83 id16 MOVE W -> MOVE S; H14/R26 immediate head deaths True/False; outcome loss -> win.
- V3_portal/australia/seed82001/B: r63 id27 MOVE E -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/autarky/seed82002/A: r69 id16 MOVE E -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> win.
- V3_portal/autarky/seed82002/B: r9 id9 MOVE W -> MOVE E; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/default/seed82003/A: r169 id31 MOVE W -> MOVE E; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/default/seed82003/B: r93 id5 MOVE S -> MOVE E; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/devil/seed82004/A: r106 id18 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome win -> win.
- V3_portal/devil/seed82004/B: r43 id24 MOVE W -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> loss.
- V3_portal/dilemma/seed82006/A: r28 id4 MOVE N -> MOVE E; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/dilemma/seed82006/B: r38 id14 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/queen_of_spades/seed82007/B: r236 id90 MOVE W -> MOVE E; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/schooltime/seed82008/A: r22 id10 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome win -> win.
- V3_portal/schooltime/seed82008/B: r41 id3 MOVE N -> MOVE S; H14/R26 immediate head deaths True/False; outcome win -> loss.
- V3_portal/slithery_fight/seed82009/A: r90 id112 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/slithery_fight/seed82009/B: r54 id67 MOVE E -> MOVE W; H14/R26 immediate head deaths True/False; outcome loss -> win.
- V3_portal/trophy/seed82011/A: r76 id7 MOVE E -> MOVE W; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/trophy/seed82011/B: r59 id6 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/islands/seed82012/A: r45 id6 MOVE N -> MOVE E; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/islands/seed82012/B: r123 id34 MOVE E -> MOVE S; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- V3_portal/maze/seed82013/A: r406 id116 MOVE S -> MOVE W; H14/R26 immediate head deaths False/False; outcome win -> win.
- V3_portal/stripes/seed82014/B: r215 id49 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome win -> win.
- V3_portal/unsw/seed82016/A: r76 id131 MOVE NN -> MOVE N; H14/R26 immediate head deaths True/True; outcome win -> win.
- V3_portal/unsw/seed82016/B: r71 id72 MOVE S -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> win.
- xxqztest/australia/seed82001/A: r122 id41 MOVE W -> MOVE E; H14/R26 immediate head deaths True/False; outcome loss -> win.
- xxqztest/australia/seed82001/B: r97 id23 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> win.
- xxqztest/autarky/seed82002/A: r24 id8 MOVE N -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> loss.
- xxqztest/autarky/seed82002/B: r25 id11 MOVE W -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> loss.
- xxqztest/default/seed82003/A: r20 id2 MOVE W -> MOVE N; H14/R26 immediate head deaths True/True; outcome loss -> loss.
- xxqztest/default/seed82003/B: r92 id23 MOVE E -> MOVE W; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- xxqztest/devil/seed82004/A: r61 id18 MOVE E -> MOVE W; H14/R26 immediate head deaths True/False; outcome win -> win.
- xxqztest/devil/seed82004/B: r30 id3 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- xxqztest/dilemma/seed82006/A: r81 id4 MOVE E -> MOVE N; H14/R26 immediate head deaths True/True; outcome win -> win.
- xxqztest/dilemma/seed82006/B: r40 id19 MOVE N -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> win.
- xxqztest/queen_of_spades/seed82007/A: r137 id48 MOVE E -> MOVE S; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- xxqztest/queen_of_spades/seed82007/B: r105 id13 MOVE N -> MOVE W; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- xxqztest/schooltime/seed82008/A: r100 id48 MOVE W -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> loss.
- xxqztest/schooltime/seed82008/B: r82 id30 MOVE E -> MOVE S; H14/R26 immediate head deaths True/False; outcome win -> win.
- xxqztest/slithery_fight/seed82009/A: r46 id10 MOVE E -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> win.
- xxqztest/slithery_fight/seed82009/B: r60 id56 MOVE S -> MOVE N; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- xxqztest/trophy/seed82011/A: r119 id83 MOVE S -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> win.
- xxqztest/trophy/seed82011/B: r58 id6 MOVE E -> MOVE S; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- xxqztest/islands/seed82012/B: r68 id7 MOVE N -> MOVE W; H14/R26 immediate head deaths True/False; outcome loss -> loss.
- xxqztest/maze/seed82013/A: r355 id447 MOVE N -> MOVE E; H14/R26 immediate head deaths True/False; outcome win -> win.
- xxqztest/maze/seed82013/B: r265 id319 MOVE S -> MOVE W; H14/R26 immediate head deaths True/False; outcome loss -> win.
- xxqztest/unsw/seed82016/A: r68 id89 MOVE W -> MOVE N; H14/R26 immediate head deaths True/False; outcome win -> win.
- xxqztest/unsw/seed82016/B: r46 id54 MOVE W -> MOVE S; H14/R26 immediate head deaths True/False; outcome win -> win.

Paired win-status promotions 7; regressions 5. Full committed activations by game, map/side/opponent summaries and actor histories remain in JSON.

## Limits

- One audited development base82001, ranked17 A/B and two unchanged opponents; no confirmation, cross-seed or significance claim.
- Packed LOG records attach to the following official action of the same actor; every log must commit MOVE in its logged direction. Logging counts replacements, not rejected proposals.
- Only the first action change with equal physical/SONAR/action prefix digest/count, actor/body/round/population is a paired action intervention. Later divergent-prefix activations are descriptive trajectories.
- Original proposals are not in SHORTER_HEAD_TRADE logs. Original trigger and no-added-head exposure are independently verified only at strict-prefix H14/R26 first action changes.
- Replay geometry uses complete bodies restricted to radius3 current visibility and ordinary-tail boundary proof. It does not reconstruct segment-facing input metadata, private ownSeg/trail, score values, friend hard/soft SONAR protection or the donation flag; actual raw predecessor-chain fixtures are separate evidence.
- The candidate rejects newly exposed partial enemy heads; the first-change geometry compares all current visible head masks, including incomplete bodies. Partial bodies never authorize an exact shorter-length witness.
- A first ordinary free step onto a head is a local legal trade witness, not a promise about next actor order, intervening occupancy, hidden enemies or longer/paid/portal attacks.
- A current continuation is collision-before-tail-removal ordinary living space, not guaranteed next-turn behavior or survival.
- Pickups require a present-to-absent pearl event immediately followed by that actor landing; positive net length gain is separate from pickup when payment offsets growth.
- Actor deaths, later pickups and end length after the first same-prefix change are downstream observations; final outcome changes are paired results, not proof this one local event caused a win.
- Initial repeated position events are checked and skipped. Negative, unaffected, unmatched and geometry-inconsistent pairs remain in JSON; outputs are never overwritten.
