# R12: Verified Local Pearl Reservation

Registered 2026-10-05 before competitive matches. Parent is frozen H14
`tyhh_qos_counter_queen_guard_consolidate_v127`; main SHA-256
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
No production, opponent, engine, map or parent source is changed.

Single factor: after round 350, a quiet nonqueen of length 2 to 6 avoids
ordinary scored pickup of a currently visible pearl that a currently visible
queen, or nonqueen of verified/lower-bound length at least max(8, own+2), can
reach in one or two proven visible steps with a distinct clear continuation.
Another living nonpearl first-step escape must exist. A reserved pickup gets
a 2000-point score penalty; ordinary scored free-harvest cannot bypass it.
Reservation is recomputed from the current view and never authorizes death.
All earlier combat, expansion, portal, donation and safety priorities remain
unchanged. No receiver, recent threat, early phase, sole pearl exit or blocked
receiver route leaves the H14 behavior unchanged. Other pearls remain edible.

Hypothesis: portals loses longest length despite ample total resources because
short dragons repeatedly intercept pearls and fragment growth. Read-only
official replay reconstruction on H14's three development batches, six games
per map, recorded round >=350 pickups: portals H14 2507 (2312 by length <=5),
xx 1981 (1583 by <=5); late splits H14 785, xx 587. At round 499 mean total /
longest length was H14 67.33 / 10.17 and xx 55.83 / 18.17. This is association,
not proof that reservation helps. Default also has a severe resource deficit;
stripes ended before round 350, so this factor cannot repair those early losses.

Risks: a receiver may choose another goal, giving up a pearl may lower total
resources, a visible short queen may receive resources that a long nonqueen
needs, and conservative current-body proof may suppress all reservations.
Two-step proof is a heuristic, not a promise of receiver's future action.

Evaluation remains native unswbc 1.2.7, ranked 17 maps, both seats, xxqztest and
V3_portal separately, 68 games per preregistered base seed. Root owns matches
and source freezing. First development screen 82001; only a promising result
expands to 83001/84001. Unused independent seeds are reserved for a frozen
finalist. Primary outcome is wins with draws non-wins. Secondary diagnostics
are reserved opportunities, actual pickups by short/long dragons, longest /
total lengths, seat and map slices. Three seeds report descriptive mean and
sample std only. All failures and replays are retained.
