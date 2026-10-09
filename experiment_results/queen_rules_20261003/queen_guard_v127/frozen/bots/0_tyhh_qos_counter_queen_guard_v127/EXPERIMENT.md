# H3: Exact Queen Survival Guard

This is an isolated child of `tyhh_qos_counter_queen_v127`, registered before
candidate matches on 2026-10-03. No normal movement scores change. A final
queen-only guard follows friend and portal review. It retains safe proposals;
otherwise it searches up to four free steps using exact current-body projection,
known visible vacant continuation and existing friend-exit protection. A split
is accepted by the guard only if legal, parent retains at least three, the
child has an escape and the parent has a known exit while child cells remain
occupied. Without a proven replacement, the original last resort is retained;
this avoids introducing a new unverified friendly collision or portal crossing.

Target evidence: parent queen_v127 portals / xxqztest / B at replay index110
splits length8 to2 at (23,4); index111 moves south into its neck at (23,5).
The focused fixture uses this body geometry on an open synthetic local map;
it does not establish the real replay position had a safe escape.

Primary metric is matched ranked17 wins against each opponent; secondary
queen survival and immediate post-split self deaths. Root owns source freeze
and match scheduling. The inherited parent registration below is historical.

Registered 2026-10-03 before new-rule candidate matches. Parent:
`tyhh_qos_relay_counter`; parent, production and opponents remain unchanged.

Integration hypothesis: fixed queen preservation and correct free-step
movement improve the queen-first scoring result against unchanged V3_portal
and xxqztest. This is a rules-adaptation package, not a single-component causal
ablation. A separate feeding version will isolate queen donation if evaluated.

Changes: symmetric queen IDs 0/1; no voluntary queen trades/donations; one
early bootstrap split only when population is one and length at least six;
routine scout expansion retained; emergency splitting retained. Scouts can
trade for the enemy queen. Friendly queen exits/body receive protection even
when the queen is short. Queen status is relayed below length six.

Queens and dragons of length at least eight require resolved portal geometry,
currently visible vacant landing, known continuation and visible space.
Complete relayed geometry permits a first crossing; no blind exception is
added to the final portal review. This can restrict resource-room access.

Projection fixes initial ceil(length/4) free steps, pre-payment eligibility,
collision before movement, pearl growth before extra tail payment and no
double counting the same pearl. Bounded free-step harvesting uses exact own
body projection and visible resources, maximum four steps. Enemy reach uses
a conservative heuristic with visible lower-bound lengths, not hidden state.

Evaluation: project .venv-battlecode toolkit 1.2.7, ranked17 maps, two opponents,
both sides, seed base 78001 with fixed per-map offsets. 68 games per version;
the historical ten-map 40-game subset is separately reported. Freeze sources,
binaries, engine and maps before matches. Primary metric: wins (draws are
non-wins); secondary: queen survival/length, longest and total. Retain
intentional no-action deaths; process failures are invalid matches.

Single-seed-per-map results are descriptive development evidence only. No
significance, held-out generalization or individual component attribution.
Copied verification files describe the parent; new tests/results live under
`experiment_results/queen_rules_20261003/`.
