# R6: Bounded Visible Queen Evasion

Registered 2026-10-04 before competitive matches. Parent is frozen H14,
tyhh_qos_counter_queen_guard_consolidate_v127. The only factor is one final
queen-only movement review after H14 reviewQueenSurvival. No-enemy turns,
nonqueen behavior, scoring, feeding, splitting maturity and portal policy
remain unchanged.

Hypothesis: exact living actions can still put the queen within an enemy
head's short approach. A bounded known-geometry distance review can replace
such actions with a living 1-4-step escape, including legal paid steps,
without changing ordinary resource decisions in the absence of visible heads.

The review projects candidate actions with existing projectAction, retains
H14 queenActionSafe continuation and friend protection, and compares the
nearest visible enemy-head approach through ordinary open visible edges.
Projected queen body and static other bodies block these approach paths.
Approach depth is four, with 128 popped nodes per enemy. Missing edges,
hidden cells, excluded portals and exhausted budgets create uncertainty;
they cannot earn an unreachable-is-safe benefit. Replacement requires a
strictly larger certified distance than the original; ties select final
length and then fewer steps. A distance of five only means no known approach
within four under this bounded static model, never an attack-proof claim.

Risks: static blockers ignore future body release, enemy turns and future
splits; visible enemy body length is only a lower bound. The distance model
is an evasion heuristic rather than an exact legal enemy attack simulator.
Paying tail length can reduce queen score, and extra movement can leave less
room on later turns. H14 last resorts remain when no strictly better proven
replacement exists. No matches are run by this implementation task; root
owns paired same-seed evaluation and preserves all negative results.

Focused validation will cover two candidate proximity ranks, projected and
static body blocking, unknown-frontier refusal, no-enemy and nonqueen
identity, team-B queen identification, paid-step eligibility and portal
rejection. H14 control is included without modifications to verify the
behavior difference on the same proximity fixture. Runtime/resource limits
remain unproven until separate judge-mode validation.
