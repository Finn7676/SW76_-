# R4 Exact Sprint Ram

Parent: frozen H14 `tyhh_qos_counter_queen_guard_consolidate_v127`.
This candidate changes only the proactive sprint-ram planner, at the same
pipeline stage after ordinary split and adjacent-ram policies.

Hypothesis: valuable enemy heads outside the original two-step search can be
traded for by scouts without invalid paid steps or premature body release.
Longer legal attack reach may reduce enemy queen/long-dragon survival across
maps and seeds. Win-rate improvement is unmeasured until paired evaluation.

The replacement searches 1-4 ordinary-open steps in the current visible view,
bounded by 256 examined transitions. All intermediate steps use the unchanged
exact `projectAction` simulator; only the final observed enemy-head collision
is allowed to kill the attacker. Free steps are fixed from initial length,
paid eligibility is checked before movement, collision precedes tail release,
and pearl growth precedes extra-tail payment. Other bodies remain static.
Targets maximize observed segment count, then minimize route length among
routes examined within the budget. Incomplete own-body positions reject this
proactive search rather than certify an uncertain route.

Eligibility remains nonqueen, nonchampion, length at least three, population
at least four. Original trade thresholds are unchanged: observed target
length at least initial length plus two and at least five; in attrition, plus
one and at least four. Queen safety, early enemy-queen attacks, first-ram,
ordinary splits, feed/consolidation, exploration and portal guards are intact.
No portal or unknown edge/cell is traversed by this search.

Validation is preregistered as focused assertions for four-step reach,
paid-step pearl eligibility, fixed free steps, eat-before-payment,
collision-before-tail-release, queen/champion/population rejection, threshold
and route priority, current-visibility and portal rejection. Native compile
is required before root-owned frozen multi-map/multi-seed evaluation.

No matches are launched by this implementation task. Root owns the schedule,
independent review, paired baselines, all negative-result retention and audit.
