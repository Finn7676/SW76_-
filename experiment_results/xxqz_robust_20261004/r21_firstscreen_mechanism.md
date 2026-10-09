# R21 First-Screen Mechanism

All68 R21 and matched H14 replays have complete passed official outcome audits. This read-only analysis rechecks frozen source/binary/map/engine manifests, each replay SHA256, actual split arrays, and reconstructed final standings. No game was launched and no frozen source was modified.

| Mechanism Evidence | Games |
| --- | ---: |
| games | 68 |
| identical_action_games | 30 |
| identical_physical_sonar_games | 30 |
| first_action_divergence_games | 38 |
| strict_prefix_first_ordinary_allocation_effect_games | 38 |
| other_first_action_divergence_games | 0 |
| round_zero_verified_first_allocation_games | 16 |
| round_zero_verified_first_with_newborn_cascade_games | 16 |

The strict-prefix first actual rear allocation is an observed ordinary-factor effect. It is not a complete activation count: the source emits no ordinary-branch telemetry, and later reverse allocations can also come from emergency logic.

| Raw Early Nonqueen LEN>=5 Split Pool | H14 | R21 |
| --- | ---: | ---: |
| early_LEN5plus_nonqueen_actual_splits | 2545 | 2718 |
| child2 | 1370 | 129 |
| child_LENminus2_parent2 | 1175 | 2589 |

## Round20

| Opponent / Seat | Paired Games Reaching20 | Mean Population Delta | Mean Total Delta | Mean Birth Delta |
| --- | ---: | ---: | ---: | ---: |
| V3_portal / A | 17 | -0.29 | -0.47 | +0.00 |
| V3_portal / B | 17 | -0.06 | -0.18 | -0.06 |
| xxqztest / A | 17 | -0.06 | -0.12 | +0.12 |
| xxqztest / B | 17 | -0.41 | -0.88 | -0.18 |

## Actual Parent-LEN2 Geometry

H14: 1175 observed early pool parent-LEN2 allocations; geometry {'ordinary_edge': 1175}; actual same/next-round death reasons {'self': 1004, 'body': 25, 'head': 6}. Portal-link witnesses 0. Geometry is not a death-cause inference; full event details and next-turn/action/update/death timing are in JSON.

R21: 2589 observed early pool parent-LEN2 allocations; geometry {'ordinary_edge': 2589}; actual same/next-round death reasons {'self': 1150, 'body': 30, 'head': 12}. Portal-link witnesses 0. Geometry is not a death-cause inference; full event details and next-turn/action/update/death timing are in JSON.

## Paired Examples

Promotions: 9 observed paired win-status changes.

- V3_portal / australia / seed82001 / A: loss -> win; round154 id107 SPLIT 2 -> SPLIT 4 strict-prefix=True; round20 own deltas {'population': 0, 'total': 0, 'longest': 0, 'queen': 0}.
- V3_portal / autarky / seed82002 / A: loss -> win; round0 id2 SPLIT 2 -> SPLIT 12 strict-prefix=True; round20 own deltas {'population': -2, 'total': -3, 'longest': 0, 'queen': 0}.
- V3_portal / autarky / seed82002 / B: loss -> win; round0 id3 SPLIT 2 -> SPLIT 12 strict-prefix=True; round20 own deltas {'population': -1, 'total': -3, 'longest': 0, 'queen': 0}.
- V3_portal / portals / seed82005 / B: loss -> win; round44 id3 SPLIT 2 -> SPLIT 3 strict-prefix=True; round20 own deltas {'population': 0, 'total': 0, 'longest': 0, 'queen': 0}.

Regressions: 3 observed paired win-status changes.

- V3_portal / stripes / seed82014 / B: win -> loss; round65 id9 SPLIT 2 -> SPLIT 3 strict-prefix=True; round20 own deltas {'population': 0, 'total': 0, 'longest': 0, 'queen': 0}.
- xxqztest / schooltime / seed82008 / B: win -> loss; round140 id65 SPLIT 2 -> SPLIT 13 strict-prefix=True; round20 own deltas {'population': 0, 'total': 0, 'longest': 0, 'queen': 0}.
- xxqztest / unsw / seed82016 / B: win -> loss; round0 id19 SPLIT 2 -> SPLIT 8 strict-prefix=True; round20 own deltas {'population': -3, 'total': -5, 'longest': 1, 'queen': 0}.

## Limits

- One development screen only; no held-out seed or generalization claim.
- True exposure is the strict-prefix first actual allocation effect in each matched game; this is a lower bound, not all ordinary branch activations.
- Later raw LEN-2 allocations include other branches; no ordinary activation count is inferred from them.
- No committed trace difference does not establish no rejected/proposed branch activation.
- Post-divergence numerical child IDs are not treated as paired causal identities.
- Full bodies are offline diagnosis; radius3 current visibility is not reconstructed private trail, sonar memory, or source input.
- Portal-link geometry and actual death timing/reason are separate evidence; invalid_action is never relabeled as portal death.
- Round20/50/100 checkpoints are before that round; early-ended games are not padded.
- Round-zero descendant splits establish actual allocation timing, not a causal win explanation.

This report describes observed mechanisms and paired outcomes only. The independent experiment decision owns expansion or production promotion. Complete event witnesses, hashes, and all matched tuples are in `r21_firstscreen_mechanism.json`.
