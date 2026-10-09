# Crown development seed 73001: Portals / V3

Read-only diagnosis, 2026-09-29. Production source remains frozen at the
manifest in tyhh_qos_relay_crown/READY.md. Both candidate games lose.

CROWN_DONATE, CROWN_APPROACH, CROWN_COLLECT: zero each on both sides,
counted from replay log events, not CLI stdout.

| Candidate side | Version | 400: max / population | 450: max / population | 500: max / population |
|---|---|---:|---:|---:|
| A | baseline | 4 / 24 | 5 / 27 | 8 / 15 |
| A | crown | 4 / 24 | 5 / 27 | 10 / 16 |
| B | baseline | 8 / 33 | 5 / 31 | 9 / 20 |
| B | crown | 8 / 33 | 5 / 31 | 9 / 20 |

These are completed-round snapshots from the existing replay analyzer,
which checks final population, maximum length, and total length against
replay standings. Baseline and crown source paths are their respective
0007_V3_portal_portals_seed73001_A.replay and
0008_V3_portal_portals_seed73001_B.replay.

## First blocking gates

Reconstructed all action-time bodies from initial map plus movement,
split, and death events. At every crown action, a toroidal 7x7 head view
updates its last locally observed enemy round and first observed round.
The following optimistic upper bounds deliberately omit sonar enemy
reports and use each receiver's actual complete length, rather than
restricting length to what the bot can verify.

| Rounds 400–499 | Side A | Side B |
|---|---:|---:|
| Own action turns | 2415 | 2973 |
| Enemy body visible on current turn | 2059 | 2531 |
| Local recent-enemy quiet window fails | 2388 | 2945 |
| Local quiet plus minimum age passes, upper bound | 11 | 7 |
| Also satisfies donor length, reserve and horizon, upper bound | 10 | 6 |
| Also sees an actual-length-qualified friendly recipient head | 0 | 0 |

No competitive counterfactual is inferred. The first readily identified
barrier is local enemy exposure; receiver availability then blocks the
few remaining optimistic donor cases. Remote sightings may block further
turns, but are not needed to explain zero donation/approach activation in
these two games. Snapshot-only reasoning would have missed this stronger
local-exposure evidence.

## Next-step boundary

Earlier ordinary split cessation may create larger receivers, but does
not automatically remove the local quiet barrier. A separate
mature350 + crown combination is a legitimate new hypothesis, not an
established improvement and not permission to modify this frozen
candidate. No matches or production edits were performed for this
diagnosis. The root agent owns comparison selection and final validation.
