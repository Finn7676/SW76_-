# Queen-rule Experiment Comparison

One deterministic pearl seed per ranked map, A/B paired; descriptive development comparison only

| Version | Opponent | W/L/D | Queen Alive | Mean Queen | Opponent Mean Queen | Legacy W/L/D |
|---|---|---:|---:|---:|---:|---:|
| queen_v127 | V3_portal | 14/20/0 | 4/34 | 0.65 | 0.29 | 8/12/0 |
| queen_v127 | xxqztest | 16/18/0 | 3/34 | 0.35 | 0.32 | 9/11/0 |
| queen_lookahead_v127 | V3_portal | 13/20/1 | 4/34 | 0.79 | 0.24 | 9/11/0 |
| queen_lookahead_v127 | xxqztest | 13/21/0 | 5/34 | 1.38 | 0.32 | 9/11/0 |
| queen_margin_v127 | V3_portal | 15/19/0 | 4/34 | 0.65 | 0.29 | 9/11/0 |
| queen_margin_v127 | xxqztest | 13/21/0 | 3/34 | 0.35 | 0.26 | 8/12/0 |

## Paired Changes

queen_lookahead_v127 versus queen_v127: 4 changes to a win; 8 former wins became non-wins.
queen_margin_v127 versus queen_v127: 1 changes to a win; 3 former wins became non-wins.

## Representative Losses

Selection is fixed: default/portals loss per opponent, side A before B; otherwise largest final queen deficit.

### queen_v127

- V3_portal, default, side A: queen 0:0, longest 0:6, total 0:130; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_v127/0005_V3_portal_default_seed78003_A.replay).
- V3_portal, portals, side A: queen 0:0, longest 9:12, total 51:71; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_v127/0009_V3_portal_portals_seed78005_A.replay).
- xxqztest, portals, side A: queen 0:0, longest 11:36, total 65:61; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_v127/0043_xxqztest_portals_seed78005_A.replay).

### queen_lookahead_v127

- V3_portal, default, side A: queen 0:0, longest 0:6, total 0:130; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_lookahead_v127/0005_V3_portal_default_seed78003_A.replay).
- V3_portal, portals, side A: queen 0:0, longest 8:13, total 72:78; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_lookahead_v127/0009_V3_portal_portals_seed78005_A.replay).
- xxqztest, default, side A: queen 0:0, longest 7:9, total 68:36; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_lookahead_v127/0039_xxqztest_default_seed78003_A.replay).
- xxqztest, portals, side A: queen 0:0, longest 10:34, total 62:57; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_lookahead_v127/0043_xxqztest_portals_seed78005_A.replay).

### queen_margin_v127

- V3_portal, default, side A: queen 0:0, longest 0:6, total 0:130; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_margin_v127/0005_V3_portal_default_seed78003_A.replay).
- V3_portal, portals, side A: queen 0:0, longest 9:12, total 51:71; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_margin_v127/0009_V3_portal_portals_seed78005_A.replay).
- xxqztest, default, side A: queen 0:0, longest 3:36, total 9:118; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_margin_v127/0039_xxqztest_default_seed78003_A.replay).
- xxqztest, portals, side A: queen 0:0, longest 11:36, total 65:61; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/queen_margin_v127/0043_xxqztest_portals_seed78005_A.replay).

