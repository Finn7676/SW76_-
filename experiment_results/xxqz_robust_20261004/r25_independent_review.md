# R25 Independent Source Review

Reviewed on 2026-10-08 against `R25_EXPERIMENT_DRAFT.md`, without changing
production policy, launching competitive games, or reading reserve-seed results.

## Verdict

No blocking correctness, isolation, or preregistration-conformance issue was
found in the reviewed R25 source. This is a source/local-fixture review, not
evidence of competitive benefit or an authorization to claim improved wins.

Reviewed production SHA256 values:

- H14 `main.cpp`: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`
- R25 `main.cpp`: `2f682b33094ce2ed644cefd730cbcaad2ed400999f99c813c3579185db10e0a9`
- R25 `short_terminal.hpp`: `74f24a1a78d63371543cdd985cd6a53b0bf9860badbe05234bf35205d86b6c52`

## Source Findings

1. **Isolated factor.** The complete H14/R25 `main.cpp` diff consists of the
   module include at candidate line 1473 and the final review call at line
   1538. `helper.hpp`, `queen_feed.hpp`, `consolidate.hpp`, and `bot.toml` are
   byte-identical to H14. Scoring, movement projection, payment, donation,
   split branches, attacks, and inherited reviews have no source changes.

2. **Exact quiet, ordinary LEN2 entry.** `short_terminal.hpp:3` requires
   length two and the complete current visible segment count. Lines 5-20
   reject any current enemy part, require exactly one own head and one own
   tail, require distinct current visible occupied own cells, check the
   ordinary tail-to-head edge/facing, and reconcile the observed cells with
   `ownSeg`. Lines 53-60 exclude queen, split, donation, empty/multistep
   proposals, nonordinary moves, hidden/occupied/nonpearl destinations, and
   anything other than a surviving known LEN3 terminal projection. Source
   observation rebuilds `parts` from current tiles at `main.cpp:458`; this
   gate does not admit remembered enemy-free geometry as current evidence.

3. **Terminal proof is limited to walls and projected own body.** Module
   lines 23-32 require a known surviving projection with length/body-size
   agreement. Every nonkelp exit must be `E_OPEN`, currently visible, and
   in the projected own body. Unknown/portal exits or other-body occupancy
   cannot establish the registered terminal condition. The unique verified
   LEN2 body and unoccupied original destination establish the original
   projection's unique three cells without introducing a body-model change.

4. **Unchanged scorer; direct reviewed selection.** Module lines 64-66 save
   and restore the full proposed action around unchanged
   `chooseScoredAction`. Its fallback action is never committed by R25.
   Lines 69-80 inspect directions in inherited 0-3 order, require a finite
   unchanged `moveScores[d]`, and select strictly by greater score among
   admitted alternatives. Equal-score order matches the scorer's
   `main.cpp:1446` comparison. If none survives review, the proposal remains
   restored. Scorer scratch arrays/BFS state have no later action-review
   consumer after this final call.

5. **Friendly protection is retained.** Module lines 70-77 require an
   ordinary visible unoccupied replacement destination, exact inherited
   projected survival, no `directFriendCollision`, and zero existing hard
   and soft protection risk. `actionRisk` at `main.cpp:845` evaluates the
   projected body against inherited friend exits; R25 does not adjust that
   model or bypass soft protection with a score improvement.

6. **Continuation respects growth and collision timing.** Module lines
   35-49 require a current-view ordinary next cell outside the replacement's
   projected body and outside current other bodies. The continuation inserts
   its new head, grows on a current visible pearl, or removes the old tail
   otherwise. An old own cell vacated by the first move is therefore eligible;
   a segment still in the projected body is rejected before tail removal,
   consistent with inherited `projectAction` at `main.cpp:797`. The next
   projection must fail the same wall/own-body terminal proof. Both moves are
   initial free steps on separate turns at LEN2/LEN3; this does not invent a
   same-turn paid second move or double-consume the first destination pearl.

7. **Final review cannot be overwritten by inherited reviews.** The order
   is protection, portal safety, queen survival, R25 review, projection, and
   emission at `main.cpp:1535`. After R25, `projectAction` reads the action and
   `emitAction` at `main.cpp:1023` formats that same action. Remaining code
   only emits diagnostics/SONAR; no later scorer, fallback, or action review
   can reintroduce the terminal proposal.

## Verification Reviewed

The reviewer independently compiled the source-bound focused suite with
`g++-10 -O2 -std=c++20` and ran all 38 controls successfully. The only compile
warning is the inherited test-only renamed-main missing-return warning.
Controls cover positive/unscored selection, gate exclusions, own-body
completeness/identity/ordinary linkage, enemy head/body, replacement visibility
and occupancy, hard/soft friendly protection, terminal continuations, and
source occupancy fallback. The three inherited test files are byte-identical
to H14; their previously recorded results and production O2 compilation were
read in `verification/VALIDATION.md`, rather than rerun by this reviewer.

Existing exclusive native and official-rule outputs were inspected without
regenerating or overwriting them. Native artifact source hashes match the
reviewed source and its saved input SHA matches the rule artifact and current
actual-state file. It records fresh H14 `MOVE S` and R25 `MOVE E`, with empty
stderr. Rule-script and both replay hashes match the recorded artifact. Both
arms record identical target input and a 27385-event physical/SONAR/action
prefix, with other actors' recorded actions/SONAR held fixed. The original
scripted `S,N` arm records actor 75 self death at round 189 (native callback
code `S`, decoded replay reason `self`); the alternate `E,S` arm records two
successful LEN2 updates and no actor 75 death in the evidence window.

## Limits And Remaining Evidence

The native selection test uses a fresh process, not historical persistent
memory regeneration. The alternate round-189 `S` is scripted; the fixture
does not establish what the live candidate will choose at that next turn.
Recorded other actors do not react to the changed body. Current-view
continuation admission excludes known collisions and the registered terminal
proof, but makes no guarantee about hidden occupancy, future actor motion,
or long-term safety. There are no R25 competitive results in this review and
no win-rate, cross-map stability, runtime-bound, or reserve-seed conclusion.
Those limits do not constitute a blocker for this narrowly registered source
factor and local validation.
