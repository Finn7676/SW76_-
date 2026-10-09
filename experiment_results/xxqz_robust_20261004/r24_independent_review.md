# R24 Independent Implementation Review

Reviewed 2026-10-08. Candidate:
`tyhh_test/tyhh_qos_robust_room_bound_v127`; actual H14 control:
`tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127`.
No strategy source was edited, no competitive game was started, and no
reserved-seed result was read. Initial evidence files remain preserved.

## Findings

No blocking implementation issue was found for the registered first
68-game development screen. The following limits are material to its
interpretation:

- `main.cpp:361` raises only the ordinary-component area cap from 24 to
  49. It does not require the component to fit current vision, prove safe
  survival, or establish exclusive ownership. A long or irregular known
  component of area 49 can qualify. Unknown edges, a second canonical
  portal boundary, missing finite resource evidence, and area 50 still
  reject. The implementation matches the registered area-cap hypothesis,
  not a new complete-current-view or safe-portal policy.
- `main.cpp:347` still accepts any remembered finite `pearlDue`, including
  future times; `receive` and `observe` retain H14's original update
  semantics. More accepted rooms can issue more occupancy claims at
  `main.cpp:1556` and discourage incoming backup scouts via the inherited
  claimed-room policy. This can retain resources or reduce useful
  redeployment; a local proof result alone does not distinguish them.
- The 17-map geometry diagnosis admits only two additional configured
  rooms, both 25-cell rooms on `default`. It admits no new room on
  `portals` or the other 15 maps. Cap 25 would cover the same static
  configured room set; there is no configured qualifying room of area
  26..49 in that pool. Corpse-resource evidence at runtime need not match
  the static configured-resource filter. This is a narrow default-room
  coverage hypothesis, not evidence of broad-map superiority.
- `verification/room_native.py:26` preserves default topology but replaces
  its original respawn ecology with chosen mirror-symmetric fixed
  intervals, relocates subject 5, and in r39 also relocates dragon 7.
  These are fresh round-zero controls, not exact historical r55/r39
  trajectories or reconstructed persistent memory. Other dragons are
  scripted; the queens die in round zero, and subject 5 sees unit count 3
  rather than the historical 8/6. All survivors deliberately receive
  `ENDTURN` at round one. Those deaths are not competitive outcomes.
- Floodfill membership uses `std::find` over at most 49 cells. The size
  guard is sound, but the increased quadratic work and two proof calls
  per turn have not been measured against the sandbox budget. Successful
  native builds and one-turn controls do not establish budget compliance.

## Source And Bound Audit

The production diff against actual H14 contains exactly two replacements:
the comment at line 338 and literal 24 to 49 at line 361. Helper,
`queen_feed.hpp`, `consolidate.hpp`, and `bot.toml` are byte-identical.
There is no new donor/split policy, ordinary scorer change, timer, memory,
claim wire, offline-map access, or pipeline reordering.

The guard checks the existing cell count before pushing an unseen
neighbor. Thus a completely traversed component with exactly 49 cells is
admitted and an attempted 50th cell returns -1. Vector membership prevents
duplicate expansion. Traversal follows only ordinary open edges and stops
at portals. Boundary identity is `canon(c,d)`, not just portal PID; two
different boundaries sharing a PID reject, whereas the two incidences of
one internal canonical portal edge count as one boundary. The same proof
is used for preaction occupancy at line 1520 and eligible postaction claims
at line 1556. These call sites and their gates are unchanged.

## Independent Verification

All completed checks below exited 0 against the frozen hashes:

- Fresh C++20 `g++-10 -O2` production builds for R24 and actual H14 are
  warning-free and reproduce the native fixture's recorded binary hashes.
- The source-bound cap suite passes for R24 and for actual H14 supplied
  through `BOT_SOURCE` with bound 24. It covers exact 24/25/49/50 cells,
  unknown terrain, missing/negative/infinite/future resource timers,
  invalid start, no portal boundary, two distinct boundaries with the same
  or different PID, and both sides of one internal canonical portal edge.
- Both source-bound observable-state suites pass all four r55/r39 by
  current-plus-entry/self-observed-union cases. Masking remembered edges
  outside current vision preserves the proof result in these particular
  fixtures. R24 r55 selects S and locally gains one; R24 r39 still selects
  N and leaves. H14 selects N and proves no room in both. These controls
  isolate the area cap but do not reconstruct native historical memory.
- Independently rebuilt genuine-input probes match both recorded probe
  binary hashes and pass all four saved native inputs. Production
  `receive/observe` yield R24 PID 0 versus H14 -1 with 49 visible tiles,
  zero sonar messages, and subject 5 at `(4,4)`, length 2.
- Independently rebuilt production binaries replay all four saved input
  streams with `ENDGAME` appended after the first turn. Each exits cleanly
  and reproduces the recorded action/sonar/log/protocol output exactly,
  plus the expected `ENDTURN` delimiter stripped by `Bot.ask`. A bare
  one-turn input reaches expected EOF on the next update; it is not a
  complete protocol stream.
- The three inherited source suites are byte-identical to H14 and pass
  against R24: action projection/payment/queen identity/portal checks,
  seven queen-guard assertions, and eight consolidation checks. Included
  renamed-main harnesses alone emit the inherited no-return warning; no
  harness calls that renamed main.
- Read-only artifact verification passes all 31 bindings: all four replay,
  map, input and output files, embedded map and init-plus-input contents,
  and seven top-level source/map/toolkit/engine bindings. Actual
  `.venv-battlecode` is unswbc 1.2.7 with the recorded engine hash. The
  system `python3` instead exposes 1.1.0 and must not run this fixture.

Recorded native physical evidence is H14 N to `(24,7)` with next-turn
length 2 for both layouts; R24 r55 S to `(4,5)`, pearl removal and next-turn
length 3; R24 r39 remains N to `(24,7)`, next-turn length 2. Subject 5 has
no round-zero death in all four records. The r39 unchanged action is an
explicit negative control: accepting a room does not prohibit recrossing.
The official engine fixture itself was not rerun during this review.

## Verdict And Next Gate

R24 is implementation-ready only for its registered development screen.
This review supplies no competitive gain, cross-seed robustness, long-body
portal safety, improved concentration, or ranking evidence. Screen results
should remain paired with actual H14 and separate V3/xxqz, A/B seats,
default-room effects, claim activation, pickup/retained length and paired
win/loss transitions. Failure or regressions must remain visible; neither
four successful controls nor more proof activations justify promotion or
opening reserved confirmation seeds.

## Bound Hashes

- R24 main: `cea6542e3c5d8b462730456afa0289503f06e7d507098a5a39b38af9cf005857`
- H14 main: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`
- R24 production: `441839d479354c93b217407515bd599eca24dce3aa8e163e7ef147d9d8a864be`
- H14 production: `98a8c546c653a6a9ae1aabdae284a22ef460ac3bc525bf842ed31223f3232c9f`
- Native probe source: `8353de201cfa3719b7a60877f2180ea424d3deecbfec37260ee83de9f08a8fc3`
- Native script: `868286d723815aaa686345132f4f36529d8e4bb3a7d0d53ff20c5c64fe46d32f`
- Cap tests: `8d38b59b761bb61d5ae115d15c24cdc28c4fcb031e0b3c6991cca4f936550d58`
- Observable tests: `25451d2dbc4145d7a24ba43b9f2443c214a834a57a95f06a5aba868549607f12`
- Self-observed header: `0d6e84e394747c7c0b8edf196cc2dcf59a71df812ab4497ffecdf5ca42f3530e`
- Official 1.2.7 engine: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`

The analysis and statistical-testing discipline retained negative controls,
source binding, and observation/claim separation. No p-value or pooled
competitive estimate was computed from these constructed cases.
