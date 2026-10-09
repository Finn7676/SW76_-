# H14 Development Qualification

Audited ranked17 base78001: V3 18/34wins, xx19/34wins; old10subset23/40.
All68 source/engine/map/log/replay audits passed. This is the first current
continuation candidate passing the strict development joint>50% gate.

## Frozen Independent Confirmation

All408 candidate/reference games passed source/binary/engine/map/log/replay
and score audits. Three preregistered unused seed bases79001,80001,81001,
ranked17, both opponents and both sides; unswbc1.2.7; draws are non-wins.

| Version | V3 Wins/102 | xx Wins/102 | Old10 Wins/120 |
|---|---:|---:|---:|
| H14 |56 (54.9%)|54 (52.9%)|64|
| Unchanged relay_counter |37 (36.3%)|45 (44.1%)|51|

Registered pooled-side joint>50% gate passed. Batch rates mean+/-sample std:
V3 54.9%+/-7.4%, xx52.9%+/-5.1%; no hypothesis test. The paired comparisons
have45 gained wins and17 lost former wins. This is not a significant or
every-map/every-side victory claim: sideB is24/51 versus both; portals versus
xx is0/6, default versusV3 is2/6 and versusxx1/6. Queen alive38/204, reference
34/204: do not claim reliable queen survival. Four portal sandbox games had
no runtime failures, but all were losses and do not establish full-map limits.

Portal protection still requires observed safe landings for queen/longdragon.
Production and opponents were not changed. Main hash remains
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
Full per-seed/map/side evidence: ../../experiment_results/queen_rules_20261003/h14_confirmation_final.md.
