# R25 First-Screen Mechanism

Both68-game suites passed official audits before analysis. Frozen source/binary/map/opponent/engine manifests, replay hashes, canonical prefixes and final standings were rechecked. No match or policy edit was performed.

| Evidence | Count |
| --- | ---: |
| games | 68 |
| identical_action_games | 61 |
| identical_physical_sonar_games | 61 |
| first_action_divergence_games | 7 |
| strict_prefix_first_action_changes | 7 |
| mechanism_consistent_observable_first_changes | 7 |
| other_first_action_divergence_games | 0 |
| H14_first_actor_next_round_self_deaths | 7 |
| H14_first_actor_same_or_next_round_deaths | 7 |
| H14_first_actor_later_only_pickups | 0 |
| H14_first_actor_alive_at_end | 0 |
| R25_first_actor_next_round_self_deaths | 0 |
| R25_first_actor_same_or_next_round_deaths | 0 |
| R25_first_actor_later_only_pickups | 5 |
| R25_first_actor_alive_at_end | 0 |

The consistent first changes are observed committed action differences with the registered necessary visible geometry. No private activation telemetry exists. Local escape and later collector outcomes are reported separately from the game's win status.

| Opponent | Seat | H14 Wins | R25 Wins | H14/R25 Own Pickups | H14/R25 Nonqueen Deaths |
| --- | --- | ---: | ---: | ---: | ---: |
| V3_portal | all | 15/34 | 16/34 | 20701/21049 | 6646/6748 |
| V3_portal | A | 7/17 | 7/17 | 11137/11309 | 3455/3486 |
| V3_portal | B | 8/17 | 9/17 | 9564/9740 | 3191/3262 |
| xxqztest | all | 16/34 | 15/34 | 22115/22111 | 7038/7034 |
| xxqztest | A | 10/17 | 10/17 | 11859/11593 | 3812/3715 |
| xxqztest | B | 6/17 | 5/17 | 10256/10518 | 3226/3319 |

## Same-Prefix First Changes

- V3_portal / schooltime / seed82008 / A: r263 id243 MOVE E -> MOVE N; strict-prefix=True; mechanism-consistent=True; outcome win -> win.
  H14: first projection exact=True; next-round self death=True; later-only pickups=0; death={'event': 92600, 'round': 264, 'id': 243, 'reason': 'self', 'active_turn': 243, 'body_before': [(43, 33), (42, 33), (41, 33)]}; alive-at-end=False.
  R25: first projection exact=True; next-round self death=False; later-only pickups=0; death={'event': 117484, 'round': 313, 'id': 243, 'reason': 'self', 'active_turn': 243, 'body_before': [(41, 32), (42, 32)]}; alive-at-end=False.
- V3_portal / slithery_fight / seed82009 / B: r289 id138 MOVE W -> MOVE E; strict-prefix=True; mechanism-consistent=True; outcome loss -> win.
  H14: first projection exact=True; next-round self death=True; later-only pickups=0; death={'event': 226002, 'round': 290, 'id': 138, 'reason': 'self', 'active_turn': 138, 'body_before': [(47, 19), (48, 19), (48, 20)]}; alive-at-end=False.
  R25: first projection exact=True; next-round self death=False; later-only pickups=1; death={'event': 228891, 'round': 293, 'id': 138, 'reason': 'self', 'active_turn': 138, 'body_before': [(51, 20), (51, 19), (50, 19)]}; alive-at-end=False.
- V3_portal / trauma / seed82010 / B: r410 id121 MOVE W -> MOVE N; strict-prefix=True; mechanism-consistent=True; outcome win -> win.
  H14: first projection exact=True; next-round self death=True; later-only pickups=0; death={'event': 62140, 'round': 411, 'id': 121, 'reason': 'self', 'active_turn': 121, 'body_before': [(33, 17), (34, 17), (34, 18)]}; alive-at-end=False.
  R25: first projection exact=True; next-round self death=False; later-only pickups=1; death={'event': 62655, 'round': 413, 'id': 121, 'reason': 'self', 'active_turn': 121, 'body_before': [(36, 16), (35, 16), (34, 16)]}; alive-at-end=False.
- xxqztest / schooltime / seed82008 / B: r192 id288 MOVE W -> MOVE N; strict-prefix=True; mechanism-consistent=True; outcome win -> loss.
  H14: first projection exact=True; next-round self death=True; later-only pickups=0; death={'event': 78429, 'round': 193, 'id': 288, 'reason': 'self', 'active_turn': 288, 'body_before': [(38, 15), (39, 15), (39, 16)]}; alive-at-end=False.
  R25: first projection exact=True; next-round self death=False; later-only pickups=0; death={'event': 79142, 'round': 194, 'id': 288, 'reason': 'body', 'active_turn': 288, 'body_before': [(40, 14), (39, 14)]}; alive-at-end=False.
- xxqztest / trauma / seed82010 / B: r344 id137 MOVE S -> MOVE E; strict-prefix=True; mechanism-consistent=True; outcome win -> win.
  H14: first projection exact=True; next-round self death=True; later-only pickups=0; death={'event': 63806, 'round': 345, 'id': 137, 'reason': 'self', 'active_turn': 137, 'body_before': [(34, 22), (34, 21), (33, 21)]}; alive-at-end=False.
  R25: first projection exact=True; next-round self death=False; later-only pickups=1; death={'event': 64835, 'round': 348, 'id': 137, 'reason': 'self', 'active_turn': 137, 'body_before': [(36, 21), (36, 22), (35, 22)]}; alive-at-end=False.
- xxqztest / unsw / seed82016 / A: r156 id316 MOVE N -> MOVE S; strict-prefix=True; mechanism-consistent=True; outcome win -> win.
  H14: first projection exact=True; next-round self death=True; later-only pickups=0; death={'event': 99623, 'round': 157, 'id': 316, 'reason': 'self', 'active_turn': 316, 'body_before': [(32, 35), (32, 36), (33, 36)]}; alive-at-end=False.
  R25: first projection exact=True; next-round self death=False; later-only pickups=1; death={'event': 107937, 'round': 166, 'id': 316, 'reason': 'self', 'active_turn': 316, 'body_before': [(34, 36), (34, 35), (33, 35)]}; alive-at-end=False.
- xxqztest / unsw / seed82016 / B: r451 id563 MOVE E -> MOVE W; strict-prefix=True; mechanism-consistent=True; outcome win -> win.
  H14: first projection exact=True; next-round self death=True; later-only pickups=0; death={'event': 352137, 'round': 452, 'id': 563, 'reason': 'self', 'active_turn': 563, 'body_before': [(61, 34), (60, 34), (60, 33)]}; alive-at-end=False.
  R25: first projection exact=True; next-round self death=False; later-only pickups=1; death={'event': 355804, 'round': 456, 'id': 563, 'reason': 'self', 'active_turn': 563, 'body_before': [(61, 34), (60, 34), (60, 33)]}; alive-at-end=False.

Paired win-status promotions 1; regressions 1. All68 pairs, full first-actor event witnesses and downstream histories remain in JSON.

## Limits

- One development base82001, all17maps, both seats and both unchanged opponents; no held-out, cross-seed, p-value or generalization claim.
- The first committed change requires byte-equivalent canonical physical/SONAR/action prefix hashes and counts plus actor/body/round/population agreement; later divergent prefixes are not causal pairs.
- Observable geometry uses true replay bodies restricted to the current radius3 view; it does not reconstruct private trail, current segment facing metadata, inherited score values, persistent memory, donation flag or accepted protection/SONAR status.
- No dedicated R25 telemetry exists. A strict-prefix change with necessary observable gates is mechanism-consistent, not proof that a private R25 call or every private admission gate activated; no complete activation count is inferred.
- Continuation is a current-view next-turn projection with consumed pearls removed and collision checked before tail removal. It is not a same-turn sprint or a promise of future hidden-body/actor safety.
- A pickup is counted only for a present-to-absent pearl transition immediately preceding the same actor landing; positive net length gain is separate because payment can offset growth.
- The selected actor exists identically at the first same-prefix action, but subsequent pickups/death/end length are downstream trajectory observations. Later child IDs are not paired across versions as causal identities.
- Fresh native and scripted local rule fixtures are separate source/legality evidence, not trajectory benefit or a causal explanation for wins.
- Initial round-minus-one position updates are checked against map endpoints and never prepended as movement or counted as pickups. All negative and unaffected pair tuples remain in JSON.
