# Active portal Scout experiment

Base: `tyhh_test/tyhh_qos_protect_route` as of 2026-09-28. The original is unchanged.

This variant adds a bounded task for length 2–8 dragons when population is at
least 6 and no enemy head is visible. Champions are excluded. Tasks end at
round 220 on maps below 1200 cells and at round 300 on larger maps.

Known open edges and resolved portals can lead to an unexplored portal in at
most six moves. An unknown portal exit terminates the route and is never
treated as an intermediate cell. First-step checks retain the base's longer
friendly-body exclusion and require escape space for known landings. Confirmed
crossings and observed friendly body segments pointing through a portal mark
that portal locally explored. No new relay or claim messages are added.

An active task delays routine splitting and replaces unconditional early
portal entry. Combat and contested pearl sprint priorities are retained. A
visible pearl within four known moves interrupts the task when it is adjacent,
no farther than the portal, or in a reachable cluster with at most a three-move
detour. The route is recomputed each turn and its action passes the existing
teammate-protection review. Equal-distance task routes prefer visible pearls,
then the existing additive unexplored-cell/crowding/exit terms.

Ordinary routing, ordinary move scoring, protection, room-claim protocol,
population target, and champion thresholds remain those of the base.

This file describes the intervention, not a competitive result.
