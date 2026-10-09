# R24 Versus Actual H14 Room Mechanism

Both 68-game suites passed full official audits before analysis. Frozen source/binary/map/opponent/engine manifests, replay hashes and final standings were rechecked. No match was launched and no frozen candidate changed.

One matched development base82001 only; descriptive statistics, no significance test or p-value.

| Version | Opponent | Seat | Wins | Confirmed Pickups | Claim Beams | Claim Turns |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| H14 | V3_portal | all | 15/34 | 20701 | 2178 | 2178 |
| H14 | V3_portal | A | 7/17 | 11137 | 961 | 961 |
| H14 | V3_portal | B | 8/17 | 9564 | 1217 | 1217 |
| H14 | xxqztest | all | 16/34 | 22115 | 1966 | 1966 |
| H14 | xxqztest | A | 10/17 | 11859 | 1176 | 1176 |
| H14 | xxqztest | B | 6/17 | 10256 | 790 | 790 |
| R24 | V3_portal | all | 15/34 | 20763 | 2302 | 2302 |
| R24 | V3_portal | A | 7/17 | 11137 | 964 | 964 |
| R24 | V3_portal | B | 8/17 | 9626 | 1338 | 1338 |
| R24 | xxqztest | all | 16/34 | 22063 | 2045 | 2045 |
| R24 | xxqztest | A | 10/17 | 11859 | 1223 | 1223 |
| R24 | xxqztest | B | 6/17 | 10204 | 822 | 822 |

## Paired Trace

| Evidence | Value |
| --- | --- |
| games | 68 |
| identical_action_games | 66 |
| identical_physical_sonar_games | 64 |
| identical_world_action_excluding_sonar_games | 66 |
| first_action_divergence_games | 2 |
| strict_physical_sonar_prefix_first_action_games | 0 |
| same_world_action_prefix_first_action_games | 2 |
| changed_action_maps | ['default'] |
| changed_physical_sonar_maps | ['default'] |
| changed_world_action_maps | ['default'] |

Win-status promotions 0; regressions 0. All changes remain in JSON.

## Default Room Pickups

Counts below refer to the two offline 25-cell single-boundary components. They are actual removed-pearl/landing events, not configured respawn sources, inferred proof activation or guaranteed retained terminal length.

| Opponent | Seat | H14 Outcome | R24 Outcome | H14 Own/Enemy Pickups | R24 Own/Enemy Pickups | Own Claim Turns H14/R24 |
| --- | --- | --- | --- | ---: | ---: | ---: |
| V3_portal | A | loss | loss | 0/84 | 0/84 | 0/3 |
| V3_portal | B | loss | loss | 3/175 | 13/86 | 0/121 |

V3_portal B: first action round40 id5 MOVE S -> MOVE E; full physical/SONAR prefix equal=False, board/action prefix excluding SONAR equal=True; before-head (4, 3), area25, whole component current view=True. First-action confirmed pickups 0 -> 1.

| xxqztest | A | loss | loss | 4/152 | 4/152 | 0/47 |
| xxqztest | B | loss | loss | 3/149 | 2/111 | 0/32 |

xxqztest B: first action round137 id12 MOVE N -> MOVE W; full physical/SONAR prefix equal=False, board/action prefix excluding SONAR equal=True; before-head (26, 17), area830, whole component current view=False. First-action confirmed pickups 0 -> 0.


## Interpretation And Limits

The cap change has no dedicated activation telemetry. Same-prefix first action changes and emitted valid claim packets are narrower observable effects; later harvest and final outcomes are downstream trajectories. More claims must not be presented as a causal explanation for winning. A default-room pickup increase alone cannot establish broad-map or cross-seed improvement.

- One development base82001 only; descriptive statistics, no test/p-value, confirmation or cross-seed claim.
- Room component labels and full bodies are offline geometry, not reconstructed persistent bot memory or proof activation.
- A current whole-component view plus current pearls is a sufficient observable resource witness, not a certificate of future ownership/safety.
- Confirmed pickup requires an observed present-to-absent transition immediately before actor landing on that cell in the same turn; net growth is separate because move payment can offset growth.
- Claim packets establish emitted beams/union-qualified hits, not recipient acceptance, avoidance, occupancy exclusivity or win causality.
- First full physical/SONAR prefix is reported separately from board/action prefix excluding sonar; earlier changed packets can invalidate the former before any action changes.
- Initial round-minus-one position events are asserted against map endpoints, never prepended as body movement or counted as pickups.
- Later post-divergence numerical dragon IDs are never paired across versions as causal identities.
- All negative/default/portals and unaffected-map tuples remain in JSON; no opponent/map subset replaces the registered 68-game screen.

Next decision should use the registered per-opponent/seat and paired outcome gate, retaining any adverse room or redeployment effect; expansion/promotion belongs to the main experiment owner.
