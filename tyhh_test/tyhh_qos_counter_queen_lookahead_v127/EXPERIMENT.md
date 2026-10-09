# Counter Queen: unswbc 1.2.7

# H4: Queen Body Lookahead

Parent: queen_v127. Single factor: final ordinary queen movements are reviewed
using at most 256 nodes per root and six future one-step turns. The simulated
body collides before its tail releases, visible pearls grow once, and current
other-dragon cells remain blocked. Scouting, splitting, feeding, queen IDs,
portal protection and ordinary move scores are unchanged.

Prediction: redirect a known short dead end toward a longer living route,
reducing queen self deaths before emergency splitting is necessary.
Unknown geometry, invisible cells, incomplete own-body memory, and exhausted
node budgets are uncertainty, not proven death; they retain the existing
policy. A horizon of six is not a guarantee about hidden enemies or their
future moves. A rejected protected portal also terminates the proof; the
final chosen alternative still passes the existing immediate portal guard.

Protocol: development78001, ranked17, two unchanged opponents and both sides,
68 games with old10 subset40. Compare paired against queen_v127. Keep all
negative results. No production promotion or multi-seed stability claim yet.

Inherited parent registration follows; it is not H4 evidence.

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
