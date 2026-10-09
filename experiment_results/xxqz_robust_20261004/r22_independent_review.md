# R22 Independent Source Review

Reviewed 2026-10-06 after `R22 READY` freeze. Candidate:
`tyhh_test/tyhh_qos_robust_queen_rear_v127`; control: frozen R10
`tyhh_qos_robust_queen_hunt_v127`. No bot source was edited, no competitive
game was run, no confirmation seed was inspected, and no data was removed.

## Verdict

No blocking implementation issue was found for the preregistered 68-game
development screen. R22 adds exactly the frozen R21 five-line allocation to
R10's ordinary-growth branch. The result is an R10 attack-package by split
allocation interaction cell, not a pure queen-valuation experiment.

## Source Findings And Risks

- `main.cpp:1279` is the only added production block. It admits the new
  childLEN-2 allocation only for nonqueens, ROUND<350, LEN>=5, and successful
  new-size can_split/splitSafe. The original growth admission at line 1277
  and split2 fallback at line 1284 are unchanged. All original wantExpand,
  TARGET, large-map, consolidation, queen bootstrap, and split2 safety gates
  remain. Helper, feed, consolidate, and TOML match frozen R10 byte-for-byte.
- R10 `sprintRamAction` remains unchanged: the LEN<3 entry rejection at
  `main.cpp:1125`, complete own-body gate, fixed free-step/payment semantics,
  visible ordinary topology, node/depth limits, and queen-target priority
  are intact. Its call remains after growth at line 1293. Early attacks,
  tactical/emergency/late splits, queen/portal guards, and the final pipeline
  at line 1564 are unchanged. R22 adds no R16 LEN2 relaxation, order change,
  R20 witness review, or R17 maturation policy.
- Nonblocking interaction risk: new LEN2 parents cannot use exact sprint.
  Rapid cascade/cap access can instead expose a longer child to that search
  earlier. Changed occupancy, free-step budgets, corpse collection, and rear
  frontier directions may offset either mechanism. These are the intended
  interaction to measure; isolated wins must not be assumed additive.
- The inherited `bridged` at `main.cpp:725` treats unavailable segments as
  not proven bridged, so `splitSafe` at line 731 can accept incomplete neck
  or tail evidence. It proves neither complete-visible bodies nor escaping
  parents/children. An unseen portal neck can leave a new parent2 straddling
  a portal, a known engine death mechanism. R22 deliberately retains the
  R10/R21 gate; no safety certificate is attached to its accepted split.

## Independent Verification

All checks used the READY hashes below and exited 0:

- Fresh native R22 and R10 production builds with g++-10, C++20, -O2, without
  warnings. Their binary hashes match READY.
- The 17 rear-cascade source assertions passed against R22 and separately
  against the actual frozen R10 main using REAR_CASCADE=0. They cover length,
  round boundary, queen/bootstrap, champion, cap, large-map, consolidation,
  portal priority, new-size fallback, original split rejection, and the
  unchanged emergency reverse split.
- Inherited R10 sprint4 tests and queen_hunt tests passed against R22,
  including exact body/payment/eligibility checks and 200 frozen R4 ordinary
  target controls. These test files are byte-identical to R10. Include-based
  renamed-main test compiles alone emit the inherited no-return warning;
  no test calls that renamed main.
- The native cascade script was rerun independently under
  `.venv-battlecode/bin/python`, asserted unswbc 1.2.7, and recorded the
  expected official engine hash. All four LEN7/14 candidate/control cases
  passed. R10 creates only child2 and retains parent5/12. R22 LEN7 produces
  children5,3; LEN14 produces12,10,8,6,4,2. Every new parent retains2;
  prefix/reversed-suffix split arrays match, native newborns act in round0,
  the final child moves after the last split, and no round0 deaths/notices
  occur. The same-round mechanic is independently reproduced.

Permanent raw rerun record:
`r22_review_evidence/r22_independent_native.json`, SHA256
`13bbe9ffb88a693cfed7a9631144b9f242bd1752768881d5a4cec69594708abb`.
It is byte-identical to the author's independently produced fixture JSON.
It retains replies, map/binary hashes, split arrays, moves, deaths, and
notices. The script parses the packed replay but does not export it.

## Remaining Coverage

The official fixture uses modified autarky geometry with ordinary edges and
no pearls. It validates allocation/scheduling and native pipeline behavior
in those four constructed cases, not competitive hunts, hidden portal/crowd
consequences, or complete persistent trajectories. Round1 ENDTURN deaths
are deliberate fixture cleanup. Direct source fixtures provide a synthetic
full trail and do not prove rejection of unavailable-body evidence.

No competitive gain, B-seat robustness, queen survival, judge-points or
memory compliance follows from these checks. The screen should compare
R22-R10 with R21-H14 on matched 82001 games, retain paired gains/losses and
both sides, and distinguish cascade counts from eligible exact attacks.
Different frozen candidates must not be pooled; confirmation remains closed.

## Frozen Hashes

Source was rechecked after all reruns and remains unchanged:

- R10 main: `de27f3b2abb1440d75395c9676eb4d80967947bd7adb7e66f9570005f24ca8ba`
- R22 main: `43ed3e523628fb34f74279943f80408f149353d5a3440a6fbadf73cfe4ff8138`
- Cascade source tests: `21e204671dec880b48a8c5d66e2353a12c731710c68ada6658fec6d68ba311b0`
- Native script: `c7236fd697193401d2e79bb39b39bf1ab93d1ce2d218bba0a6eb9bb1a281259f`
- Sprint tests: `5f2e385767f8869fe74bc3de53556fc8c6242c38444e3eae16e5f76429d2a958`
- Hunt tests: `431522e135dde8ef2900b743272b60cf5e04e121ac75dd17c9790e38ed8c68e2`
- Native R22: `2ae55dbec7cb7860fb22f682847488380c8e5d7839e8d09d64de903ea147926e`
- Native R10: `9c417c402e5742764a5a425571cf51283690d2c41b165b68ed13c1974f2671b7`
- Official engine: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`
