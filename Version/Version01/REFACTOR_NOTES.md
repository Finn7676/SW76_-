# xxqz structural refactor

This project preserves the behavior of `Desktop/xxqz/main_no_mapdb.cpp`.
The baseline directory is untouched. Its exact source is frozen in
`tests/baseline.cpp.txt`, SHA-256:

```
8226834a3208f5f44b2c43217265dde23e4efdf7ffc08838d83c19dac402dfb3
```

## Architecture

The reference teams inform the separation of responsibilities, not this bot's
strategy. The local Just Woke Up source separates its robot lifecycle from
communication, map memory, exploration and navigation. Confused separates the
entry loop, common turn updates, communication, movement and navigation. Generalized
Stroke's Theorem describes distinct state-selection and behavior responsibilities
in its [2026 postmortem, section 2](https://battlecode.org/assets/files/postmortem-2026-generalized-strokes-theorem.pdf).

Here, those principles become explicit state owners, mechanics modules, named
policy phases, and an executor. There are no virtual methods, behavior registries,
class hierarchies, singletons, or heap-allocated action objects.

Reference code inspected:

- [Just Woke Up Robot.java](/Users/fanlingjun/Desktop/2025code/java/src/v57_Optimaler/Robot.java)
- [Confused RobotPlayer.java](/Users/fanlingjun/Desktop/bc25-main/java/src/finals/RobotPlayer.java)
- [Confused Unit.java](/Users/fanlingjun/Desktop/bc25-main/java/src/finals/Unit.java)
- [Version41 Behavior.java](/Users/fanlingjun/Desktop/Battlecode26-main/src/Version41/Behavior.java)

### File tree and responsibilities

```text
xxqz_refactor/
├── bot.toml                 unchanged root-level source globs
├── helper.hpp               unchanged engine API/parser
├── main.cpp                 turn lifecycle
├── core.hpp                 configuration, turn state, flags, constants, action value
├── world.hpp                persistent observed/received world facts
├── topology.hpp             wrapped geometry, portal inference and landing learning
├── occupancy.hpp            private stamped occupancy and release-time queries
├── observer.hpp             full observation scan and visible-chain reconstruction
├── body.hpp                 own trail, hidden reconstruction, split geometry
├── search.hpp               bounded searches and persistent BFS buffers
├── sonar.hpp                protocol, reception, outgoing message selection
├── planner.hpp              ordered policy phases and original scoring/fallback
├── action.hpp               executor and deferred move/sprint side effects
├── debug.hpp                optional diagnostics
├── REFACTOR_NOTES.md
└── tests/
    ├── baseline.cpp.txt     byte-for-byte original strategy
    ├── regression.cpp.txt   mechanics and golden-policy regression suite
    ├── verify.py            game trace comparison and build profiles
    └── results/             saved validation reports
```

The action value is in `core.hpp` so the planner can return actions without
including the executor. Its fixed array has room for future longer actions; the
current planner still creates only one-step moves, two-step sprints and splits.

### Dependency graph

Arrows mean “uses”; every module ultimately uses `core.hpp`.

```text
main ──> Observer ──> BodyMemory ──> Topology ──> WorldModel
  │          │           │
  │          └───────────┴──> OccupancyModel
  │
  ├──> Sonar ──> WorldModel + Topology + OccupancyModel
  ├──> Planner ──> Search + BodyMemory + read-only world/turn/mechanics
  │                  └──> Topology + OccupancyModel + TurnState
  └──> Executor ──> BodyMemory + Topology + WorldModel + engine output

Debug reads turn/topology/occupancy only and is compiled out by default.
```

`main` constructs and owns the objects for the dragon's lifetime. Each module
holds non-owning references to those objects; nothing relies on a free mutable
bot global. The supplied engine helper retains its own existing parser globals.

- `GameConfig` owns dimensions and the fixed population target.
- `TurnState` owns the single round/length/population/identity snapshot and derived
  lists. Vectors are cleared/reused. A fixed 49-cell array preserves tile iteration
  order for sonar without another tile scan or allocation.
- `WorldModel` owns edge, portal, pearl, exploration and enemy-sighting memory.
  It contains only knowledge learned in the game.
- `OccupancyModel` owns private flat stamp arrays. Only observation and own-body
  reconstruction write them. Searches use query APIs, including `blockedAt`.
- `BodyMemory` owns `trail`, `pendingPath`, and `trailId`; it alone implements their
  maintenance and hidden-segment reconstruction.
- `Search` owns reusable BFS marks, distances, first directions, queue and stamp.
- `Planner` owns the reusable route-goal buffer, replacing its hidden static local.
  It does not mutate world, body, occupancy or engine state.

## Behavior preservation

The lifecycle is unchanged: read turn → resolve prior single-move portal landing
→ append pending sprint intermediates and current head → receive sonar → observe
visible and hidden occupancy → choose → execute → send sonar → end turn.

`Planner::choose` explicitly preserves the priority:

1. Derive modes and adjacent ram candidate.
2. Sacrifice.
3. Early ram.
4. Normal expansion split.
5. Adjacent ram.
6. Two-step sprint ram.
7. Two-pearl sprint.
8. Route to visible/soon pearls, then unseen cells.
9. Score ordinary moves; if necessary reverse split, split two, desperate ram,
   open-edge fallback, then current facing.

All thresholds, search bounds (24/320 for routing), scoring arithmetic, loop order,
strict tie comparisons and seed perturbations remain those of the frozen source.
The target is still `min(62, unit_limit - 2)`. Normal expansion still uses the
round-470/champion rule with the low-population exception.

`fragile` applies only to the original normal-move proximity scoring branches.
It does not inherit all champion behavior, prohibit expansion, or prohibit every
ram when population is two. Champion derivation is unchanged.

Mechanics intentionally preserved even when unusual:

- Unknown ordinary edges remain traversable. Unresolved portals return `-1` from
  `Topology::step`, but policy can still select them for exploration.
- Learned directed exits take precedence over endpoint inference.
- Hidden own segments remain occupied until the exact baseline release step.
  Eating delays own release; enemy eating/splitting uncertainty is unchanged.
- Sprint bookkeeping records intermediate cells and preserves the baseline's
  single-move-only pending portal key behavior.
- Controlled-space enemy distance remains wrapped Manhattan distance.
- Sonar receive still runs when outgoing sonar is disabled. Message layout,
  clipping, rejection rules, cap, priority and tile-order ties are unchanged.
- Debug portal prediction retains the baseline's debug-only learned-exit clearing;
  it does not affect builds without that macro. Choice logging stays after action
  execution, as in the original.

There is no hardcoded map knowledge, density/divisor population experiment,
new combat policy, or pathfinding replacement in this project.

## Build and verification

Native Clang command (run in the new project):

```sh
clang++ -std=c++20 -O2 -Wall -Wextra -pedantic -Werror main.cpp -o /tmp/xxqz-clang
```

Portable GCC command on a configured GCC installation:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -pedantic -Werror main.cpp -o /tmp/xxqz-gcc
```

This machine's Homebrew GCC 16.2 points to a missing macOS 26 SDK; validation uses
explicit include paths for its C++ headers and the installed macOS 15.2 SDK.
The full invocation and result are in `tests/results/gcc-build.json`. No SDK or
system compiler configuration was modified.

Each extraction phase was syntax-compiled. The complete bot is checked with
warnings treated as errors. The standard toolkit project build/run path is
exercised, as is its shipped judge clang and metered WebAssembly sandbox.
Validation uses toolkit 1.0.1 in an isolated temporary installation, rather than
the older system installation. A replay-viewer auto-install failure during the
CLI smoke run did not affect compilation or game execution.

Reproduce the checks with toolkit 1.0.0 or newer available to Python:

```sh
python3 tests/verify.py --output /tmp/xxqz-native-check --matrix --sanitizers
python3 tests/verify.py --output /tmp/xxqz-wasm-check --sandbox
```

Each output path must be new. `--maps arena stronghold` restricts the default
profile, and `--build-only` checks builds without playing games. Test fixtures
have a `.cpp.txt` suffix deliberately: the judge recursively collects C++ source
files and would otherwise link the test mains into the bot. The verification
script builds separate snapshots outside the project. It also avoids the current
toolkit's header-insensitive wasm cache by naming snapshots using their content.

The regression suite covers portal inference/actual landing overrides, hidden
own-body release and early-return rejection, sprint intermediates, visible chains
through portals, eating/splitting slack, unknown enemy tails, stale trail trimming,
identity reset, length-two portal safety, fragile/champion and expansion boundaries,
reverse split, unresolved-portal policy, and sonar validation/priority.
It compares against the frozen implementation on 560 boundary cases and 600
deterministically generated states, including occupancy, action output, deferred
side effects and outgoing sonar. It also runs under address/undefined-behavior
sanitizers.

Game comparisons shadow **every dragon on both teams**: baseline and candidate
receive identical spawn and turn input. Every output byte is compared, including
MOVE, SPLIT and SONAR; the first difference or bot runtime error fails the run.
When traces match, replacing either or both teams with the candidate produces the
same game. This tests both team positions more directly than comparing win rates
from independently diverging matches. It does not prove equivalence on all
possible inputs or measure performance against unrelated opponents.

Default profiles use all 11 current bundled maps. The native matrix disables
each feature individually and all features together, and tests both debug macros
and a nonzero seed on arena/stronghold. The nonzero-seed profile additionally tests
Colosseum and devil. Reports record source/map hashes, output-trace hashes, outcomes,
turn counts, runtime failures, and sandbox CPU/memory measurements.

## Results

- All 11 current maps: **181,734 exactly matching turns** on both native and
  judge-sandbox default builds; every dragon on both teams was compared.
- Native feature matrix: **15 configurations, 41 games,
  552,224 exactly matching turns**. Every requested feature/debug/seed check passed.
- Clang and GCC builds passed with warnings treated as errors. Both compilers
  passed the regression suite; GCC also matched Clang baseline actions on arena.
- Address and undefined-behavior sanitizers passed the mechanics/golden tests.
- Standard CLI compilation and arena smoke matches passed in both bot orderings.
- Judge clang-to-wasm compilation and metered execution passed.
- Sandbox CPU points: **1.42% increase** in aggregate, with a maximum of
  **5,200,633 points per turn** out of the toolkit's 100,000,000-point limit.
- Maximum sandbox memory was **512 KiB**, unchanged from baseline.
- No observed action, split, sonar, or deferred trail/portal-state differences.
- Production sources contain no hardcoded map machinery; includes are acyclic.
- The final lifecycle file is **61 lines**. The original source, helper and build
  configuration were verified unchanged.

Machine-readable evidence: [native matrix](tests/results/native.json),
[sandbox results](tests/results/sandbox.json), [GCC build](tests/results/gcc-build.json),
[GCC parity](tests/results/gcc-parity.json), [CLI smoke](tests/results/cli-smoke.json),
[environment](tests/results/environment.json), and [baseline hashes](tests/results/baseline-manifest.json).


## Known deviations

No intentional strategic or output-protocol differences. The new observation
cell cache and named policy phases change internal representation and execution
cost; measured costs are reported below. Future changes should rerun the golden
regressions and compare the first differing action before interpreting win rates.
