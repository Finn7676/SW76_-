# 2026-10-01 portal feedback experiment

Fixed seed `76001`; ten official maps; A/B sides; 40 matches per version
(20 per opponent). All suites passed source, map, binary, log, result and
replay checks. No draws or runner failures occurred. This is a descriptive
single-seed comparison, not a stability or significance claim.

| version | V3_portal | xxqztest | parent | decision |
|---|---:|---:|---|---|
| counter baseline | 9/20 | 8/20 | — | reference |
| counter-safeportal | 10/20 | 10/20 | counter | selected H1 |
| counter-feednear | 8/20 | 9/20 | counter | reject: V3 regression |
| counter-reclaim | 9/20 | 8/20 | counter | reject: no win gain |
| relay baseline | 8/20 | 10/20 | — | reference |
| relay-safeportal | 10/20 | 11/20 | relay | parent check positive |

No H4 combination was launched: only H1 passed the pre-registered beneficial
screen, so there was no independently beneficial second module to combine.

The selected H1 policy gates only dragons of length at least eight: unknown,
invisible, occupied, or insufficient-space portal landings are removed in
early entry, scoring, protection replacement and final fallback. Short dragons
still scout. The counter version improves both opponent win counts by +1/+2;
the relay parent check improves by +2/+1. Keep the change as an experimental
candidate until more seeds are run.

Selected sources: [counter-safeportal](/home/tyhh/SW76_-/tyhh_test/tyhh_qos_counter_safeportal),
and its positive relay-parent check [relay-safeportal](/home/tyhh/SW76_-/tyhh_test/tyhh_qos_relay_safeportal).

Reproduction (from repository root):

```sh
PATH=/tmp/clang-toolchain:$PATH python3 experiment_results/protect_local_20260927/run_suite.py \
  --bot tyhh_test/tyhh_qos_counter_safeportal \
  --opponents tyhh_test/V3_portal tyhh_test/xxqztest --maps all --seeds 76001 --jobs 2 \
  --output experiment_results/relay_feedback_20261001/counter_safeportal
```

Representative retained losses include [counter-safeportal portals vs V3 A](/home/tyhh/SW76_-/experiment_results/relay_feedback_20261001/counter_safeportal/0007_V3_portal_portals_seed76001_A.replay), [counter-safeportal default vs xxqztest A](/home/tyhh/SW76_-/experiment_results/relay_feedback_20261001/counter_safeportal/0023_xxqztest_default_seed76001_A.replay), and [counter-safeportal portals vs xxqztest A](/home/tyhh/SW76_-/experiment_results/relay_feedback_20261001/counter_safeportal/0027_xxqztest_portals_seed76001_A.replay). These are raw packed replays; terminal standings and event traces are validated, but a terminal loss alone is not assigned a single causal explanation.

Module traces for reclaim are in [counter_reclaim_traces.json](counter_reclaim_traces.json); the implementation's `RECLAIM_COLLECT` count is an action-log count, not a pearl-success metric.
