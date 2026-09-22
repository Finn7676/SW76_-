# Version03

C++20 bot with `SenseNeaby() -> Decide(context) -> Move(decision)`.
Sensing refreshes persistent memory and turn facts; the decision layer returns a
typed action; Bot sends the action and updates predicted memory. The spelling
`SenseNeaby` is intentional. Moves remain single-step; sprinting is not enabled.

## Files and ownership

| Files | Responsibility | Owner |
|---|---|---|
| main.cpp | Engine loop and exception recovery | A |
| bot.hpp / bot.cpp | Module ownership and action execution | A |
| sensing.hpp / sensing.cpp | Observations, occupancy, enemy heads, movement options | C/B |
| turn.hpp | Turn facts, states and typed actions | All |
| decision.hpp / decision.cpp | Priority rules and state handlers | A |
| combat.hpp / combat.cpp | Length-2 enemy-head interception | A/B |
| strategy.hpp / strategy.cpp | Targets, adjacent-pearl check, split planning | A |
| population.hpp / population.cpp | Round-based population target and split score | A |
| navigation.hpp / navigation.cpp | BFS and body simulation | B |
| safety.hpp / safety.cpp | Legal steps, visible pockets, dead ends and fallback | B |
| scoring.hpp / scoring.cpp | Movement preferences | B |
| memory.hpp / memory.cpp | Persistent map, pearls, body and timers | C |
| geometry.hpp / geometry.cpp | Wrapped positions, distance and headings | B/C |
| types.hpp | Shared types | All |
| config.hpp | Experiment parameters and diagnostics switch | A/D |
| helper.hpp | Official game protocol and controller | Infrastructure |
| bot.toml | Packages all C++ source/header files | D |
| .gitignore | Generated-file exclusions | D |

## Decision states

First matching rule wins each turn: Intercept, EmergencySplit, ExpansionSplit,
Navigate, then Fallback. Current/previous states appear in replay indicators.

- Intercept: a length-2 dragon with another friendly dragon remaining may attack
  an adjacent enemy head. Prefer more visible enemy segments, then enemy ID.
  Only that head is exempted from obstacle checks; walls, portals, allies and all
  other body segments stay blocked. No chase or guaranteed kill is implied.
- EmergencySplit: no open steps; retain a length-2 parent when allowed. If denied,
  transition to Escape, which scores dead-end steps, then Fallback if needed.
- ExpansionSplit: score and eligibility permit creating a length-2 child.
- Navigate: score open moves using target and route preferences.
- Fallback: use the existing least-bad direction heuristic.

Only Bot emits normal engine actions. Exception recovery does not replace an
already emitted action if memory updates or diagnostics subsequently throw.

## Round-based population policy

All population parameters are in config::population. The early baseline is
`early_population_target = 20`, clamped to the engine unit limit. Desired
population depends on rounds, not map dimensions. The old map-area formula,
its enable switch, cells-per-dragon parameter and cap helper have been removed.

With current parameters and an engine limit of at least 20:

- Through round 240: target 20, score bonus 60.
- Rounds 241–320: multiplier stays 1.00, target 20, bonus 60.
- Rounds 321–500: multiplier declines from 1.00 to 0.15 and bonus from 60 to -40.
  The target declines toward 3.
- After round 500: hold the final target and bonus.

This changes the desire to create new dragons, not the number already alive.
Current population still affects expansion eligibility and its deficit score:

```
deficit = (desired_population - current_population) / desired_population
score = deficit_weight * deficit + phase_bonus
      + extra_safe_direction_bonus * max(0, safe_directions - minimum_safe_directions)
```

Expansion additionally requires a legal split, population below target, configured
length range [4,4], score >=35, and the configured safe-direction count (currently
0). A length-4 expansion produces parent 2 + child 2. Length >=6 dragons are
preserved; they also cannot emergency-split while preserve_core_in_emergencies is
true. Non-core emergency splits bypass the population target/score but still obey
engine legality and core-dragon protection. Adjacent pearls do not veto emergencies. These are tuning choices, not
a guarantee of safety or stronger match performance.

## Pearl before splitting

`defer_split_for_adjacent_pearl = true` is a hard veto for economic expansion
only. Emergency splitting ignores this veto. Each turn, check all four one-step destinations through known
ordinary open edges, including map wrapping. A currently visible pearl at any
such destination blocks economic expansion, independently of the target or BFS route.
Pearls behind walls or across portals do not qualify. This is not a score penalty.
Only adjacency to the head qualifies, not adjacency to a body segment.
The rule applies even if a body occupies the pearl tile; movement safety still
chooses the actual action. It defers splitting without forcing an unsafe move.
Interception retains its priority for length-2 dragons, which cannot split anyway.

Replay text includes the expansion evaluation: current/desired population, split
score, safe directions and the rejection reason (`adjacent-pearl` when applicable).
Interception instead reports the enemy ID and visible length.

## References and tuning

Architecture draws on bc25-main/java/src/finals (shared per-turn refresh),
2025code/java/src/v57_Optimaler (sense, select state, dispatch), and
Battlecode26-main/src/Version41 (separate behavior selection and execution).
These sources are references, not build dependencies or project instructions.

The active tyhh source inspired length-4 expansion, preserving length >=6, and
seven config::movement weights: pearl 1400, eating 300, spawn bonus 70 minus 5
per round, unseen 30, target distance 8, route 55. Other Version03 heuristics remain.
Unlike tyhh, population desire declines late; Version03 also lacks its enemy-head
risk scoring. Results need match testing. config::combat enables interception and
protects the last friendly dragon by default.

## Build and validation

Source review only; no compilation, tests, benchmarks or matches were run for
these changes, per the user's instruction to handle testing.

From Version03, compile all sources:

```sh
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic *.cpp -o mybot
```

From the workspace root:

```sh
python3 benchmark.py tyhh Version03 --maps arena --games 2
```
