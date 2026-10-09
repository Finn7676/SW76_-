# R26 Independent Implementation Review

Reviewed 2026-10-08 against candidate
`tyhh_test/tyhh_qos_robust_headtrade_v127` and actual H14 control
`tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127`.
No strategy source was edited, competitive game started, or reserved-seed
result read during this review. An independent source-bound fixture was
created and executed under `/tmp/r26-independent.s2OpBD`.

## Findings

No blocking implementation issue was found in the reviewed source for the
registered local mechanism. The following scope and evidence limits remain
material:

- `headtrade.hpp:86` reviews the final ordinary living nonqueen movement,
  including eligible living contested-pearl sprints and movement produced
  by inherited reviews. It is not confined to ordinary scorer proposals.
  The registration now states this scope; unchanged earlier branch source
  does not imply unchanged final emitted movement or collection.
- `headtrade.hpp:69` triggers for any exact enemy strictly shorter than
  projected own length, excluding the enemy queen. It does not retain the
  initially discussed gap-three threshold. Pearl growth and multi-step
  payment use the existing `projectAction` result, so the comparison concerns
  final length rather than preaction LENGTH.
- `headtrade.hpp:62` and `headtrade.hpp:121` implement a geometric
  no-new-exposure check for every currently visible enemy head, including
  partial bodies and equal/longer enemies. The check is conservative and
  does not assert enemy intent or global next-actor ordering. Zero exact
  shorter witnesses does not establish safety against hidden dragons,
  portals, longer routes, splits, or intervening actor changes.
- `headtrade.hpp:108` runs the inherited scorer again only after a confirmed
  trigger. It restores the original proposal before evaluating alternatives;
  scorer values and direction ties remain unchanged. This adds scorer and
  body-reconstruction work on triggered turns. No sandbox budget or maximum
  competitive runtime claim was established by these native fixtures.
- This reviewer independently verified source controls and inherited
  regressions, not the historical diagnosis counts or the official native
  actual-input/counterfactual artifacts being produced separately. Those
  artifacts, a complete source freeze, and their required checks must be
  recorded before launch. Nothing here establishes a win-rate benefit.

## Source Audit

The candidate main diff against actual H14 consists only of the module
include at `main.cpp:1473` and the final call at `main.cpp:1538`. The call
follows `freeHarvest`, `reviewProtection`, `reviewPortalSafety` and
`reviewQueenSurvival` and precedes final projection and emission. No later
movement reviewer can overwrite an admitted replacement.

Helper, TOML, queen-feed and consolidation files are byte-identical to
actual H14. The complete-body proof follows the corrected R20 predecessor
chain approach: one head, unique predecessor links, every shown member in
the reconstructed chain, no portal body link, and exact own LENGTH or
known ordinary/current-visible neighbors around the enemy tail. Own
`ownSeg` ordering must also match the proven chain. This explicitly avoids
treating `projectAction().known` as proof of a complete own body.

The review preserves queen, split, donation, portal/unknown, empty and
already-fatal proposals. It requires a complete exact nonqueen shorter-enemy
ordinary first-step witness before doing extra scorer work. Each replacement
has a finite inherited raw score, one current-visible ordinary step, a
living complete projection, ordinary visible continuation and no direct,
hard or soft friendly-protection risk. It rejects any new geometric ordinary
first-step head exposure and any remaining complete shorter-enemy witness.
The highest raw score wins with original N/E/S/W tie order; no admitted
alternative preserves the proposal. No generic fallback is committed.

## Independent Verification

All commands below completed with exit status 0:

- Fresh actual H14 and R26 production builds using `g++-10 -std=c++20 -O2`
  emitted no warnings. The H14 binary reproduces its recorded production
  hash. Extra `-Wall -Wextra -pedantic` builds emit the same inherited unused
  `enemyEscapes` warning for both; no new candidate warning was found.
- The independent `/tmp/r26-independent.s2OpBD/review.cpp` fixture includes
  the actual candidate source and passes both team identities, exact
  shorter/equal/longer enemies, own queen and enemy queen exemptions, and a
  pearl that changes equal preaction lengths into a shorter-enemy trigger.
- Rejection controls pass for partial own/enemy bodies, malformed enemy
  predecessors, own trail-order mismatch, portal enemy-tail uncertainty,
  unknown/portal proposals, split/donation, invalid direction, an already
  fatal head ram, and no admitted alternative.
- Complete and partial new-attacker controls preserve the original when
  the sole otherwise-eligible endpoint would add another visible head
  exposure. Hard and soft friend-protection controls and a no-continuation
  control also preserve the original. A living two-step ordinary proposal
  is reduced to an admitted one-step replacement under the registered scope.
- The three unchanged source-bound inherited suites pass against R26:
  projection/payment/queen identity/bootstrap/portal checks, seven queen
  guard assertions and eight consolidation checks. Included renamed-main
  harnesses emit the inherited no-return warning and never call that renamed
  entry point.
- The finalized focused source-bound suite
  `verification/headtrade_tests.cpp.txt` passes all 72 controls against both
  R26 and the H14 control build. It covers exact shorter-length triggers,
  pearl-growth length changes, multi-step living proposals, every excluded
  proposal class, malformed/partial/portal/unknown own and enemy chains,
  new-attacker exposure, friend gates, continuation, bent chains, raw-score
  preservation and all N/E/S/W pairwise tie orders. The own-duplicate case
  duplicates an actual own segment (`parts[occPart[cid(12,10)]]`).
- The archived `verification/independent_review_tests.cpp.txt` reproduces
  the independent fixture from the candidate tree and passes after compiling
  with `g++-10 -std=c++20 -O2 -x c++`.
- The fresh native actual-input binding script passes both registered cases:
  default 82003A round 20/id 2 and australia 83001A round 352/id 341.
  Each emits `SHORTER_HEAD_TRADE`, selects `MOVE N`, and exactly matches its
  separately scripted official application arm. This confirms the local
  candidate decision and prefix/input binding only; it is not a game result.

Independent fixture command:

```bash
g++-10 -std=c++20 -O2 -x c++ \
  tyhh_test/tyhh_qos_robust_headtrade_v127/verification/independent_review_tests.cpp.txt \
  -o /tmp/r26-independent.s2OpBD/archived_review
/tmp/r26-independent.s2OpBD/archived_review

.venv-battlecode/bin/python \
  experiment_results/xxqz_robust_20261004/r26_candidate_native_fixture.py \
  --candidate /tmp/r26-independent.s2OpBD/r26_production
```

Inherited fixture commands use `g++-10 -std=c++20 -O2 -x c++` against
the candidate's `verification/{candidate,queen_guard,consolidate}_tests.cpp.txt`
and execute each resulting binary. No competitive, native engine, or
held-out game was launched by these commands.

## Verdict

The reviewed code implements the registered final movement-risk factor
without a source-level blocker. This is a local implementation verdict,
conditional on the separate required official actual-input checks and freeze
before the development screen. Native source fixtures establish admission
and exclusion behavior, not future survival, reduced total collector loss,
competitive gain or confirmation performance.

## Bound Hashes

- Registration: `f3d351d8b675d7e6ba9f3473759dca94b11350580ceef705c09a214f9e0f4c15`
- R26 main: `660363800c8cce16b1717d0f9dbbdf9feba6e02083380ccb1b3e1a5e92cc06d4`
- R26 module: `69bf74738cbf99f655eb9b715eec10f41b8946d184808b41fac28417c04ce4da`
- R26 production: `72a799b31a4fd26a67eebea5fa90fa1de8f692ae996d10d68a89e50e1223e1fa`
- H14 production: `98a8c546c653a6a9ae1aabdae284a22ef460ac3bc525bf842ed31223f3232c9f`
- Independent fixture: `ae8a5ff924b6d0066c10db1428f1c9f7e4f33858b2d0f6e15025b1f9f0c487c1`
- Archived independent fixture: `98a0dd0cbc7309e457faa4a142aa2ec88b12e0b7f6038b3c4a8233a89beefc15`
- Archived fixture binary: `388c2ec078e587510a5d168abf5886b7f0a3b940c5ac1c31d32d976bf94d3bf6`
- Focused 72-control tests: `3f8d8fb15afac7f92cb3e9917290a37683e3ad60993a04cc99cad7bebe4beb3c`
- Focused candidate binary: `c261563593382caee2e438154b300c8a7005525008a5f96d6eeb82d360e5a7dc`
- Focused H14 binary: `bb034c0d9fcc561edc3093c0bbec176cb1c74067435c67a00f50a562240292a8`
- Native binding script: `d6287bd41461716a3ad2f21b4a11e232cabd5d57283fd8288257da0d8e59963b`
- Native actual-state fixture: `d7673b4945db3b8c5dd452821eb205adc4546a04a12953a2128f83fd1fd052b0`
- Native candidate result: `416882b5e92eb36439c290d6d34d8e382c65e8d0e5869e7c513a641475ccf016`
- Inherited projection fixture: `a575e2e4b3eff22ad8b2e90c1a35355fd5d6d4c0af01956551af98adf42cd3e2`
- Inherited queen fixture: `9dea7beefc3b48de61e12133c33b42bdcafb80f9ecfe47fd6645e62b9d913def`
- Inherited consolidation fixture: `5de377877b7bbada6b7f8f2197d8db9544c2bec26b213251ca3bb7bbacdfa3e4`
