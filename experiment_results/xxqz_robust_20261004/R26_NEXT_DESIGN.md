# R26: Nonqueen Shorter-Enemy Incoming Head Trades

Recorded 2026-10-08 after the negative R25 screen and before R26 competitive
matches. Parent: frozen H14, unchanged xxqztest and V3 regression opponent,
native unswbc 1.2.7. Only audited development bases 82001/83001/84001 were
read. Reserved 91001/92001/93001 remain unopened.

## Hypothesis

H14 ordinary nonqueen movement can retain a head next to a currently visible
shorter enemy even when an ordinary living alternative exists. Its ordinary
shorter-head score penalty is capped at 120 points (40 during attrition),
whereas collecting a pearl adds 500. Its final friend, portal and queen
reviews do not review hostile nonqueen incoming head trades. Deaths sacrifice
more collected length than the shorter enemy, and shorten further collection
time. This motivates avoiding a completely witnessed incoming first-free-step
head trade before it occurs, including movements without a pearl.

Mechanism success means a committed action removes the registered witness
and avoids that immediate recorded head death. It does not imply a winning
game: R25 already demonstrated improved local survival without an xx win gain.

## Breadth And Limits

A read-only strict body reconstruction of all 102 audited H14/xx games
rechecked replay hashes and reconstructed final standings. It found 2,312
enemy-initiated nonqueen head deaths. Of these, 1,077 follow known living
ordinary movements with a complete-current-view enemy first-step witness;
the actual attacker action, attacker body and victim posthead match the
recorded witness. Original actions are 1,060 single steps and 17 two steps.

There are 779 shorter-enemy deaths; 581 have a currently ordinary living
one-step alternative and continuation that reduces shorter-head witnesses,
including 239 events in lost games. Most start at length three (449/581),
not length five. A pearl-only LEN>=5 slice has only one alternate/death,
and a postlength-minus-enemylength >=3 condition leaves only 38 actual
deaths and 14 alternate cases. Those restrictions would miss the dominant
observed opportunity and are not the proposed policy.

Requiring an alternative with zero first-step witnesses from any complete
enemy, and excluding an actual enemy queen attacker, leaves 556 observed
shorter-enemy deaths in 73 games, including 226 in losses. Of these, 540
originals are single steps, including 220 in 32 lost games across 15 maps.
These are geometric opportunities, not candidate activation counts. The
diagnostic does not reproduce private H14 branch state, friend protection
risks, or the candidate's rejection of newly exposed partial enemy heads.
Multi-step originals can be contested-pearl sprints or free harvesting;
they must not be labeled ordinary scorer decisions from final output alone.

The source-derived rationale is generic. The unchanged opponent also
prioritizes incoming enemyAttackRisk before raw score, but its broader
short-distance heuristics, partial-body uncertainty, paid attacks and
split attacks are not copied into R26.

## Actual Loss Witnesses

The primary early witness is default, seed82003, H14 seatA, round20 id2.
Source replay:
`../queen_rules_20261003/robust_h14_82001/0039_xxqztest_default_seed82003_A.replay`
SHA256 `03558c792b675643b83c1533f7ba44b5ac50b53aa0e3dfa6b5ad2652235c2bdf`.
Action event3226 is MOVE W. Own body is
[(31,18),(0,18),(0,19)], LEN3, POP8. W eats the pearl at (30,18),
becoming LEN4. Completely visible enemy3 has body[(30,19),(30,20)],
LEN2, and takes its free MOVE N at event3234; both heads die at event3235.
MOVE N instead retains LEN3 at [(31,17),(31,18),(0,18)] and has ordinary
N continuation with no complete-visible incoming first-step witness.

Both official scripted arms reproduce 3,219 identical preceding physical,
SONAR and action events, prefix SHA256
`90d43403957d6e6dbeea8da599e1c9630f44377f52ef25dbc5a66143d67a8f64`.
Identical raw input SHA256:
`1c8427502ca20b1879f195d3fc7f10bcb0e36490bfa93bab4f520238f4a03852`.
Raw body proof reconstructs unique predecessor links from segment facing,
checks own LENGTH, and proves every possible enemy-tail extension is
ordinary/current-visible or a wall. It does not repeatedly reverse facing.
Fresh frozen H14 emits W; the current R26 source emits N. Official W kills
both heads, while official N survives the same recorded enemy N action.

The second witness is australia, seed83001, seatA, round352 id341, POP62,
LEN5, MOVE W event206654, pearl(2,4) -> LEN6. Complete enemy414 LEN2
[(2,5),(2,6)] takes N event206811, causing head death206812. Ordinary
N instead retains LEN5. Source replay SHA256:
`0120e065da9808750aedf8736dc155dc78c5dc31325f801798b6594141485fed`.
The 206,298-event prefix SHA256 is
`c70e93c5d2da57a6b91c716b12d71fe083fd4ea114ebc50632d4ea914a589fd2`.
Raw input SHA256 is
`4466959f64ce5966f07e63532b907b664184c5f9333bdfdf3e0155ed83d14428`.
Fresh H14 W / R26 N and both exact scripted rule arms also passed.

Evidence: r26_headtrade_fixture.py, r26_headtrade_actual_state.json,
r26_candidate_actual_native.json and the four r26_{default,australia}_
{original,alternate}.replay files. The native results are fresh-input
controls, not a replay of prior-round private bot memory. Alternate arms
hold other actors' recorded actions and SONAR fixed; live reactions can
differ. Engine SHA256:
`26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`.

## Single Factor

Use H14-child final reviewShorterHeadTrade after all inherited movement
reviews. Admit a nonqueen living ordinary movement of at most four
directions with its complete unique own predecessor chain agreeing with
ownSeg and LENGTH. Collect complete visible enemy bodies using the same
corrected predecessor/tail-boundary proof. Trigger only when an enemy
nonqueen shorter than the original projected own length can take its first
free ordinary step directly onto that projected head. Enemy queen trades
are exempt because queen elimination is a primary scoring objective.

Consider only current-visible ordinary living single-step replacements
with a current ordinary continuation and no hard, soft or direct friendly
protection risk. Require zero remaining confirmed shorter-enemy witnesses
and no newly added first-step head exposure from any currently visible
enemy head, including partial bodies. Rank accepted replacements by the
unchanged H14 scorer and existing NESW tie order. Keep the original if none
qualifies. Do not add distance penalties, attack search, mission memory,
split suppression, portal exploration or a phase cutoff.

This is a review of the committed final movement, so it also covers living
contested-pearl/free-harvest paths when they satisfy its proof. It preserves
the earlier decision ordering; suicidal direct head attacks, donations,
splits, queens and nonordinary/unknown paths do not pass the entry gates.
It does not assert that an enemy acts next or that a zero witness action is
globally safe. Intervening actors and hidden or longer attacks remain unknown.

R5 changes growth-split admission before movement; R6 changes queen
distance-based evasion; R20 changes same-length queen first-step exposure;
R25 changes quiet LEN2 terminal pearl pickups. R26 concerns hostile
nonqueen first-step length-losing trades and allows a resource sacrifice
when a proof-supported alternative exists. It is not an R10 attack-order
or R21 split-allocation combination.

## Validation And Screen

Before games: production O2 build, candidate/H14 source controls for both
actual inputs, all inherited suites, independent source review, and bound
the review's runtime on crowded current views. Focused controls must cover
curved/wrapped own and enemy chains, ambiguous/partial chains, hidden tail
side neighbors, portals and unknown edges, postgrowth enemy equality,
enemy queen exemption, direct attacks, donation/split/queen exclusion,
friend direct/hard/soft risk, newly exposed partial enemy head, no alternate,
ordinary continuation and freeHarvest/contested-pearl final paths. Preserve
actual native N and corresponding official action arms after source freeze.

One registered 68-game 82001 screen uses ranked17 maps, both seats, unchanged
xxqztest plus V3, source-frozen candidate, unchanged runner and shared cap4.
Estimated CPU-only six minutes and 0.4GB plus verification; no GPU/API cost.
Recheck complete manifest/outcome audits and paired H14 comparisons. Report
xx/V3 and A/B counts, paired promotions/regressions, all negative outcomes,
strict-prefix candidate activations, first-step witness elimination,
actual immediate head deaths, later pickup/length and runtime separately.
Do not infer collection or wins from the two passing rule fixtures.

Falsification: no committed action changes means the proposed policy does
not reach production decisions; remaining immediate witnessed deaths after
accepted changes refute its narrow local mechanism; fewer deaths with no
xx gain refutes the intended competitive benefit on this screen. A negative
screen is retained without expansion or coupling a rescue factor. Expand
to 83001/84001 only after an interpretable promising screen under the fixed
continuation rule. No confirmation seeds until a finalist qualifies and
is frozen. The original robustness preferences remain unchanged.

Risks include foregoing pearls or intentional useful trades, longer-lived
body crowding, less enemy attrition, uncertain future actors, hidden attack
routes and additional scoring work. Complete current-view proof bounds
what is known; it is not a survival guarantee.
