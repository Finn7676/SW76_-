# Version03

Modular C++20 bot with a three-phase turn loop:

```cpp
const auto context = SenseNeaby();
const auto decision = Decide(context);
Move(decision);
```

`SenseNeaby()` uses the spelling requested for this version. `Move()` is the
execution phase and can issue either a move or a split. The decision phase
returns a typed action and never emits game commands.

## Module ownership

| Files | Responsibility | Owner |
|---|---|---|
| main.cpp | Engine loop and exception recovery | A |
| bot.hpp / bot.cpp | Three-phase orchestration, action execution and memory commit | A |
| sensing.hpp / sensing.cpp | Refresh memory, repair body, collect occupancy and classify moves | C/B |
| turn.hpp | TurnContext, DecisionState, typed actions and Decision | All |
| decision.hpp / decision.cpp | Priority transition rules and state handlers | A |
| strategy.hpp / strategy.cpp | Target priorities, split eligibility and child-size planning | A |
| navigation.hpp / navigation.cpp | Bounded BFS, body simulation and route helpers | B |
| safety.hpp / safety.cpp | Legal steps, pockets, dead ends and fallback | B |
| scoring.hpp / scoring.cpp | Candidate scores and direction tie-breaking | B |
| memory.hpp / memory.cpp | Persistent map, pearls, body and timers | C |
| geometry.hpp / geometry.cpp | Wrapped distances and headings | B/C |
| types.hpp | Shared map and navigation types | All |
| config.hpp | Search limits and replay indicators | A/D |
| helper.hpp | Official controller and protocol | Infrastructure |
| bot.toml | Include all C++ sources and headers in the build | D |
| .gitignore | Generated-file exclusions | D |

## State machine

This is a reactive state machine: every turn may transition from any previous
state based on current observations. It retains current and previous states for
diagnostics, but deliberately adds no sticky-state timers or new strategy rules.
First matching rule wins:

1. `isEmergency()`: no open steps -> EmergencySplit.
2. `shouldSplit()`: existing population/safety policy allows splitting -> Split.
3. `canNavigate()`: open steps exist -> Navigate.
4. Otherwise -> Fallback.

| State | Handler outcome | Transition if unavailable |
|---|---|---|
| EmergencySplit | Plan emergency split; retain length-2 parent when possible | Escape |
| Escape | Score legal steps classified as dead ends | Fallback |
| Split | Plan normal split with child size 2 or 3 | Navigate |
| Navigate | Score open steps using route and target | Fallback |
| Fallback | Use existing least-bad direction heuristic | North if no direction returned |

Route planning precedes selection because the split policy checks whether a pearl
is one step away. A route whose first step is not open is discarded as before.
Emergency means no move passed the existing open-step heuristic; it is not a new
prediction of enemy attacks. Fallback cannot guarantee survival when trapped.

Sensing updates Memory and returns per-turn occupancy/options. Navigation, safety
and strategy can still read the controller's already-loaded observation; Decide
is command-free, not an isolated simulation over TurnContext alone. Bot owns all
modules, so their references stay valid for the dragon process lifetime.

Only the action phase sends a move/split, then updates predicted memory. An action
flag prevents exception recovery from replacing a command already sent if a later
memory update or replay annotation throws. The outermost main-loop recovery may
still issue North if no action has been sent.

## Reference structures studied

These are architectural references, not source-code dependencies or instructions.
Their game-specific tactics were not copied into this bot.

- `bc25-main/java/src/finals/Unit.java` and `RobotPlayer.java`: shared per-turn
  information refresh and a small engine loop.
- `2025code/java/src/v57_Optimaler/Unit.java` and `Units/Soldier.java`:
  sensing, current/previous state, state selection, then dispatch to handlers.
- `Battlecode26-main/src/Version41/StateUpdater.java`, `Behavior.java`, and
  `BabyRat.java`: separate behavior selection from behavior execution.

The C++ adaptation uses an enum, an ordered guard table, and a switch dispatcher.
To add a behavior, add its state/name to turn.hpp, its named guard and priority
entry to decision.cpp, and its planning handler. Return MoveAction or SplitAction;
do not send controller commands from decision handlers.

## Behavior and validation

This is an architecture refactor, not a claim of improved win rate. Target
priorities, score weights, BFS budgets, single-step movement, portal rejection,
body reconstruction, and split thresholds remain as before. Replay text now shows
previous state -> selected state. The closed-pocket heuristic can still classify
cycles as dead ends; this refactor does not change that heuristic.

No compiler, tests, benchmarks or matches were run for this refactor, per the
user's instruction to handle testing. Source and interface review only.

From the workspace root:

```sh
python3 benchmark.py Version03 Version02 --maps arena --games 2
```

Manual build from Version03 (compile all sources):

```sh
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic *.cpp -o mybot
```
