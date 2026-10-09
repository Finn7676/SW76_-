# R3: Late Visible Donor Routing

Registered 2026-10-04 before competitive matches. Parent is frozen H14
`tyhh_qos_counter_queen_guard_consolidate_v127`.

Hypothesis: late consolidation currently depends on incidental proximity.
Routing eligible quiet nonqueen donors toward currently visible qualified
nonqueen receivers may increase opportunities without loosening death rules.

Isolated factor: when the existing consolidateReceiver cannot approve an
immediate donation, a donor satisfying the existing quiet, population and
length constraints may take one proven safe ordinary-edge step toward a
currently visible receiver. Receiver length must exceed donor length, or be
equal with lower ID. Ordinary known-open visible BFS is capped at depth 6
and 128 expanded cells; all occupied cells are excluded. Targets must have
the original consolidatePickup distance at most 2 and sufficient original
collection horizon plus routing distance. The first step must have an exact
known surviving projection and no direct friendly collision, hard friend
exit risk or current forward-yield risk. Every turn recomputes current
receiver heads, not stored targets.

Unchanged: queen feed and suicide eligibility, consolidate receiver race
review, recipient hold, early exploration, expansion maturity and queen guard.
This routing neither authorizes a portal step nor guesses hidden geometry.
Tests cover a productive path, queen/enemy/length/ID rejection, current view,
unknown or portal edge rejection, occupied paths, depth and time horizon.

Root owns paired xxqztest evaluation, retention of negative results and
source freezing. No competitive matches are launched by this task.
