# R2: Persistent Local Consolidation Receiver

Registered before candidate source changes or competitive evaluation.

Parent: frozen H14 `tyhh_qos_counter_queen_guard_consolidate_v127`.
Engine target: unswbc 1.2.7; the fixed queen is excluded from this factor.

## Isolated Factor

After round 350, a nonqueen receiver that personally observes an eligible
local consolidation donor keeps its ordinary expansion split disabled
through the final round (500), instead of H14's 12-round hold. Observation,
donor eligibility, collection geometry, queen visibility, and donation
logic remain unchanged. Emergency splits remain unchanged.

## Hypothesis And Expected Observation

The existing hold can expire after an accepted donation, letting a growing
receiver split its collected length back into small dragons. A persistent
hold is expected to improve endgame maximum length and avoid repeated
consolidation-then-fragmentation. This is a hypothesis, not an observed
cause or a performance claim.

## Risks

An eligible donor is not a guarantee that a donation happens. A receiver
may therefore stop exploring or replenishing population without receiving
pearls. Holding remains active if enemies later appear or population falls
below eight; emergency survival splits are still available. Long dragons
may have lower mobility or become more attractive attack targets.

## Frozen Controls And Analysis

All H14 source except the hold expiry assignment remains byte-identical.
Main/helper/queen-feed/bot configuration and opponents are unchanged.
The root experiment plan owns map, seed, opponent, side, budget, and result
auditing. Compare this candidate with paired H14 games, retaining all
losses and draws; inspect queen survival, maximum length, population, and
per-map/per-side win changes. No competitive games are launched by this
implementation task.

## Verification Plan

Source-bound C++ fixtures verify observer-ID reset, queen exclusion, no
hold before round 350, eligible-donor trigger, persistence after the old
12-round window, persistence through round 500, expiry beyond the engine
limit, and ordinary split suppression. Source comparison verifies that
emergency split code and all other policies are unchanged. Compile the
candidate natively with g++-10 before recording source hashes and READY.
