# R8 Combat Before Routine Growth

Parent: frozen R4 `tyhh_qos_robust_sprint4_v127`, main SHA-256
`455fbc4744a2634a9c529a61f6b4ee582d7d6da0ceb85d2854b81aba39b60675`.

Hypothesis: an eligible length-at-least-four scout currently splits before
R4's exact 1-4-step attack can run. Prioritizing the already validated attack
over routine population growth may remove valuable visible enemy heads
before their next turn. The possible cost is fewer scouts and excessive
trade-up deaths; no win-rate benefit is asserted before paired evaluation.

The sole production factor is the call order. The existing
`sprintRamAction(champion, attrition)` block moves immediately after
`FEAT_RAM_FIRST`, before routine splitting. Its function, thresholds, node
budget, target ordering and exact-body semantics are unchanged. Early portal
entry, fixed enemy-queen attacks, adjacent first-ram and late sacrifice stay
ahead of it. Ordinary growth, other adjacent attacks, sprint eating, all
post-action protection, queen feeding and consolidation remain unchanged.

Only the five source/config files were cloned from R4; R7 is not an ancestor.
Helper, bot config, queen feeding and consolidation must remain byte-identical
to R4. R4 source is never edited.

Focused validation must exercise `takeTurn`, not only the attack helper:
split-eligible scouts with valuable heads 2, 3 and 4 ordinary-open steps away
choose the exact attack before growth; a pearl-supported length-four route
also outranks growth; without a target, routine splitting still occurs.
Queen bootstrap and established champion decisions must match R4. The same
fixture is compiled against R4 with parent expectations, proving that the
test distinguishes the changed branch order. Four-step fixtures remain
inside the current 7x7 view by using a bent route.

Native build and focused tests are required before root freezes or evaluates
this candidate. This implementation task launches no matches. Root owns the
multi-map/multi-seed schedule, paired controls, independent review, audit and
retention of negative results.
