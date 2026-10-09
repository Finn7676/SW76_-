# R21 First-Screen Parser Semantic Audit

Independent read-only audit by `/root/robustness_next/r21_parser_semantics`.
This audit inspects replay schema, topology, event reconstruction and source
binding. It does not analyze experimental outcomes, select a candidate, or
make a performance/generalization claim. No game was launched. No frozen
candidate, opponent, engine, map, suite or mechanism-parser file was edited.

## Bound Artifacts

- Subject: `r21_firstscreen_mechanism.py`, SHA256 `b0c28698c4752c0991b9463a239d793f028b6080913f4abf3cca4e11399112dd`.
- Independent check: `r21_firstscreen_geometry_check.py`, SHA256 `26a0031491760fe17e7fe08a9d7653ee31045ff5db16e08b97c8ea06e91ee3bd`.
- Exclusive output: `r21_firstscreen_geometry_check.json`, SHA256 `df69f31e1dc672fba6b9cecaa6528929b02d8d1cbeb84fe0fec474f5bff53f69`.
- Official viewer: installed native `unswbc 1.2.7` `replay-viewer.vsix`, SHA256 `3d822ee45f4b570d6111bbd294897f196bb298ec9f538efdce75a8f67e22344d`.
- Official engine: SHA256 `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`.

The independent script reads only frozen map paths/hashes from H14 `run.json`,
the subject parser as a library, the bundled viewer, and rounds 0 through 2 of
one existing H14 control replay. It never calls the mechanism parser's `main`,
reads result CSV/audit outcomes, or reads the fixture's terminal result.

## Schema And Initialization

The bundled viewer's generated schema and decoder establish these offsets:

| Kind | Event | Relevant Fields |
| ---: | --- | --- |
| 2 | Pearl countdown | Signed Int32 countdown at data offset 0; tile Point pointer 0 |
| 3 | Tile change | hasPearl bit 0; tile Point pointer 0 |
| 4 | Dragon action | Signed Int32 dragon ID offset 0; optional action pointer 0 |
| 5 | Engine log | Signed Int32 ID offset 0; text pointer 0 |
| 6 | Dragon log | Signed Int32 ID offset 0; text pointer 0 |
| 9 | Dragon update | Signed Int32 ID offset 0; facing Uint16 offset 4; head/tail pointers 0/1 |
| 10 | Dragon split | Parent/child Int32 offsets 0/4; team/facing Uint16 offsets 8/10; parent/child body lists pointers 0/1 |
| 11 | Dragon death | Signed Int32 ID offset 0; reason Uint16 offset 4 |

Kind 2 is not an edge update. The schema has no topology-change event;
the viewer's map event handler changes only pearl tiles. Replay-root map
topology is therefore the correct source of edge types and portal pairing.

The viewer constructs `roundAt(0)` from `map.initialDragons` and stores events
before the first `roundStart` separately as `initEvents`. It does not apply
those initial dragon-coordinate records as movement. The subject parser
correctly validates their head/tail against the map without inserting another
head. In the existing slithery_fight 82009 B control replay, all 14 initial
coordinate records match the map, and 23 subsequent splits before round 3
match both exact reconstructed prebody and the same-turn split action.

A kind 4 event with a null action pointer establishes no recorded action, not
the reason for a later death. The subject's internal `kind=None` distinction
is correct; its display label `NO_VALID_ACTION` should be understood only as
a placeholder. `NO_ACTION_RECORDED` would avoid confusion with reason 4 of a
kind 11 death. The audit does not relabel any observed death.

## Exhaustive Geometry Check

All 17 frozen maps have a complete `2 * width * height` canonical edge table,
no conflicting toroidal seam records, and exactly two same-orientation
endpoints for every portal ID. The independent implementation uses the
official viewer's N/W endpoint representation, rather than reproducing the
subject parser's E/S endpoint representation.

For every cell and direction, it checks ordinary open-edge landing or the
paired portal landing against `pair_geometry`. All 78,324 topology-reachable
head/neck pairs and 78,324 passable directional transitions agree, including
portal IDs and ordinary/portal classifications. The check counts possible
connections, not dragons that occupied them in competitive games. Raw map
hashes, dimensions and per-map coverage are in the exclusive JSON output.

For a paired horizontal endpoint `(x, y)`, south lands at `(x, y)` and north
at `(x, y - 1)`. For a paired vertical endpoint `(x, y)`, east lands at
`(x, y)` and west at `(x - 1, y)`, with coordinates wrapped. A matching open
edge establishes an ordinary topological connection. A matching portal
transition establishes a possible portal connection, not the historical
movement path if multiple matching connections exist. Geographic adjacency
alone is insufficient to determine whether an edge is open or a portal.

## Split, Timing And Source Binding

The split's official prebody is the copied parent body followed by the
reversed copied child body. The subject asserts equality with the reconstructed
prebody and same-turn action child size before mutating stored bodies. This
avoids live-array aliasing and the former initial-head duplication issue.

The next turn, next action, next successful update and actual death are stored
separately. A death during another actor's turn may precede the dragon's next
action. The active-turn ID and actual kind 11 reason must remain part of the
evidence. An `invalid_action` label by itself does not establish portal death.

The frozen source binding was independently rechecked: removing the exact
five-line ordinary rear-cascade block from frozen R21 `main.cpp` yields frozen
H14 `main.cpp` byte-for-byte. The shared `helper.hpp`, `queen_feed.hpp`,
`consolidate.hpp` and `bot.toml` files are also byte-identical. Bound main hashes:

- R21: `eb364e6e9ccf138a4b3b12d071e356a2ecee6300a4b0209675fe6888382f5573`.
- H14: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.

Under that single-factor source binding, an identical physical/sonar prefix,
same actor/prebody/population, H14's actual SPLIT 2 and R21's actual
SPLIT LEN-2 give a defensible first observed allocation effect. This is a
game-level lower bound, not a complete ordinary-branch activation count.
Common action review can replace a proposed split; unchanged committed action
history cannot prove that no proposal was considered. Later raw LEN-2 splits
can originate in emergency branches. Numerical child IDs after divergence
must not be used as matched causal identities across games.

## Interpretation Caveats

- `next_round_or_earlier_death_reasons` counts allocation-linked death witnesses. A persistent parent can split more than once before one eventual death, so several allocations may reference the same death event. It is not necessarily a count of distinct deaths; deduplicate by replay and death event if a distinct-death metric is needed.
- The subject log attachments omit original `data.number(0)` IDs for kinds 5 and 6. This does not change reconstructed bodies or action comparison, but diagnostic logs are attributed only by active-turn order. Preserve original IDs in a future parser/output version before claiming ID-specific log attribution.
- Current radius-3 visibility is offline geometric context, not a reconstruction of private trail, sonar memory or full bot inputs.
- Round 20/50/100 snapshots precede those rounds. Early-ended games must not be padded.
- Geometry and actual death timing/reason are separate observations. Neither schema correctness nor the topology check establishes survival, win causality or cross-seed robustness.

## Reproduction

The check completed successfully with:

```bash
.venv-battlecode/bin/python -B experiment_results/xxqz_robust_20261004/r21_firstscreen_geometry_check.py
```

The default output already exists and is intentionally preserved. For a new
read-only rerun, supply a previously nonexistent `--output` path inside the
workspace or `/tmp`. The script refuses to overwrite evidence.
