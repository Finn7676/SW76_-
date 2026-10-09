# H1 safeportal — counter base

Registered 2026-10-01 before any candidate matches. Parent is the frozen
`tyhh_test/tyhh_qos_relay_counter` (main SHA-256
`ceaa0183760c064b25141cb8f7cbe2d73213050a760942db0768419572529873`, helper
SHA-256 `6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`).
The V3_portal and xxqztest opponent directories are unchanged.

This version tests one intervention: for dragons of length >= 8, a portal is
allowed only after this dragon has observed the exact directed crossing and
recorded its landing (`learnedExit`), and the landing is currently visible,
unoccupied, has at least one continuation other than an unverified portal
return, and has sufficient reachable space. A topology pair or sonar report
alone is not treated as a safety proof because visual state cannot establish
far-side occupancy or stale exits. Long-dragon portal choices are logged as
`SAFEPORTAL ... allow/reject`; the final protection review also checks early
portal, tactical, sprint, scored, and emergency fallback actions. Rejected
actions are rerouted to a scored legal move or split when available. If every
legal edge is an unconfirmed portal, the liveness-preserving trapped exception
is explicitly logged as `SAFEPORTAL ... trapped_exception`; this is an
unavoidable engine-action boundary rather than a claimed safe crossing.

Dragons shorter than 8 retain counter's portal exploration so they can learn
new exits. Combat, sacrifice, scoring, split thresholds, sonar, and teammate
protection are unchanged. `SAFE_PORTAL_MIN_LEN=8` is a single registered
threshold, not a parameter sweep. The module is evaluated as one factor;
landing, space, and final-review subchecks are not separately attributed.

Evidence and validation are in `verification/safeportal_tests.cpp.txt`.
The test covers pair knowledge without an observed crossing, current landing
occupancy, short-scout exemption, and no-continuation rejection. Native C++20
and ASan/UBSan builds passed; inherited parent verification files are retained
with `parent_` names and do not constitute results for this candidate.

Root schedules the fixed-seed 76001, ten-map, both-side, 40-game suites against
both unchanged opponents. Failed games and no-valid-action deaths are retained
and interpreted separately from runner faults.
