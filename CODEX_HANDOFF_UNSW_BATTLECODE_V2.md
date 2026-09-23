# UNSW Battlecode — Codex Handoff Report v2

## Status

**Phase 0 / V05 baseline and instrumentation is already implemented.**

Do **not** rebuild Phase 0 unless the current source tree shows it is incomplete or broken.

The next implementation sequence is:

```text
V06  Unified Perception
V07  Derived Perception Cache
V08  Stateful Expansion + Navigation
V09  Candidate-State Scoring Engine
V10  Persistent Dragon Roles
Later Symmetry System
```

Two roadmap revisions are important:

1. **BugNav is removed from V08.**
2. **V08 must explicitly support deliberate early map expansion**, both for immediate exploration quality and to create a clean foundation for a future symmetry system.

---

# 1. Development philosophy

The bot should evolve in this order:

```text
sense once
    ↓
derive reusable local facts once
    ↓
expand deliberately + remember navigation progress
    ↓
score candidate resulting states
    ↓
assign longer-lived strategic roles
    ↓
optionally exploit symmetry later
```

Do not merge all of these changes together.

Each phase should be testable against a frozen previous version.

---

# 2. Current architecture to preserve

Expected modules include:

```text
decide.hpp
sense.hpp
memory.hpp
navigation.hpp
tactics.hpp
forage.hpp
kamikaze.hpp
opening.hpp
sonar.hpp
strategy.hpp
model.hpp
main.cpp
```

Current high-level orchestration is approximately:

```text
memory.observe()
receive_sonar()
look()

short_trade?
find_dive?
split?
choose movement?
broadcast pearl
fallback
```

Existing strengths that should be preserved:

```text
- View / StepInfo abstraction
- persistent Memory
- direct observation overriding sonar memory
- timestamped sonar reports
- bounded forage routing
- safe first-step execution
- isolated kamikaze/dive logic
- separate opening/split logic
- StrategyConfig feature flags
```

---

# 3. V06 — Unified Perception

## Objective

Replace overlapping sensing work with one architectural sensing pipeline.

This should be a **behavior-preserving refactor**.

Do not add new tactical behavior in V06.

## Target API

Create something like:

```cpp
struct Perception {
    View view;

    std::vector<Position> visible_pearls;
    std::vector<Position> visible_enemy_heads;

    int round = 0;
};
```

and:

```cpp
Perception sense_nearby(
    Controller const& ct,
    Game const& game,
    Memory& memory,
    StrategyConfig const& config
);
```

## Target flow

```text
ct.get_tiles()
    ↓
single visible-tile pass
    ├─ update Memory
    ├─ build View::snap
    ├─ collect own body segments
    ├─ collect ally heads
    ├─ collect enemy pieces
    ├─ collect visible pearls
    └─ collect broadcast candidates
```

After the scan:

```text
reconstruct own body
group enemy dragons
derive safe first steps
```

## Important implementation note

The helper API may already cache current vision, so the main guaranteed benefit is **not necessarily fewer engine sensing calls**.

The guaranteed benefits are:

```text
fewer repeated loops
fewer repeated hash operations
fewer repeated object extractions
one consistent current-turn truth source
```

## Sonar integration

Pearl broadcasting should consume:

```cpp
perception.visible_pearls
```

instead of rescanning all visible tiles.

## Acceptance criteria

```text
- no intentional strategy change
- same actions on existing regression cases
- lower or equal duplicated sensing work
- all Phase 0 tests remain green
```

---

# 4. V07 — Derived Perception Cache

## Objective

V07 means:

> **derive expensive local facts once from the newest world state, then reuse them everywhere.**

It is **not** primarily “search local memory first, then update new vision.”

Memory reconciliation belongs to Memory/V06.

V07 begins after current vision and persistent memory have been reconciled.

Target sequence:

```text
previous Memory
      +
new vision
      ↓
V06 Perception
      ↓
updated Memory + current View
      ↓
V07 derived analysis
      ├─ topology / connected regions
      └─ enemy threat / reachability
      ↓
navigation / tactics / forage reuse results
```

---

# 5. V07.1 — Connected-component cache

Current problem:

```text
candidate North -> pocket flood fill
candidate East  -> pocket flood fill
candidate South -> pocket flood fill
candidate West  -> pocket flood fill
```

If multiple candidates belong to the same visible free region, this repeats almost identical work.

Replace it with:

```text
visible free cells
      ↓
compute components once
      ↓
component metadata
      ↓
candidate lookup
```

Suggested structure:

```cpp
struct RegionInfo {
    int size = 0;
    bool touches_unknown = false;
    int min_degree = 0;
    int max_degree = 0;
};
```

Maintain:

```text
component_id[position]
regions[id]
```

Then derive:

```text
pocket size
hard trap
soft trap
escape quality
```

through component lookup.

---

# 6. V07.2 — Threat cache

Current problem:

```text
for each candidate
    for each enemy
        can enemy reach candidate?
        is it forward?
        how many escapes?
```

Instead compute tactical enemy facts once.

Possible representation:

```cpp
struct EnemyThreat {
    int enemy_id;
    Position head;
    int visible_length;

    int escapes;

    uint8_t reachable_mask;
    uint8_t forward_mask;
};
```

or a simpler aggregate:

```cpp
struct LocalThreatMap {
    std::array<int, 4> enemy_reach_count{};
    std::array<int, 4> forward_reach_count{};
};
```

Later V09 should consume these cached facts.

## V07 acceptance criteria

```text
- little or no behavior change
- pocket flood-fill count drops substantially
- repeated enemy-analysis work drops substantially
- existing tactical semantics remain intact unless fixing an explicit bug
```

---

# 7. V08 — Stateful Expansion + Navigation

## Revised objective

**Do not add BugNav.**

The transferable MIT lesson is:

```text
remember target
remember progress
detect stagnation
recover intelligently
```

UNSW dragons are not point agents. Navigation must account for:

```text
body geometry
growth
head collision
local traps
pearls
splitting
toroidal movement
sonar
possibly portals
```

So V08 becomes:

> **persistent navigation state + deliberate early expansion + stagnation recovery**

---

# 8. V08.1 — Navigation state

Add something like:

```cpp
enum class NavMode {
    Expand,
    Route,
    Retarget,
    Escape
};

enum class NavPurpose {
    Pearl,
    Explore,
    Combat,
    Escape
};

struct NavigationState {
    std::optional<Position> target;

    NavPurpose purpose = NavPurpose::Explore;
    NavMode mode = NavMode::Route;

    int best_distance = INF;
    int stagnant_rounds = 0;

    std::array<Position, 6> recent_heads{};
    int recent_count = 0;

    std::optional<Direction> temporarily_avoid;

    int sector = -1;
};
```

Track:

```text
target changed?
distance improved?
same positions repeated?
same directions repeated?
target stale?
```

---

# 9. V08.2 — Early expansion

Early dragons should not all chase the nearest frontier.

The opening should deliberately spread information collection across the map.

Target behavior:

```text
early game
    ↓
assign expansion direction/sector
    ↓
prefer unexplored regions in that sector
    ↓
avoid duplicating nearby ally exploration
    ↓
keep movement safe
    ↓
feed observations into Memory
```

This prepares the team for future symmetry inference because observations become geographically diverse.

## Important principle

Do **not** hard-code expansion around an assumed symmetry.

Build expansion around:

```text
coverage
information gain
anti-overlap
safe movement
```

A future symmetry module should consume this data rather than dictate V08 exploration.

---

# 10. V08.3 — Sector assignment

Simple initial version:

```cpp
int sector = ct.get_id() % NUM_SECTORS;
```

Possible sector sets:

```text
N / E / S / W
```

or:

```text
NW / NE / SE / SW
```

Do not over-engineer the first implementation.

The goal is to reduce:

```text
dragon A ─┐
dragon B ─┼──> same nearby frontier
dragon C ─┘
```

and encourage:

```text
        dragon A ↖

dragon B ←       → dragon C

        dragon D ↓
```

---

# 11. V08.4 — Exploration scoring

Current nearest-frontier logic can evolve into scored exploration.

Possible exploration score:

```text
+ information gain
+ sector preference
+ distance progress
- nearby ally overlap
- danger
- revisiting recently seen area
```

Memory should expose enough information for this.

Useful data:

```text
seen
observed_at
frontier
staleness
```

Do not add full symmetry yet.

---

# 12. V08.5 — Stagnation and anti-oscillation

Detect cases such as:

```text
E -> W -> E -> W
```

or:

```text
same target
no distance improvement for N rounds
```

Suggested recovery order:

```text
1. detect stagnation
2. temporarily avoid repeated first step
3. rerun bounded route search
4. if still stagnant, retarget
5. if locally trapped, enter Escape mode
6. return to Route after progress resumes
```

This replaces the earlier BugNav idea.

---

# 13. V08.6 — Local escape

Use V07 topology information.

If local movement becomes constrained, temporarily prioritize:

```text
larger connected region
more exits
less hard-trap risk
lower enemy threat
```

rather than pure target distance.

This is a local recovery primitive, not a full second pathfinding algorithm.

## V08 acceptance criteria

Measure more than final win rate:

```text
- early dragons spread farther apart
- reduced exploration overlap
- map coverage rises faster
- no large systematic loss of pearl economy
- oscillation cases fall
- stale targets are dropped
- trapped dragons recover more often
```

Useful metrics:

```text
coverage by round
unique cells observed
frontier overlap
average ally-head separation
stagnant-turn count
retarget count
```

---

# 14. V09 — Candidate-State Scoring Engine

## Correct mental model

V09 can be thought of as a **new tile-scoring machine**, but more precisely:

> **it scores the consequence of each candidate move, not just the destination tile itself.**

The same tile can be good or bad depending on:

```text
entry direction
enemy head reach
enemy facing
body shape
available exits
route objective
pearl gain
ally congestion
```

So V09 should operate on:

```text
(current state, candidate move)
      ↓
predicted immediate resulting state
      ↓
feature extraction
      ↓
hard classification/filter
      ↓
ranking
```

---

# 15. V09 data structure

Keep `StepInfo`.

Add something like:

```cpp
struct MoveEval {
    StepInfo step;

    int route_progress = 0;
    int pearl_value = 0;

    int space = 0;
    int exits = 0;

    int enemy_reach = 0;
    int enemy_forward_reach = 0;

    int ally_pressure = 0;

    bool wins_trade = false;
    bool loses_trade = false;
    bool enters_trap = false;
};
```

Do not force these exact fields if source analysis suggests better ones.

---

# 16. V09 pipeline

```text
safe candidate moves
      ↓
V07 topology features
      ↓
V07 threat features
      ↓
V08 navigation progress
      ↓
resource features
      ↓
MoveEval
      ↓
hard filters
      ↓
ranking
```

This is inspired by Version41-style candidate micro, but must be adapted to UNSW dragon mechanics.

---

# 17. V09 two-stage decision model

Do not immediately build one giant weighted sum.

Prefer:

## Stage 1 — hard filters / tactical class

Examples:

```text
illegal
suicidal
clearly losing head trade
hard trap with no tactical payoff
```

## Stage 2 — ranking

Possible ranking features:

```text
pearl value
route progress
space
escape degree
enemy pressure
forward-head threat
ally congestion
future mobility
```

This keeps behavior interpretable.

## V09 acceptance criteria

Inspect:

```text
survival rate
head-trade quality
trap-entry frequency
average local space after move
pearl collection efficiency
combat replay quality
```

Every major scoring change should have an explainable replay effect.

---

# 18. V10 — Persistent Dragon Roles

Only implement after V09 is stable.

Create longer-lived roles such as:

```cpp
enum class Role {
    Scout,
    Harvester,
    Hunter,
    Core,
    ScoreReserve,
    Escape
};
```

Possible state:

```cpp
struct DragonState {
    Role role;

    int role_since = 0;
    int no_enemy_rounds = 0;

    NavigationState nav;
};
```

Role assignment may consider:

```text
length
round
unit count
enemy density
resource density
map area
late-game state
```

Use hysteresis.

Do not allow roles to flip every turn.

---

# 19. Future symmetry system

Symmetry should come **after** V08 early-expansion infrastructure exists.

The symmetry system should consume observations from Memory.

Conceptual flow:

```text
observed cell / edge / landmark
        ↓
candidate transformed coordinate
        ↓
compare with known observation
        ↓
support or eliminate symmetry hypothesis
```

Potential transformations should only be introduced if actual UNSW map generation supports them.

Possible candidates to investigate later:

```text
horizontal reflection
vertical reflection
180-degree rotation
other game-specific transforms
```

The important preparation from V08 is:

```text
wide early coverage
clean coordinate memory
observation timestamps
sectorized exploration
```

Future symmetry can then generate:

```text
predicted exploration targets
predicted map structure
possible strategic targets
```

without being coupled to the exploration implementation itself.

---

# 20. Team ownership

Assign ownership by architecture layer.

## Teammate A — Perception / Memory

Own:

```text
V06
V07 topology
V07 threat preprocessing
future symmetry observation support
```

## Teammate B — Navigation / Expansion

Own:

```text
V08 navigation state
sector expansion
stagnation
retargeting
anti-oscillation
escape
```

## Teammate C — Tactics / MoveEval

Own:

```text
V09 candidate features
hard filters
candidate ranking
combat analysis
```

## Teammate D — Infrastructure / Strategy

Own:

```text
A/B harness
feature flags
metrics
replay analysis
V10 role state
```

Avoid ownership such as “hunting”, “survival”, and “pearl collection”, because those cut across the same modules and create merge conflicts.

---

# 21. StrategyConfig migration

Suggested temporary flags:

```cpp
struct StrategyConfig {
    bool unified_perception = false;

    bool component_cache = false;
    bool threat_cache = false;

    bool expansion = false;
    bool sector_exploration = false;
    bool nav_memory = false;
    bool anti_oscillation = false;
    bool retargeting = false;
    bool escape_mode = false;

    bool candidate_micro = false;

    bool role_system = false;

    // existing settings...
};
```

Flip one logical subsystem at a time during development.

Remove obsolete flags after systems become stable.

---

# 22. Testing discipline

For every phase:

```text
1. strict compile
2. sanitizers
3. unit/behavior tests
4. dedicated scenario tests
5. previous frozen version vs new version
6. swap sides
7. replay review
8. explain observed behavior change
```

Do not merge only because:

```text
win rate = 56%
```

Prefer explanations such as:

```text
V08 increases map coverage by round 50 and reduces frontier overlap.
```

or:

```text
V09 avoids forward enemy-head threat that the old scalar score missed.
```

---

# 23. Codex instructions

Read this report first, then inspect the current local source tree.

Treat source code as ground truth.

**Phase 0 / V05 is already implemented.**

Do not rebuild it unless the code proves it incomplete.

## First requested task

Audit the current codebase and verify:

```text
1. current V05 instrumentation
2. exact act() call sequence
3. current sensing passes
4. current Memory update path
5. current pocket implementation
6. current tactical enemy recomputation
7. current exploration/frontier logic
8. current persistent state across turns
9. current StrategyConfig switches
10. current test harness
```

Then produce an exact V06 implementation plan.

Do not implement V07+ until V06 is merged and validated.

---

# 24. Ready-to-paste Codex prompt

```text
Read CODEX_HANDOFF_UNSW_BATTLECODE_V2.md first.

Then inspect every source file in the current UNSW Battlecode bot directory.

Context:
- Phase 0 / V05 baseline + instrumentation is already implemented.
- Do not rebuild it unless the code proves it incomplete.
- The roadmap is V06 -> V07 -> V08 -> V09 -> V10.
- BugNav has been removed from the roadmap.
- V08 must support deliberate early expansion, sectorized exploration, stagnation detection, retargeting, anti-oscillation and local escape.
- V09 is a candidate-state scoring engine, not merely a destination-tile weighted score.
- A future symmetry system should consume the wide map coverage produced by V08, but symmetry is NOT part of V08.

Rules:
1. Source code is ground truth.
2. Do not rewrite the whole bot.
3. Implement one phase at a time.
4. Preserve behavior in pure-refactor phases.
5. Keep perception/memory, navigation, tactics/micro, combat, opening/splitting, strategy and orchestration separated.
6. Use feature flags where useful for ablation.
7. Keep prior versions frozen for comparison.
8. Add tests for every new primitive.
9. Before editing, state:
   - files that will change;
   - behavior expected to remain identical;
   - new invariant introduced;
   - exact tests that will validate it.
10. Do not port MIT Battlecode mechanics literally. Transfer architecture and engineering patterns only.

First task:
Audit the current implementation against this report and produce a concrete V06 plan.
Do not modify code yet.
```

---

# 25. Target architecture

```text
                       Controller
                           |
                           v
                    Perception
                           |
          +----------------+----------------+
          |                |                |
          v                v                v
       Memory          Topology          Threats
          |                |                |
          +----------------+----------------+
                           |
                           v
                   NavigationState
                           |
                    early expansion
                           |
                     target choice
                           |
                           v
                    candidate moves
                           |
                           v
                       MoveEval
                           |
                           v
                  tactical selection
                           |
                           v
                       execute
                           |
                           v
                    DragonState
                     / future roles
```

Future symmetry plugs into:

```text
Memory observations
      ↓
symmetry hypothesis manager
      ↓
predicted exploration/strategic targets
```

Core principles:

```text
sense once
derive once
reuse everywhere
expand deliberately
remember navigation progress
score resulting states
separate tactics from strategy
test every upgrade independently
```
