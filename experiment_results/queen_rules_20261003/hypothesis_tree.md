# Updated-rule Experiment Registration

Date: 2026-10-03. New engine cohort; do not pool with old 1.1.0 experiments.

| ID | Intervention | Fixed protocol | Prediction |
|---|---|---|---|
| B0 | Unchanged relay_counter | unswbc1.2.7, ranked17, both opponents/sides, seed78001+map index | Establish new-rule baseline |
| H1 | Queen preservation + sprint correctness + safe-portal final review + free harvesting | Same as B0 | More queen survival/length and more wins |
| H2 | Add only queen-directed donation to H1 | Same as B0 | Faster queen growth without losing queens |

H1 is an integrated rules-adaptation package. Its components are not causally
identified by B0 versus H1. H2 is a single policy addition, if run.

Primary: completed-game wins, draws are non-wins. Secondary: queen survival
and queen length, longest, total length, per map/side, intentional no-action
deaths and real runtime failures. The old ten-map subset is 40 matches; the
updated ranked17 pool is 68 matches per version. All A/B and variant matches
share the fixed same-map seed. Seeds are development seeds, not held-out.

Freeze sources/binaries/engine/maps before launch. Never edit a frozen suite
or delete failed/negative experiments. Failure reattempts require new labels.
Native tests do not establish judge-point compliance; bounded sandbox smoke
is separate. One seed per map cannot establish statistical significance.

Resource plan: CPU-only, at most four native matches simultaneously. First
completed games provide the runtime estimate. No API/GPU cost. Preserve all
logs/replays/snapshots, expected storage well below 1 GB for three suites.

## Results

B0: 26/68 wins (V3 13/34, xx13/34), legacy subset19/40.
H1: 30/68 wins (V3 14/34, xx16/34), legacy subset17/40.
H2: 31/68 wins (V3 13/34, xx18/34), legacy subset18/40.

H1 outcome is mixed: updated17-map aggregate improves, historical40 subset
weakens, queen survival improves only6/68 to7/68. Not supported as a reliable
queen-preservation success. H2 is also mixed: xx improves, V3 worsens relative
to H1, final queen survival falls7/68 to6/68. No joint victory established.

Failure attribution: H1 queen deaths are self33/head23/body5; median length
before death2, median death round index122 before feeding starts280.
Next hypothesis (not tested here): queen-specific emergency escape must avoid
shrinking to two and validate exact body-aware follow-up moves. Preserve both
negative variants; do not keep raising donor aggressiveness on these results.

All204 main games and8 separate sandbox smoke games completed and audited.
Native assertions and27 updated-engine fixtures passed. Report and raw
evidence: REPORT.md, combined_verified.json, each label/verified_audit.json.

## Continuing Goal Experiments

H3: final exact queen survival guard. Matched result V3 15/34 wins with1
draw, xx16/34; queen survival10/68 versus parent7/68. Weak development gain,
not a joint winner and not confirmed. Old10 subset19/40. All audits passed.

H6: reducing routine expansion target to32. Rejected in matched development:
V3 13/34 and xx13/34, versus parent14/34 and16/34; old10 subset15/40.
Two wins gained, six lost. All audits passed; retain negative suite.

H4 body lookahead: V3 13/34 with1draw and xx13/34; queen survival9/68.
Four wins gained, eight lost. Reject this setting for integration; no joint gain.
H7 graded queen threat: V3 15/34 and xx13/34, survival7/68 unchanged.
One win gained, three lost. Reject this setting for integration.
H4/H7 audits all passed. Queen-specific metrics cannot replace the win target.

H8 maturation350: V3 18/34 and xx16/34, old10 subset23/40; six wins gained,
two lost versus parent. Queen survival6/68. Promising length-policy factor,
not jointly qualified. H9 feed x maturation and H10 guard x maturation are
registered factorial completions, not presumed additive wins.

H5 local nonqueen consolidation: V3 16/34 and xx21/34, old10 subset22/40.
Seven wins gained, one lost versus feedparent. All68 audits passed. Actual
collection is under separate event audit; do not infer it from policy logs.
H9 feed+maturation: audited18/34 V3 and17/34 xx, not jointly over50%.
H10 guard+maturation: audited17/34 V3 with1draw,15/34 xx; no joint gain.
H11 consolidation+maturation: audited17/34 V3,18/34 xx; no joint gain.
H12 maturity300: audited14/34 V3,18/34 xx; worse V3 than350.
H13 consolidation+maturity300: audited16/34 V3,18/34 xx; no joint gain.

H5 mechanism audit:2025logged donor suicides,1837with intended recipient
collection and2326 directly observed recipient pearl pickups across43games.
This supports actual local transfer, not guaranteed collection of whole bodies.
H14 exact queen guard + consolidation qualified development18/34V3 and19/34xx,
old10subset23/40; no maturation changes. Frozen three-seed independent
confirmation passed the preregistered aggregate gate:V3 56/102,xx54/102.
Matched reference37/102,45/102; all408confirmation games audited. Some
perbatch/side/map slices remain below half; no significance claim is made.

H15 multi-step short-scout routing to unvisited portals: built and8focused
checks passed, read-only review found no blocker. Not competitively run;
unknown outcome. Retained after H14 reached the registered stop condition.
