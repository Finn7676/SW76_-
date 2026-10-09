# XXQZ Multi-seed Robustness Continuation

User reported H14 loses to xxqztest on other leaderboard seeds. Previous
three-seed pooled-side pass is retained, not evidence of robust superiority.
No external replay/settings have yet been supplied. Local engine remains1.2.7.

## Fixed Evaluation

Ranked17 maps, both A/B sides, unchanged xxqztest primary and V3 regression.
Each seed base adds ranked map index:68games per version per batch. Primary
metric: wins versusxx/34; draws non-wins. Also report side, map, old10subset,
queen,longest,total and actual failures. Candidate advantage requires paired
same engine/maps/seed/sides/binaries. All negative versions and replays stay.

Development bases82001,83001,84001 are preregistered before launching matches.
First reproduce H14 on all3; new isolated candidates initially use82001, then
promising factors run the other2. Baseline H5 is existing, not modified.
Previously used78001/79001/80001/81001 results are historical diagnostics,
not held-out in this continuation. Reserve unused bases91001,92001,93001 for
frozen finalist confirmation; do not view those results before freeze.

Robustness preference: >50% vsxx in each of3 held-out batches, pooled>55%,
both sides individually>50%, no pooled V3 regression greater than5points
against matched H14. If none satisfies it, report the best verified gains and
remaining weaknesses; do not redefine the criterion from observed scores.
Three seeds: mean+/-sample std/range only; no significance test.

## Hypothesis Matrix

| ID | Parent | Factor | Expected observation |
|---|---|---|---|
| B14 | frozen H14 | No change | Quantify variance and failed map/side slices |
| R1 | H14 | Bounded short-scout route to known unvisited portal before routine split, adapted from prepared H15 | Earlier entry to resource rooms, more resources before late game |
| R2 | H14 | Witnessed eligible recipient keeps ordinary split hold until500, not12rounds | Keep consolidated length rather than fragment it again |
| R3 | H14 | Quiet donors route to currently visible qualifying receiver; death threshold unchanged | More local opportunities for proven collection, no remote suicide |
| R4 | H14 | Nonqueen active sprint attack searches up to4steps with v127 exactbody/payment | Better early swarm attrition and queen/carry hunting |
| R5 | H14 | Nonqueen routine split requires proven parent continuation plus child escape | Less immediate self-death from growth before survival |
| R6 | H14 | Queen final threatened-action evasion by known projected-body head-distance, max4steps | Reduce early head deaths using legal multi-step retreat |
| R7 | R3 | Add frozen R4 sprint search, completing route x attack factorial | Test interaction of late approach and early deeper attack; no assumed additivity |
| B5 | H5 | Remove only H14 queen final guard | Check previously observed rear-side guard regression across newseeds |
| R8 | R4 | Move unchanged exact sprint branch before routine split | More actionable scout head trades, with lost population risk |
| R9 | H14 | Bounded approach to a currently visible queen before routine split | More primary-score delivery under unchanged local death gate |
| R10 | R4 | Visible enemy queen target-value priority in unchanged exact sprint search | Match primary scoring priority, no new topology/step model |
| R11 | R3 | Fresh sonar remote receiver navigation after visible approach fails | Broader assembly without remote suicide or guessed safety |
| R12 | H14 | Quiet short donors reserve locally collectible pearls for a visible larger recipient | Avoid fragmentation on portals without globally stopping exploration |
| R13 | R10 | Same combat-before-growth call order as R8 | Complete queen-value x attack-order factorial, test interactions |
| R14 | R12 | Reserve pearls only when all admitted alternatives are proven living with continuation | Prevent a pickup penalty from choosing a blind portal |
| R15 | H14 | Defer routine split in a proven terminal pearl pocket when collecting first enables existing reverse-split escape | Avoid repeat split-to2, eat-to3, neck death while keeping split fallback unchanged |
| R16 | R13 | Permit exact sprint search from length2, with unchanged earlier-pearl payment checks | Recover legal pearl-funded short-scout attacks without allowing unfunded paid steps |
| R17 | H14 | Stop routine expansion after350 when POP>=8 and no current enemy part is visible | Retain local resource growth without changing donation acceptance or emergency splitting |
| R18 | H14 | Force early portal exploration only for portal IDs not personally crossed before | Avoid repeated forced exits from resource rooms while leaving normal navigation available |
| R19 | R10 | Add R17 local late-growth gate to queen-hunt parent | Test whether R10's xx gain and R17's retained length interact positively |
| R20 | H14 | Final queen movement review reduces completely visible enemy first-step head-swap witnesses without losing projected queen length | Avoid witnessed immediate head trades while preserving all other H14 decisions |
| R21 | H14 | Before350, eligible nonqueen ordinary growth retains2 and allocates LEN-2 to the child instead of always allocating2 | Permit earlier same-round rear-child cascades under the existing growth admission gates |
| R22 | R10 | Add only the frozen R21 early ordinary split allocation | Complete H14/R10 attack-package by ordinary/rear allocation interaction screen |
| R24 | H14 | Increase complete ordinary resource-room proof bound24->49 | Admit fully known compact resource rooms without changing portal or exploration safety |
| R25 | H14 | Final review rejects only quiet complete-body LEN2 ordinary pearl terminals when a reviewed ordinary living alternate and nonterminal continuation exist | Preserve short collectors without bypassing friend protection or changing split/portal/combat policy |
| R26 | H14 | Final ordinary nonqueen review rejects complete shorter-enemy first-step head trades when an unchanged-score ordinary replacement removes them without adding visible head exposure | Preserve collectors otherwise exposed to economically unfavorable incoming rams |

Continuation resource addendum: R8 firstscreen warrants the remaining two
development batches (136additionalgames). R9/R10/R11 get one68game screen
each; only promising isolated factors expand. Shared global cap4 is unchanged.
Allow roughly20additional CPU-only wallminutes plus auditing and focused
builds, about2GB additional storage. No GPU or external API cost. Reserved
91001/92001/93001 remain untouched until a finalist is source-frozen.

R17 is a new H14-child, not a pooled claim about the older H11 maturation
experiment (different parent/queen guard and seeds). Fixed350 starts length
retention at the existing consolidation phase; current visible-enemy gating
does not assert hidden safety and does not permit suicide. R17 retains all
emergency/protection splits and the low-population refill below8. Firstscreen
68games on82001 plus focused source-bound H14 controls; expand only if useful.
Allow about6CPU-only wallminutes and0.4GB storage plus audit. No new API/GPU
cost or confirmation-seed use. Expected benefit is less length4/5 routine
fragmentation; risks include less exploration/refill and more body traps.

R18 observed default evidence:82003B id5 entered portal0 at26 then forcibly
left at39;84003B id5 entered at46 then left at55 despite a currently visible
adjacent pearl with continuation. Resource room proof cap24 misses25-cell
rooms, but R18 does not tune that cap. Record personal successful portalPID
crossing only, reset across actors, and skip only the early forced branch.
Ordinary portal scoring, first exploration, navigation, and queen/long-body
safety remain unchanged. No inherited or relayed traversal claim. Test actual
counterfactual scoring before games; do not launch if it never changes action.
Firstscreen68games82001, estimated6CPU-only wallminutes and0.4GB plus audit.

Queen/longdragon unknown-portal restriction remains. No blind queen crossing
or unvalidated recovery guarantees. No opponent or production edits.

## Resource and Execution

CPU-only, shared maximum4matches. Existing frozen runner retained. H14
baseline204games, initial5candidates+B5control408, two promising candidates272
additionalgames, finalist/reference408confirmation:maximum1292games,
estimated90minutes CPU-only walltime including builds/audits, roughly7GB
storage; no GPU/API cost. Expand only promising factors after initial results.
Stop first wave for evidence before expanding. Results and commands are below
the existing queen_rules_20261003 runner directory with prefixrobust_* to use
the same globally locked cap and audit tool; this folder holds planning/report.

```bash
.venv-battlecode/bin/python experiment_results/queen_rules_20261003/run_suite.py --label robust_h14_82001 --candidate tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127 --maps all --seed 82001 --workers 2
```

Analyze completed suites with analyze.py, compare exact paired audits using
compare.py. Freeze each newcandidate before launch. Aggregate no results across
different candidate versions. A source/runtime failure invalidates that suite
and is retained with a newlabel for retry. Intentional SUICIDE is not crash.

R19 interaction screen completed on82001 and audited: V3 14/34,xx16/34, with no expansion. It is retained as a negative combination result; no claim is pooled with isolated R17/R10.

R20 preregistered before competitive games. A complete ordinary-edge enemy
body must be proven from the current view; its first ordinary step onto the
projected queen head is a local legal attack witness. Inputs cannot establish
the global next actor or unchanged occupancy after intervening actors. Hidden
enemies, incomplete bodies, unknown/portal geometry and longer attacks remain
unknown; eliminating this witness is not a global safety proof. Search queen
alternatives up to4initial free steps with exact existing body projection, known
ordinary edges, surviving actions, visible continuation, no friendly
collision/protection risk, exactly original projected queen length,
and strictly fewer proven witnesses without adding another complete-visible
enemy witness. Expected benefit is fewer avoidable queen head trades; risks
include less collection, stale static geometry and unmodelled attacks.

Require the actual dilemma84006A r199 official1.2.7 N-versus-NN fixture,
source-bound candidate/H14 controls and fresh independent review before
launch. Firstscreen82001 is68games using the unchanged ranked17 A/B protocol;
estimated6CPU-only wallminutes and0.4GB storage plus focused builds/audit,
no GPU/API cost. Expand to83001/84001 only with interpretable xx gain; reserved
confirmation seeds remain unopened. Root launches and audits the suite.

R20 implementation clarification before games: use predecessor links for
bent bodies, not repeated reverse facing. Enemy tail completeness requires
all possible non-kelp neighbors to be known ordinary and visible. Own LENGTH
proves its complete chain. Restrict replacements to initial free steps and
same projected length, avoiding a coupled payment or resource-gain factor.
Original N/NN engine counterfactual stays valid; the older enemy16
completeness assertion is narrowed by QUEEN_BODY_INFERENCE_CORRECTION.md.

R21 preregistered before implementation/competitive matches. Read-only
H14/xx B-seat loss diagnostics found equal early total length but lower
population, and actual slithery_fight82009B/autarky83002B split allocation
examples. These observations motivate, but do not prove, a resource or win
effect. Only change the allocation at the existing ordinary growth branch
for nonqueens with ROUND<350 and LEN>=5: use childSize=LEN-2 when the
existing growth admission, can_split and splitSafe accept it, otherwise
retain H14's split2 fallback. Queen bootstrap, all tactical/emergency
branches, exploration, maturation, donor and protection policies stay fixed.
No new survival guarantee is attached to the larger child.

Expected effect: newborn rear child can split again in its current round,
while H14 leaves the longer part with the parent until its next action.
Risks: fewer parent free steps, extra newborn occupancy, earlier crowding,
frontier direction changes and length2 portal/body hazards. Require
source-bound H14 control for LEN4/5/7/queen/after350/cap/portal cases plus
official1.2.7 split arrays and same-round cascade before firstscreen. Use
68game82001 screen, matched H14 and both opponents; expand only after a
positive interpretable screen. Budget about6CPU-only wallminutes/0.4GB plus
verification, shared cap4. Never pool R20 and R21 or open reserved seeds.

R22 preregistered before source edits/matches. Parent is frozen R10, which
includes R4 exact sprint plus queen-target priority; call this the R10 attack
package, not a pure queen-value factor. Add only R21's five-line ordinary
growth allocation arbitration; do not change LEN2 sprint eligibility, order,
payment, target values, late growth, queen or portal guards. Completing the
82001 two-factor screen tests whether R21-H14 and R22-R10 differences agree,
not whether gains add. Existing matched cells: H14 V3/xx15/16, R10 16/19,
R21 20/17 out of34each. These are screening data, not confirmation.

Expected benefit is earlier scouts/cap access combined with exact attacks;
counter-prediction is LEN2 parents lose R10's LEN>=3 sprint eligibility and
free-step budget, or population/crowding changes prevent useful hunts. R16's
negative LEN2 eligibility result does not permit jointly relaxing that gate.
R21's inherited missing-neck/portal risk remains. Require candidate/R10
source controls, preserved sprint/queen-hunt tests, production compilation,
official1.2.7 cascade fixture and independent review. One68game82001 screen,
about6CPU-only wallminutes/0.4GB plus verification. Judge R22 against R10 and
H14 on identical seeds/sides/opponents, report B-seat and paired interaction
descriptively. Expand only a qualified promising screen; held-out seeds stay
unused and no different candidates are pooled.

R24 preregistered2026-10-08 before implementation or matches: increase only
H14's provenResourceRoom cell bound24->49. Forty-nine is the radius3 square
view capacity, not a map-specific exception or a tuned seed threshold. The
17-map ordinary-component diagnosis finds two newly admitted25-cell rooms
on default; the other16maps, including portals, have no new room from this
bound change. Known ordinary connectivity, one canonical portal boundary,
finite pearl-source evidence and unknown-boundary rejection remain intact.
No change to exploration priority/scoring, splitting, queen/long portal
safety, donation, claim wire/TTL or memory semantics.

Hypothesis: the old bound structurally excludes compact resource rooms that
fit within current vision, so complete room evidence never suppresses forced
early departure or emits the existing occupancy claim. Source-bound r55
default fixture should change N recross to S pickup; r39 remains a negative
ordinary-scorer control. Their known-view histories do not reproduce sonar
or every timer and must not be called exact native replay memory. Actual
competitive first action and room occupancy are audited separately.

Require source-bound24/25/49/50 boundaries, unknown/second-portal/no-resource
rejections, r55/r39 candidate/H14 controls, inherited tests and production
builds. Official1.2.7 source-bound fixture plus fresh independent review
precede one68-game82001screen. Expected fewer premature room departures;
risks include longer proof work, excluding redundant scouts from a room,
and reduced redeployment. No other mechanism will be added to rescue the
screen. EstimatedCPU-only6minutes/0.4GB, shared global cap4, no API/GPU or
confirmation use. Expand only on interpretable xx gain under the unchanged
protocol. R23 endpoint dodge remains deferred for weak loss coverage.

R25 preregistered in R25_EXPERIMENT_DRAFT.md before implementation. Only a
quiet current-view nonqueenLEN2 single ordinary pearl step with complete
unique ordinary own body and exact postgrowth wall/own-body terminal proof
is reviewed. Replacement alternatives are known/current ordinary surviving
single steps with no direct/hard/soft friendly risk and a current ordinary
nonterminal continuation; use unchanged scorer and tie order directly.
No replacement means preserve original. Queen/split/donation/enemy-present/
portal/unknown actions remain unchanged; no R24 factor is included.

ProductionO2,38focused candidate/actual-H14 controls,3inherited suites,
actual-input fresh nativeS/E and official exact27385-prefix S,N/E,S rule
arms have passed. Independent source verdict/freeze remain required before
the single68game82001screen. EstimatedCPU-only6minutes/0.4GB plus audit,
four shared workers, no new GPU/API or reserved seed use. Mechanism fixtures
do not establish improved competitive or cross-seed win rates.

R26 preregistered in R26_EXPERIMENT_DRAFT.md before source edits/games.
Use exact complete visible shorter nonqueen enemy witnesses, not the sparse
gap-three-only gate. Final ordinary action review permits foregoing a pearl,
requires current complete own projection, replacement continuation and no
direct/hard/soft friend risk or newly added visible ordinary head exposure.
Unchanged raw scorer selects admitted replacements; attack, growth, queen,
portal, split and donor policies stay fixed. Actual input/native/official
rules, inherited source controls and independent freeze precede one68game
82001screen. Do not turn diagnostic exposures into activation/win claims.

R26 expansion decision2026-10-08 after the audited first screen and full
committed mechanism scan: xx17/34vsH1416/34,V316/34vs15/34 is a weak positive
screen, not qualification.705committed replacements/46strict-prefix first
changes, with firstactor before-next-action deaths45->0 and laterpickups2->122,
give an interpretable local effect. Global xxheaddeaths740->753 and the
legacyxx10/20->8/20 regression stay explicit. Expand the identical frozen
factor to the remaining two preregistered development bases83001/84001 to
test repeatability, as with R21's earlier one-win xxscreen. This spends136
additional games, roughly8CPU-only wallminutes/0.8GB plus official auditing,
shared worker cap4; no GPU/API or confirmation seeds. Do not modify the
factor, pool versions, treat local survival as victory, or lower final gates.

R26 development expansion completed204/204 and all official audits passed.
Three xx batches17/34,21/34,16/34 yield54/102 versusH14 52/102; B23/51
remains below50%. V3 50/102 versus53/102 is within the regression bound,
but R26 fails every-batch/pooled/B robustness gates. Keep all negative slices,
do not promote or open confirmation. R27 will test the single R26 factor
on frozen R10, completing the H14/R10/R26/R27 attack-package x review
screen; its preregistration is R27_EXPERIMENT_DRAFT.md. No additivity claim.
