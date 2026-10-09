# H7 Queen Enemy Margin

Registered 2026-10-03 before any H7 match. Isolated child of
`tyhh_qos_counter_queen_v127`; no historical bot or opponent changes.

Single factor: queen scorer replaces the binary threat penalty of1800 with
graded known-graph enemy distance penalties. Distances1,2,3,4,5,6 cost
10000,5000,2500,1200,600,300. The worst visible enemy determines the cost.
Scouts, normal scorer weights, action pipeline, body and portal logic remain
unchanged. Bounded BFS follows only known non-kelp edges and resolved portals,
blocks current occupied intermediate cells, and allows occupied target cells.
Budget uses visible length lower bound: ceil(length/4)+max(0,length-2), cap6.
This is a ranking signal, not a safety certificate: partial enemy body vision,
future body movement, eating and unknown edges can underestimate reach.

Replay evidence from parent queen_v127 default vsV3_portal, seed78003:
A index60 queen0 moves(11,22)->(12,22); enemy17 length3 at(11,23) moves E,N,
paying for its second step, and kills the queen. B index336 queen1 moves
(18,29)->(18,28); enemy23 length2 at(19,28) movesW and kills the queen.
Flat threat penalty gives no graded preference among nearby threatened moves.
Initial opposite-edge heads can be close through toroidal wrapping; the
opening inspection must use toroidal distance rather than plain coordinates.

Primary metric: matched ranked17/both-side wins separately against V3_portal
and xxqztest; queen survival is secondary. Root freezes and schedules matches.
Focused tests establish penalty ordering and implementation boundaries only,
not enemy prediction accuracy or improved competitive performance.
