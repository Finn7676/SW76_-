# Counter Queen Feeding: unswbc 1.2.7

Registered 2026-10-03 before new-rule candidate matches. Parent:
`tyhh_qos_counter_queen_v127`; parent, production and opponents remain unchanged.

Hypothesis: verified local donations to the fixed queen increase queen-first
score beyond the matched queen-rules parent. The only strategy difference is
the queen-directed feeding module; the portal final-review hard-risk fix is
synchronized with the parent. All ordinary parent actions remain fallback.

The new module never donates the queen. From round 280, a donor of length at
most 24 may donate only with population at least four, eight locally observed
quiet rounds, a currently visible confirmed friendly queen head, and a known
visible one- or two-step collection route after the donor disappears. There
must be a distinct continuation and at least `distance + donor_length + 6`
rounds left. The queen may be shorter than or equal to the donor; her fixed ID
prevents reciprocal donation. A visible competing head that reaches the first
guaranteed pearl no later than the queen blocks donation. This is a local
safety test, not a guarantee against unseen enemies or a reservation protocol.

Donation emits an empty MOVE deliberately. The engine records no valid action
and death, leaving pearls from the donor's old head. Exact projection marks the
donor dead, so no status falsely advertises a surviving donor. Queen movement
uses the unchanged pearl scorer, not hidden access to the donor's plan.

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
Copied verification files describe the parent. New module guard and integration
assertions are `verification/queen_feed_tests.cpp.txt`; evaluation artifacts
live under `experiment_results/queen_rules_20261003/`.
