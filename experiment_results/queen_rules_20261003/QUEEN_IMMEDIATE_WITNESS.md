# Queen Immediate Head-Hit Evidence

## Conclusion

One executable counterfactual exists in the 102 audited H14/xxqztest development games. On dilemma seed84006 A, queen0 round199 chose `MOVE N` and died during the immediately following id14 turn. `MOVE NN` is a fully visible, legal, two-free-step alternative that retains queen length8, has a known ordinary continuation, and changes the same actual enemy `MOVE W` from queen head collision to enemy body collision. Official unswbc1.2.7 validates both arms.

This is immediate-turn mechanism evidence, not a global survival or win-rate claim. No bot source changed, no competitive game ran, and no confirmation/holdout seed was inspected.

## Provenance

- Replay: `robust_h14_84001/0045_xxqztest_dilemma_seed84006_A.replay`.
- Replay SHA256: `b762d013933f195a0b7e8003f71ea5d62f8b71434f64cebc0157eeb98a575832`.
- Frozen H14 main SHA256: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
- Official engine SHA256: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`.
- Final evidence: `queen_immediate_witness_scan2.json`, `queen_immediate_engine_scan2.json`, and its two `_N.replay`/`_NN.replay` fixtures. Scan1 is retained as intermediate evidence.

The action-tape fixture replays all recorded movement/split actions and SONAR beams from the original map and seed. Backward SONAR requests are reconstructed from outgoing-tail origin and the current facing. Both regenerated arms match the original 19705 physical/SONAR canonical events before the queen action. Both have byte-identical official queen round199 input, SHA256 `6f2229283c3a5b5c06623d6106cff57ff3524420ba0cdcfdc51134478391d416`. Spawn and full round199 input are stored in the JSON. Fixture-only ENDTURN cleanup begins strictly after id14; cleanup deaths are explicitly separate from the comparison.

## Exact State And Action

Original queen action event19726; id14 next-turn event19732, actual `MOVE W` event19733; queen head death event19734. No lower/later dragon acts between the queen and id14. The current input has `DIR W`, `LENGTH 8`, `UNIT_COUNT 10`, zero messages, no visible pearls, and vision x21..27/y0..6.

Queen body, head first:

`[(24,3),(25,3),(26,3),(26,4),(25,4),(24,4),(24,5),(24,6)]`

Enemy14 body is `[(25,2),(25,1)]`. Both parts and its ordinary rear cell `(25,0)` are visible. Part facing proves the shown tail is the actual tail, independently of hidden replay knowledge. Enemy16 is also input-proven length2, `[(27,3),(27,4)]`, with ordinary rear cell `(27,5)` visible. Enemy38 has two visible body parts but no visible head and remains unknown.

| Queen Action | Free/Paid Timing | Final Head | Final Length | Enemy14 `W` Outcome |
|---|---|---|---|---|
| Actual `N` | First of2 free steps | `(24,2)` | 8 | Both queen0 and enemy14 die `H` |
| Alternative `NN` | Both steps free | `(24,1)` | 8 | Enemy14 dies `O`; queen0 remains alive |

The alternative final queen body is `[(24,1),(24,2),(24,3),(25,3),(26,3),(26,4),(25,4),(24,4)]`. A visible ordinary `N` continuation to `(24,0)` exists; it is not executed in the fixture. Collision is checked before tail removal. Enemy14 has one free step from initial length2 and no pearl at any ordinary living first-step destination. Its second paid step cannot be funded; a pearl only at the second destination would not rescue payment.

## Witness Counts And Unknowns

The known immediate attacker changes from one confirmed attack witness (`W`) to zero confirmed witnesses within four steps. Across the two input-proven visible enemy heads, confirmed witness count also changes1 to0. Enemy16 retains an unknown branch because its east destination leaves current vision; it is not classified as safe. Its later-turn result also cannot be inferred while holding all intervening bodies static. Hidden/partial enemies, nonordinary portal movement, and paths beyond four remain unknown.

The strict development scan found44 H14 queen head deaths. Eligibility exclusions were18 partial own bodies,10 partial/portal attacker bodies,13 intervening turns, and2 own/friendly-action deaths. Only one snapshot met the strict complete-visible-immediate-attacker criteria, and that snapshot was positive. These exclusions prevent estimating general prevalence or expected win-rate gains from this result.

## Computation And Candidate Boundary

The scan took58.59s including packed replay parsing. The selected-snapshot search examined8 known queen candidates and64 immediate-attacker expansion nodes; Python search took0.624ms. The extra displayed input-proven two-enemy comparisons used60 expansion nodes and0.286ms including body inference. Each official action-tape arm took approximately0.7s and made no native bot call. These wall-clock measurements are not judge-point measurements.

A narrow next candidate is technically justified: only if the current queen action has a confirmed complete-visible immediate-next-enemy head-hit witness, consider existing complete-body, known, living, continuation-proven, friendly-safe alternatives, preferring same-length free-step actions with fewer confirmed witnesses. Unknown must remain an independent uncertainty, not zero threat. Search requires an explicit small node/candidate cap; depth4 naive enumeration can reach340 nodes per enemy/candidate and must not be applied indiscriminately to every queen turn. Scorer behavior for ordinary unknown cases should remain unchanged. Implementation and competitive evaluation were outside this task.

## Reproduction

```bash
python3 experiment_results/queen_rules_20261003/queen_immediate_witness_tests.py
python3 experiment_results/queen_rules_20261003/queen_immediate_witness.py --output-prefix /tmp/queen_immediate_scan_new
.venv-battlecode/bin/python experiment_results/queen_rules_20261003/queen_immediate_engine.py --output-prefix /tmp/queen_immediate_engine_new
```

Outputs are exclusive: existing evidence is never overwritten.
