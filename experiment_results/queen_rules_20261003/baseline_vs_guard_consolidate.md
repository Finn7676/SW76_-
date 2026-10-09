# Queen-rule Experiment Comparison

One deterministic pearl seed per ranked map, A/B paired; descriptive development comparison only

| Version | Opponent | W/L/D | Queen Alive | Mean Queen | Opponent Mean Queen | Legacy W/L/D |
|---|---|---:|---:|---:|---:|---:|
| counter_baseline | V3_portal | 13/21/0 | 4/34 | 0.32 | 0.47 | 9/11/0 |
| counter_baseline | xxqztest | 13/21/0 | 2/34 | 0.18 | 0.24 | 10/10/0 |
| queen_guard_consolidate_v127 | V3_portal | 18/16/0 | 4/34 | 1.35 | 0.29 | 10/10/0 |
| queen_guard_consolidate_v127 | xxqztest | 19/15/0 | 6/34 | 1.65 | 0.41 | 13/7/0 |

## Paired Changes

queen_guard_consolidate_v127 versus counter_baseline: 15 changes to a win; 4 former wins became non-wins.

## Representative Losses

Selection is fixed: default/portals loss per opponent, side A before B; otherwise largest final queen deficit.

### counter_baseline

- V3_portal, default, side A: queen 0:0, longest 0:8, total 0:128; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/counter_baseline/0005_V3_portal_default_seed78003_A.replay).
- V3_portal, portals, side A: queen 0:0, longest 13:31, total 79:86; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/counter_baseline/0009_V3_portal_portals_seed78005_A.replay).
- xxqztest, default, side B: queen 0:0, longest 3:19, total 15:97; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/counter_baseline/0040_xxqztest_default_seed78003_B.replay).
- xxqztest, portals, side A: queen 0:0, longest 13:19, total 69:46; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/counter_baseline/0043_xxqztest_portals_seed78005_A.replay).

### queen_guard_consolidate_v127

- V3_portal, default, side A: queen 0:0, longest 0:6, total 0:130; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_guard_consolidate_v127/0005_V3_portal_default_seed78003_A.replay).
- V3_portal, portals, side A: queen 0:0, longest 9:12, total 56:81; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_guard_consolidate_v127/0009_V3_portal_portals_seed78005_A.replay).
- xxqztest, portals, side A: queen 0:0, longest 9:14, total 56:41; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_guard_consolidate_v127/0043_xxqztest_portals_seed78005_A.replay).

