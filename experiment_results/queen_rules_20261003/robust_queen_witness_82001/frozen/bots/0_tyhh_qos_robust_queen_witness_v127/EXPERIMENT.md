# R20 Local Queen First-Step Head-Hit Witness

Registered 2026-10-06 before source changes or competitive evaluation.
Parent is frozen H14 `tyhh_qos_counter_queen_guard_consolidate_v127`,
main SHA256 `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.

The development evidence is dilemma seed84006 A queen0 round199: ordinary
`MOVE N` retains length8 but enemy14 can next move `W` into its head;
`MOVE NN` uses the same initial two free steps, retains length8 and turns
the same enemy action into a body collision. Official unswbc1.2.7 action-tape
fixtures prove that local counterfactual, not a general survival gain.
See `experiment_results/queen_rules_20261003/QUEEN_IMMEDIATE_WITNESS.md`.

Single mechanism: after H14's final queen-survival review, inspect a living,
known, ordinary movement proposal. Reconstruct the current own and enemy
bodies only from complete visible ordinary facing chains. A witness is one
complete visible enemy's legal first ordinary free step into the projected
queen head, without colliding with its own body. When at least one witness
exists, search at most four of the queen's initial free steps for a known,
living, same-final-length alternative with an ordinary continuation and no
hard or soft friendly-protection risk. The alternative must strictly reduce
the witness count and must not add a witness for any other complete visible
enemy. Prefer the longest common prefix with H14's proposal, then the shortest
replacement. Search caps are explicit and exhausted searches preserve H14.

The input cannot prove that a visible enemy is the globally next live ID:
hidden enemies or friends may act between queen and the tested enemy. The
witness holds visible bodies static after the queen action, and is only a
local first-step possibility, never a global safety proof. Hidden/partial
enemies, unknown edges or cells, portals, longer enemy routes, and intervening
turn changes remain unknown. Zero detected witnesses is not labelled safe.
The policy does not override a split, donation, unknown/portal proposal,
partial own body, or a proposal already fatal by current geometry.

Completeness is stricter than the evidence script's straight-body inference:
the candidate follows visible predecessor parts whose facing reaches the
preceding segment, allowing ordinary bends. The own chain must match LENGTH
and H14's projection order. An enemy's tail must have all possible nonkelp
predecessor cells visible across ordinary edges; checking only its opposite
facing cell is insufficient for a body that may bend. Thus enemy14 in the
84006 fixture is complete, but enemy16 at the view boundary remains unknown.
See `experiment_results/queen_rules_20261003/QUEEN_BODY_INFERENCE_CORRECTION.md`;
the older evidence outputs are preserved without using their body inference
as a general candidate completeness test.

No exploration, attack, growth threshold, donor mechanism, original guard,
opponent, map, engine, production or confirmation seed is changed. Root owns
competitive evaluation; this implementation task runs focused fixtures only.
