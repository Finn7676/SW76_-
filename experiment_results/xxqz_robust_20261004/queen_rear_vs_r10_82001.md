# Queen-rule Experiment Comparison

One deterministic pearl seed per ranked map, A/B paired; descriptive development comparison only

| Version | Opponent | W/L/D | Queen Alive | Mean Queen | Opponent Mean Queen | Legacy W/L/D |
|---|---|---:|---:|---:|---:|---:|
| robust_queen_hunt_82001_retry1 | V3_portal | 16/18/0 | 5/34 | 1.62 | 0.53 | 7/13/0 |
| robust_queen_hunt_82001_retry1 | xxqztest | 19/15/0 | 5/34 | 1.76 | 0.32 | 12/8/0 |
| robust_queen_rear_82001 | V3_portal | 19/15/0 | 6/34 | 1.71 | 0.41 | 11/9/0 |
| robust_queen_rear_82001 | xxqztest | 17/17/0 | 7/34 | 2.24 | 0.32 | 10/10/0 |

## Paired Changes

robust_queen_rear_82001 versus robust_queen_hunt_82001_retry1: 5 changes to a win; 4 former wins became non-wins.

## Representative Losses

Selection is fixed: default/portals loss per opponent, side A before B; otherwise largest final queen deficit.

### robust_queen_hunt_82001_retry1

- V3_portal, default, side A: queen 0:0, longest 0:4, total 0:121; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/robust_queen_hunt_82001_retry1/0005_V3_portal_default_seed82003_A.replay).
- V3_portal, portals, side B: queen 0:0, longest 7:10, total 65:65; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/robust_queen_hunt_82001_retry1/0010_V3_portal_portals_seed82005_B.replay).
- xxqztest, portals, side A: queen 0:0, longest 10:15, total 53:37; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/robust_queen_hunt_82001_retry1/0043_xxqztest_portals_seed82005_A.replay).

### robust_queen_rear_82001

- V3_portal, default, side A: queen 0:0, longest 0:4, total 0:121; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/robust_queen_rear_82001/0005_V3_portal_default_seed82003_A.replay).
- xxqztest, default, side A: queen 0:0, longest 6:17, total 31:72; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/robust_queen_rear_82001/0039_xxqztest_default_seed82003_A.replay).
- xxqztest, portals, side A: queen 0:0, longest 9:21, total 47:47; [replay](/home/tyhh/SW76_-/experiment_results/queen_rules_20261003/robust_queen_rear_82001/0043_xxqztest_portals_seed82005_A.replay).

