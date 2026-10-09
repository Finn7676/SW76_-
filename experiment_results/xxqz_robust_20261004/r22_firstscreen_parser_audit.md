# R22 First-Screen Parser Semantic Audit

Independent semantic audit by `/root/robustness_next/r21_parser_semantics`.
Scope: frozen R22 versus actual frozen R10 retry1 source binding, event linkage,
and source-consistent interpretation of actual multi-step head collisions.
This audit is not a performance verdict and does not evaluate cross-seed
robustness. No game was launched and no candidate, opponent, engine, map,
suite, or mechanism parser was modified.

## Bound Evidence

- Subject: `r22_firstscreen_mechanism.py`, SHA256 `33fe2ac8c73e9c6b952b16f9a2784027a584121d980b3e7f9350c57119152999`.
- Reused scanner: `r21_firstscreen_mechanism.py`, SHA256 `b0c28698c4752c0991b9463a239d793f028b6080913f4abf3cca4e11399112dd`; its independent schema/topology audit is `r21_firstscreen_parser_audit.md`.
- Reproducible source/classifier check: `r22_firstscreen_parser_check.py`; exclusive result: `r22_firstscreen_parser_check.json`.
- Frozen R10 main: `de27f3b2abb1440d75395c9676eb4d80967947bd7adb7e66f9570005f24ca8ba`.
- Frozen R22 main: `43ed3e523628fb34f74279943f80408f149353d5a3440a6fbadf73cfe4ff8138`.

The source check independently removes the exact five-line rear-allocation
block from frozen R22 main and asserts byte equality with actual frozen R10
main. `helper.hpp`, `queen_feed.hpp`, `consolidate.hpp`, `bot.toml`, the queen
hunt tests and the sprint4 tests are byte-identical. The check verifies actual
prepared binary and installed engine hashes against each run manifest, and
identical map hashes/opponent binary hashes between the two suites. R22
changes no sprint-search source.

## Event Interpretation

The inherited scanner's corrected pre-round initialization, exact split
prebody, topology transitions, next-action/death attachment, and fixed
checkpoint timing remain applicable. The R22 pass independently reconstructs
bodies and uses recorded action events as the start of an acting action.
Round/turn markers finish the preceding action; successful updates and death
events are attached only within that action interval.

An own-initiated head trade requires the actor's actual `head` death and an
opponent's actual `head` death in that same interval. The successful prefix is
counted separately from the collision: actual hit step equals actor successful
updates plus one. A three-step collision ordinarily has two successful
coordinate updates, not three. Requested action length can exceed the actual
hit step; unused trailing directions must not inflate range.

For the after-two-step metric, the subject requires a nonqueen actor, actual
hit step at least three, ordinary open-edge successful prefix transitions,
and an ordinary final transition landing on an actual opponent head. This
excludes own-queen survival routes and portal-path collisions. The paid-step
test uses the fixed free-step count from initial length and actual actor
prefix length greater than two before a paid collision. A pearl at the
collision destination cannot retroactively establish that eligibility.

The fixed queen IDs are 0 and 1; the actor's opposite-team victim filter makes
the queen test refer to the enemy queen rather than the current longest
opponent. Head deaths, no-action pointers and queen deaths outside our acting
turn remain separate observations.

## Synthetic Validation

The exclusive check extracts the subject's nested `finish()` function from
its AST and runs fabricated in-memory event attachments. It neither invokes
the engine nor reads competitive outcomes. All 13 cases pass:

- Three-step collision classified as two successful updates plus collision.
- Four requested steps with an immediate collision do not become a long-range hit.
- An unused fourth direction preserves actual collision step three.
- Own-queen, portal collision and nonordinary prefix excluded from the nonqueen ordinary sprint metric.
- Generic harvest movement, an unpaired victim death and an enemy-acting interval excluded from our initiated trades.
- Ordinary enemy IDs do not become fixed enemy-queen hits.
- Paid collision with prefix length two rejected; prefix length three accepted.
- Absent action pointer counted separately.

These cases validate classifier discrimination, not event generation by the
official engine. The packed-reader and topology checks remain documented in
the preceding R21 audit.

## Attribution Limits

Only the first strict-prefix actual SPLIT 2 versus SPLIT LEN-2 difference is
bound directly to the new ordinary rear factor. Later actions occur on
different trajectories. Attack-count differences therefore cannot be called
new hunt-branch activations or direct effects on terminal victory.

A requested three/four-step MOVE alone does not establish hunting:
`freeHarvest()` can emit longer surviving pearl routes, and own-queen review
can emit multi-step survival routes. Successful prefix plus paired actual
head deaths establishes an observed collision action. It remains an
observation, not private branch telemetry. Ordinary valuable-head attacks
are inherited behavior; an actual enemy-queen collision does not prove that
the queen refinement, rather than an inherited valuable-head condition,
was uniquely responsible.

The ordinary growth split precedes sprint hunting in `takeTurn()`. Queen
feeding/consolidation can bypass that function, earlier portal and adjacent
combat branches can return first, and common harvest/protection/portal/queen
reviews can replace proposals before emission. Full offline body state does
not reconstruct private trail, partial observations or planner choices.

The R22 geometry summary deduplicates same/next-round parent deaths by death
event within each replay, avoiding the previous allocation-linked count
ambiguity. Raw split pools still include emergency/protection branches and
are not ordinary activation counts. Inherited `NO_VALID_ACTION` is only the
absent-pointer display label, not an inferred death reason. Ordered log
attachments omit original engine/dragon log IDs and should not be treated as
ID-specific evidence without preserving those fields.

No blocking semantic defect was found in the reviewed subject version. No
outcome selection, statistical significance or generalization conclusion is
provided by this audit.

## Reproduction

```bash
.venv-battlecode/bin/python -B experiment_results/xxqz_robust_20261004/r22_firstscreen_parser_check.py
```

The default result already exists. A rerun requires a previously nonexistent
`--output` path; the check refuses to overwrite evidence.
