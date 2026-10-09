# Phase 2 Decision Architecture

This directory keeps the Phase-1 behavior as the default. Phase-2 experiments
are independently ablatable through macros in `core.hpp`.

## Features

- `FEAT_V32_SCOUT_MISSION` (default `0`): retain the selected unexplored portal
  ID across turns, resume it after temporary tactical interruption, and clear it
  after exploration or role/timeout invalidation. Existing portal BFS and claim
  rules are unchanged. The hypothesis is less target oscillation; the risk is
  stale commitment to a portal that later becomes strategically unattractive.
- `FEAT_V32_ENEMY_RELEVANCE` (default `0`): distinguish visible enemies from
  nearby/relevant enemies using only local distance and observed length. With the
  flag off, the old visible-enemy gates are retained. The hypothesis is to avoid
  unnecessary tactical gates for distant sightings; the risk is under-reacting
  to an enemy whose route influence is not captured by the cheap predicate.
- `FEAT_V32_MISSION_EXPANSION_ARB` (default `0`): make an active Scout mission
  explicitly suppress normal expansion even when a route is temporarily not
  available. The existing valid-route suppression remains unchanged in both
  modes. The risk is delaying a profitable split while a mission is stale.
- `FEAT_V32_SEVERE_BLOCKAGE` (default `0`): classify severe blockage from local
  legal-exit geometry and friendly-head obstruction, and suppress normal
  expansion in that case. Mild crowding remains a movement-score fact. No
  generic congestion split is introduced.

`Mission` is planner-owned persistent state. Tactical policies may return an
action without clearing it; completion, invalidation, or timeout clears it.
`Situation` contains non-exclusive current-turn facts and is built without BFS.
The exploration algorithm, portal pathfinding, combat scoring, sacrifice
thresholds, and role thresholds are unchanged.
