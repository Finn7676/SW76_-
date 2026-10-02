# V5 local late-game feeding

Queens (fixed IDs 0/1) never donate. The remaining ordinary-dragon feeding policy is retained pending Queen-priority redesign.

From round 400 (inclusive), a receiver must be at least two segments longer than the donor; there is no fixed minimum receiver length. Donors of length 2–18 may feed while another teammate remains. Donors above length 18 may feed only when team population is strictly above 20. Length-12+ dragons retain the nearby high-value resource check and may also feed when eligible. Existing Champion/Core labels do not elect a team leader. The short-headshot exception remains limited to length 8 or below.

The short-dragon decision order is profitable adjacent/sprint headshot, immediate recoverable sacrifice, bounded local pursuit, then the existing survival/combat/collection/exploration decisions. The branch is in `Planner::choose`; movement, BFS, combat and the final teammate guard are reused. Before round 400 this feeding branch is inactive. Existing quiet-phase expansion reserves and other ordinary decisions retain their scheduling.

A new target must have a currently visible friendly head. Prefer the longest reachable receiver, then shorter approach distance. Exact visible length overrides sonar; partial bodies provide only a lower bound. The general checksummed ally-length packet supplies an estimate no more than three rounds old; the legacy ally-rally packet remains a fallback. These reports cannot create a remote feeding target. General length reports are sent throughout the game; the round-400 gate controls voluntary feeding.

`FeedTargetMemory` belongs to one Planner. It retains ID, head cell, length, last visible round, chase start, progress round/distance and cooldown. Follow a lost target's last visible head cell for less than four rounds, never sacrifice based on memory alone. Abandon after three rounds without improvement, eight total chase rounds, or visible disqualification. Pursuit then has a four-round cooldown; an immediate safe feed still outranks cooldown. Switch only to a receiver at least four segments longer with no longer BFS approach; switching does not renew the total chase budget.

Sacrifice uses `Action::suicide`, whose executor outputs no MOVE/SPLIT. Actual-engine checks confirm this causes death and pearls at the head and alternating body cells. Before committing, require a current visible qualifying receiver, a fully observed known route to the donor head within two ordinary moves, a free onward cell after pickup, and enough remaining turns including recipient ID order and a two-turn margin. Other bodies remain blocked in this check. Kelp and unknown/unresolved portal exits cannot justify a feed. Nearby enemy parts, known portal attack routes and enemies beside the donor's rear prevent feeding.

An existing high-value resource near the receiver postpones another sacrifice. High value means a confirmed friendly death pearl or at least three visible pearls within two cells of a candidate pearl. Collectors use the existing safe BFS to these resources with maximum depth four. Only observed unoccupied pearls count; no speculative resource target is created. This backlog check lets multiple donors feed sequentially without adding a queue or coordination messages.

All new values are overrideable `V5_FEED_*` macros in `core.hpp`:

| Macro suffix | Default |
| --- | ---: |
| START_ROUND | 400 |
| SHORT_LENGTH | 8 |
| RESOURCE_COLLECTOR_LENGTH | 12 |
| MIN_LENGTH_GAIN | 2 |
| LARGE_DONOR_LENGTH | 18 |
| LARGE_DONOR_POPULATION | 20 (strictly above) |
| TARGET_TIMEOUT | 4 |
| MAX_CHASE_ROUNDS | 8 |
| NO_PROGRESS_ROUNDS | 3 |
| CHASE_COOLDOWN | 4 |
| SWITCH_LENGTH_GAIN | 4 |
| PICKUP_STEPS | 2 |
| END_MARGIN | 2 |
| ROUTE_DEPTH | 8 |
| ROUTE_NODES | 256 |
| RESOURCE_DEPTH | 4 |
| CLUSTER_SIZE | 3 |

`BehaviorState::FEED` distinguishes pursuit/donation from ordinary exploration. Compile with `-DDEBUG_FEED` to log feeding decisions. The main.cpp revision/fingerprint changes force the toolkit's cpp-only build cache to recompile the changed headers.

Validation: `python3 verification/run_checks.py` runs the C++ regressions, including feeding eligibility, headshot priority, length evidence, local danger, pickup safety, pursuit memory and terminal horizons. The start-round regression checks rounds 398/399 are inactive and 400/401/429/430 are active. A native actual-engine fixture with a length-2 donor and length-20 receiver confirms death at round 400 and receiver growth to 21 without engine errors. In a separate length-4 moving-receiver fixture at round 400, no donation occurred within twelve rounds: enabling feeding does not guarantee a safe pickup opportunity. The broader `verification/feed_engine.py` suite has known receiver-first and two-donor failures documented in `SONAR_COORDINATION.md`; this start-round change does not resolve those movement/coordination limits. These are controlled correctness checks, not competitive win-rate measurements.

Remaining limits: receiver behavior and simultaneous teammate movement are predicted locally; hidden enemies and unknown portals cannot be ruled out. Tiny donations whose resulting pearl is first seen without a death clue use ordinary pearl collection. Different maps, sonar coverage, target escape speed and crowded resource piles still need competitive measurement.
