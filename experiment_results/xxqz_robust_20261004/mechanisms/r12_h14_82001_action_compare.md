# R12/H14 82001 Action And Pearl Comparison

Only robust_h14_82001 and robust_pearl_reserve_82001 completed development suites; no reserved seeds read

Both suites passed the stored audit; current frozen source, map and engine hashes and all 136 replay hashes were verified.

| Opponent | Paired Games | Same Actions | Same Pearl Collections | Same World Changes | Same Final Standings |
|---|---:|---:|---:|---:|---:|
| V3_portal | 34 | 34 | 34 | 34 | 34 |
| xxqztest | 34 | 34 | 34 | 34 | 34 |

All 1,387,926 official actions match, including 624,428 own actions, complete
move direction lists, split sizes and suicide actions. Own pearl collections
match at 42,816; round >=350 short nonqueen collections match at 12,213.
Raw replay byte hashes need not match: replay headers embed different frozen
candidate binary paths. The identity claim applies to the canonical semantic
streams listed above, not to raw replay bytes.

## Representative Gate Bottlenecks

- Against xxqztest, default A has 183 late own actions and none pass the
  short-nonqueen/population gate. Default B has 835 late own actions but zero
  quiet-visible-pearl/large-receiver co-presence upper-bound opportunities.
- Against xxqztest, portals A has 3,429 late own actions, 57 pass the
  visible-quiet upper bound, and only two have a visible pearl plus an
  actual-size receiver. Both occur at rounds 498/499, actor 651 length6,
  and retain the same north move. Portals B has zero such co-presence.
- At portals A round498, actor651 starts at (2,6); receiver782 actual length9
  is at (0,3). At round499, actor651 starts at (2,5), receiver782 at (0,4).
  The entire action stream in both versions has SHA-256
  `3e6adc8fef88cf5b0f5b5c0d9f446b957da1b8a6bb069a48acad5dc5602c7201`.

The broad co-presence upper bound is 2,292 of 189,544 late own actions
(1.21%); 179 are directly preempted by logged queen/consolidation feeding.
The remaining 2,113 have not yet passed sonar quiet history, reported receiver
size, a two-step collection path or a safe nonpearl escape. This supports a
narrow opportunity diagnosis, not an exact internal activation count.

## Production Call Chain

`queenFeedAction` and `consolidateAction` execute before `takeTurn`
(`main.cpp:1540`). Reservation changes only immediate scored pearl pickup
(`main.cpp:1422`) and scored free-harvest extensions (`main.cpp:913`). The
pearl BFS goal set and pearl-distance bonus remain unchanged
(`main.cpp:1308`, `main.cpp:1421`). Free harvest exits for split, donation,
multistep proposals or fewer than two initial free steps (`main.cpp:893`),
so its reservation interaction is restricted to eligible length5/6 actors.
The module additionally requires round>=350, eight quiet rounds, a current
visible queen or sufficiently large recipient, its safe two-step pearl route,
and the scout's safe nonpearl exit (`pearl_reserve.hpp:5`, `:37`, `:49`).

## Counts

```json
{
  "counts": {
    "robust_h14_82001": {
      "all_actions": 1387926,
      "own_actions": 624428,
      "own_kind_0": 608369,
      "own_kind_1": 14410,
      "all_pearl_collections": 88995,
      "own_pearl_collections": 42816,
      "own_late_pearl_collections": 15585,
      "own_late_short_pearl_collections": 12213,
      "own_kind_2": 1649
    },
    "robust_pearl_reserve_82001": {
      "all_actions": 1387926,
      "own_actions": 624428,
      "own_kind_0": 608369,
      "own_kind_1": 14410,
      "all_pearl_collections": 88995,
      "own_pearl_collections": 42816,
      "own_late_pearl_collections": 15585,
      "own_late_short_pearl_collections": 12213,
      "own_kind_2": 1649
    }
  },
  "gate_upper_bound_counts": {
    "robust_h14_82001": {
      "late_own_actions": 189544,
      "short_nonqueen_population_gate": 169623,
      "age_at_least_eight_rounds": 137905,
      "no_current_visible_enemy": 80838,
      "eight_round_visible_quiet_upper_bound": 55523,
      "quiet_upper_bound_with_visible_pearl": 29302,
      "quiet_upper_bound_with_actual_size_receiver": 4315,
      "quiet_upper_bound_with_pearl_and_receiver": 2292,
      "co_presence_not_feed_preempted": 2113,
      "co_presence_feed_preempted": 179
    },
    "robust_pearl_reserve_82001": {
      "late_own_actions": 189544,
      "short_nonqueen_population_gate": 169623,
      "age_at_least_eight_rounds": 137905,
      "no_current_visible_enemy": 80838,
      "eight_round_visible_quiet_upper_bound": 55523,
      "quiet_upper_bound_with_visible_pearl": 29302,
      "quiet_upper_bound_with_actual_size_receiver": 4315,
      "quiet_upper_bound_with_pearl_and_receiver": 2292,
      "co_presence_not_feed_preempted": 2113,
      "co_presence_feed_preempted": 179
    }
  }
}
```

## Limits

Actions compare complete official move/split/suicide payloads plus round, actor, side, initial length and head. Collections require official pearl removal and same-turn movement to that cell. World compares pearl changes, moves, splits and deaths, excluding logs/sonar/header paths. Gate counts are necessary-condition upper bounds, not actual instrumentation: wrapped 7x7 current vision and visible-enemy history are reconstructed, sonar-enemy sightings are ignored, receiver full actual length replaces the bot's potentially lower visible/reported length, and topology/escape/path/payment checks are not imposed. Feed preemption is directly logged. Exact internal reservation activation cannot be recovered because R12 emits no reservation log.

## First Differences

No action or pearl-collection divergence in any of the 68 paired games.
