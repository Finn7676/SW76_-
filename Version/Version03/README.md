# mybot_cpp

C++20 port of `Version/mybot/main.py`. The Python original is left intact so you
can compare both bots. This is a modular language port, not a strategy redesign.
No compilation, tests, or matches were run for this port, as requested.

## Files and team ownership

| File pair | Responsibility | Owner |
|---|---|---|
| `main.cpp` | Official input/output loop and exception fallback | A |
| `bot.hpp` / `bot.cpp` | Wire modules together and execute each turn | A |
| `strategy.hpp` / `strategy.cpp` | Target priorities, normal and emergency splits | A |
| `navigation.hpp` / `navigation.cpp` | Body simulation, bounded BFS, nearest goal | B |
| `safety.hpp` / `safety.cpp` | Legal moves, visible pockets, dead ends, fallback | B |
| `scoring.hpp` / `scoring.cpp` | Move scoring and deterministic direction ties | B |
| `memory.hpp` / `memory.cpp` | Map/pearl/body memory, spawn timers, portal observations | C |
| `geometry.hpp` / `geometry.cpp` | Wrapped distances and directional preferences | B/C |
| `types.hpp` | Shared position, route, target, simulation and option types | All |
| `config.hpp` | BFS limits and replay diagnostics toggle | A/D |
| `helper.hpp` | Existing official C++ controller/protocol helper copied from Version01 | Infrastructure |
| `bot.toml` | Official C++ build/package manifest | D |

All source files are at the project root, so the official tool compiles every
`.cpp` translation unit. Each header defines an interface; implementations live
in the corresponding `.cpp` file. Bot state is owned by `Bot`, not global variables.

## Data flow

```
main -> Bot::execute_turn
     -> Memory::observe
     -> Strategy::choose_target
     -> Navigation::local_bfs
     -> Safety::classify_steps
     -> Strategy split OR Scoring::pick_from_options
     -> controller action + Memory update
```

`Cell` is an `(x, y)` pair. `Cells` is a set of positions. `Moves` is a direction
vector. `Path` is an optional direction vector: an empty vector means already at
the goal; no value means no route. `Transition` separates edge type from an
optional destination; unresolved portals have no destination.

## Preserved behavior

- Target priority: visible pearls, remembered pearls, imminent spawns, exploration,
  earliest known respawn, then wandering.
- Same scoring weights, BFS depth 12 and approximately 220-node budget.
- Normal moves reject portals, despite portal-aware memory and BFS.
- Actions remain single-step moves. Multi-step simulation does not enable sprinting.
- The enclosed-pocket degree heuristic still classifies some cycles as dead ends.
- Normal population targets remain 4/5/8, depending on map area.
- Emergency splitting retains a length-2 parent and can exceed the soft population
  target while obeying the engine limit.
- The original adjacency-based body reconstruction and stale-pearl behavior remain.
- No new endgame policy, role system, sonar strategy, or threat predictor was added.
- Replay indicators are enabled by default, matching the Python program.
- Python's insertion-order respawn tie-break is preserved through `eta_order`.

C++ sets iterate in coordinate order instead of Python's hash order. Explicit
nearest-goal and N/E/S/W scoring ties are preserved. Exact replay identity is not
guaranteed for incidental container iteration or malformed portal data; compare
behavior and results in your own tests.

## Build and run when ready

From the `UNSW_battlecode` workspace root:

```sh
unswbc run maps_BeforeSprinter/arena.map Version/mybot_cpp Version/mybot
python3 benchmark.py Version/mybot_cpp Version/mybot --maps arena --games 2
```

Or compile manually from this folder (compile all `.cpp` files, not only main):

```sh
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic *.cpp -o mybot
```

For submission, pass this folder to the official CLI:

```sh
unswbc submit Version/mybot_cpp
```

No submission was made as part of this port.
