# R20 Independent Source Review

Reviewed 2026-10-06 by the read-only reviewer after `R20 READY`.
Scope: the frozen candidate `tyhh_test/tyhh_qos_robust_queen_witness_v127`
relative to frozen H14 `tyhh_qos_counter_queen_guard_consolidate_v127`.
No bot source was edited, no competitive game was run, and no data was
removed. Independent build outputs are under `/tmp/r20_independent_*`;
the official rerun evidence is permanently retained in `r20_review_evidence/`.

## Verdict

No blocking issue was found in the prescribed local first-ordinary-step
head-hit witness mechanism. This permits the planned development screen;
it is not a promotion verdict or a claim of global queen safety or wins.

The earlier reverse-facing reconstruction defect is absent from the frozen
module. Body reconstruction now follows visible predecessor links and
rejects ambiguity, cycles, disconnected parts, and incomplete evidence.
The actual curved 84006 queen body and the enemy tail's hidden side boundary
are covered by the focused assertions. The older straight-body inference
helper is not used by production R20.

## Source Findings

- `main.cpp:1019` adds the module include, and `main.cpp:1538` adds the final
  review after the unchanged H14 queen-survival guard. These are the only
  production-main differences. Helper, TOML, feed, and consolidate files
  retain the frozen H14 hashes.
- `queen_witness.hpp:14` builds each visible nonhead predecessor link from
  that part's facing. Lines 28, 34, and 37 reject multiple predecessors,
  cycles, disconnected members, and count mismatches. Own completeness has
  the independent LENGTH check at line 39 and the projection/trail identity
  check at line 98. Enemy completeness rejects each non-kelp tail neighbor
  that is nonordinary or invisible at line 44.
- `queen_witness.hpp:64` counts only a complete visible enemy's first open
  edge into the projected queen head, excluding its own body. This is a
  locally executable collision possibility under static visible geometry;
  it does not assert that this enemy is the globally next actor.
- `queen_witness.hpp:95` preserves nonqueen, split, donation, portal,
  unknown-path, incomplete-own-body, and already-fatal originals. Search is
  limited to four initial free steps at line 117 and 256 counted nodes at
  line 127. Candidates must survive the inherited exact projector, retain
  exactly the original final length at line 131, have a visible ordinary
  continuation, and introduce no hard or soft friendly-protection risk.
  Exact equality is a conservative subset of the requested no-length-loss
  condition.
- `queen_witness.hpp:139` requires each complete enemy's new witness mask
  to be a subset of its old mask; line 145 also requires a strict total
  decrease. The 2-to-1 counterexample introducing a formerly unthreatening
  enemy is rejected by focused assertions at
  `verification/queen_witness_tests.cpp.txt:164`. Search exhaustion or no
  eligible replacement preserves H14 at module line 160.

## Independent Verification

All commands completed with exit code 0 using g++-10, C++20, and -O2:

- R20 and H14 native production builds; neither emitted warnings.
- Frozen focused tests and their `-DH14_CONTROL` arm. R20 changes N to NN
  for both synthetic queen team identities; H14 keeps N. Curved/partial,
  ambiguous/disconnected/cyclic bodies, hidden side/tail, portal/unknown,
  fatal originals, nonqueen/split/donation, new-witness rejection, friendly
  risk, and final-length gates passed.
- The inherited projection, queen-guard, and consolidate fixtures compiled
  against R20 and passed. Include-based renamed-main test builds alone emit
  the inherited no-return warning; their main function is never called.
- The source-bound real-input/action-tape script was rerun independently
  using `.venv-battlecode/bin/python`, confirmed unswbc 1.2.7, and the engine
  SHA256 below. The actual round199 input digest is
  `6f2229283c3a5b5c06623d6106cff57ff3524420ba0cdcfdc51134478391d416`.
  Fresh full-pipeline H14 emits `MOVE N`; R20 emits `MOVE NN`, reports one
  complete visible enemy and a confirmed witness reduction 1 to 0. Both
  official arms recreate 19705 equal physical/SONAR prefix events and the
  same raw input. H14 queen0 and enemy14 die H in round199; R20 queen0 stays
  length8 and only enemy14 dies O. Fixture cleanup occurs after id14.

Independent official evidence: `r20_review_evidence/r20_independent_official.json`.
Its SHA256 is `f698cf1462d2889032d3c5f76f4e258a4f848e64c5cbf028dce5d5d4a0b5e8ce`.
The JSON's originally emitted replay path fields point to the temporary rerun
location; byte-identical replay copies are permanently retained as
`r20_review_evidence/r20_independent_official_H14.replay` and
`r20_review_evidence/r20_independent_official_R20.replay` without altering the
original JSON. Independent H14/R20 replay SHA256 values are respectively
`64cbbda1b45b28f4bd56e7d1eb51776f47f94e412d74d0ef2db275df85826707`
and `2d5c0a5749edb5b7f9c04350b37d535eb5e7e06af57c8ed73a35762c877e0411`.

## Residual Risks And Validation Boundaries

- `queen_witness.hpp:108` omits bodies that are not input-proven complete.
  A zero confirmed count says nothing about hidden enemies, incomplete
  bodies, unknown/portal routes, longer enemy movement, or intervening
  actors. The visible continuation at line 83 is also a static local exit,
  not a future-survival guarantee. The module comments, EXPERIMENT, and
  READY state these limits accurately.
- `verification/queen_witness_engine.py:32` deliberately uses fresh native
  processes without prior-round memory. The real-input comparison validates
  the current full pipeline, while focused tests validate supplied trail
  identity. It does not reproduce the candidate's complete persistent state
  over rounds. At script line 80 only the extracted native MOVE enters the
  official action tape; recorded SONAR is held constant. No competitive
  trajectory or changed-SONAR downstream outcome is established.
- Depth-three/four search, tie preferences, and the node-exhaustion branch
  have no dedicated focused fixture. Their guards were inspected. No
  whole-map sandbox points/memory check was performed by this reviewer.
  Ordinary native build/fixture timing cannot certify judge resource limits.

For the planned 68-game screen, retain all activation logs and audit the
confirmed-before/after counts, nodes, map/side, and subsequent queen deaths.
Keep native/source hashes bound to the frozen version. A positive competitive
effect requires paired development evidence; general safety must remain
unclaimed. Judge-budget validation is required before any production use.

## Frozen Hashes

READY hashes were rechecked after its receipt and exactly match the version
used for the independent builds, focused tests, and official fixture:

- H14 main: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`
- R20 main: `c7682278e8f70609b8edf44423273ec65df4c4729ece4ba1d971eae3308bdcfb`
- R20 module: `c760dc62345dda734182787dde599657010364a677c651e72dc6f7ce27ae58ed`
- Focused tests: `f0c8657aa571fa0add816e01beff55a9071a4be39d8d90a4afc7913b499e7f2b`
- Source-bound engine script: `17f924bb8241c545451c6e46e3a97fc853102e00f3d1df4ae4923d95d8670dde`
- Official 1.2.7 engine: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`
