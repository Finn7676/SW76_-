# V3_portal feature ablation — 2026-09-25

Design: disable one FEAT_V3_* switch at a time; 12 maps, one game on each side (24 unique conditions).
The earlier baseline had 10 identical repeats for every condition, so this study uses no redundant repeats.

Baseline: 14/24 wins (58.3%).

| Disabled feature | Wins | Win rate | Delta wins | Delta pp | Changed conditions |
|---|---:|---:|---:|---:|---:|
| FEAT_V3_RELAY | 10/24 | 41.7% | -4 | -16.7 | 4 |
| FEAT_V3_SCOUT | 11/24 | 45.8% | -3 | -12.5 | 5 |
| FEAT_V3_PORTAL_CLAIM | 12/24 | 50.0% | -2 | -8.3 | 4 |
| FEAT_V3_EXPLORED_BONUS | 14/24 | 58.3% | +0 | +0.0 | 0 |
| FEAT_V3_PEARL_ECONOMY | 14/24 | 58.3% | +0 | +0.0 | 8 |
| FEAT_V3_SHORT_AVOID | 15/24 | 62.5% | +1 | +4.2 | 1 |

## Condition flips

### FEAT_V3_RELAY
- default.map side A: win -> loss
- help.map side B: win -> loss
- schooltime.map side B: win -> loss
- trauma.map side B: win -> loss

### FEAT_V3_SCOUT
- default.map side A: win -> loss
- help.map side B: win -> loss
- queen_of_spades.map side A: loss -> win
- stronghold.map side B: win -> loss
- trauma.map side B: win -> loss

### FEAT_V3_PORTAL_CLAIM
- default.map side A: win -> loss
- help.map side B: win -> loss
- schooltime.map side A: win -> loss
- stronghold.map side A: loss -> win

### FEAT_V3_EXPLORED_BONUS
- None

### FEAT_V3_PEARL_ECONOMY
- big_empty.map side B: loss -> win
- default_small.map side A: win -> loss
- devil.map side A: win -> loss
- help.map side A: loss -> win
- schooltime.map side B: win -> loss
- stronghold.map side A: loss -> win
- trauma.map side B: win -> loss
- trophy.map side B: loss -> win

### FEAT_V3_SHORT_AVOID
- help.map side A: loss -> win
