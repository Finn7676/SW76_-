# R20 Ready

Frozen 2026-10-06 after native production O2, focused source fixtures,
inherited H14 fixtures and official unswbc1.2.7 action-tape validation.
No competitive matches were run. Source edits stop after this record;
root owns source freeze, independent review and competitive evaluation.

The production main differs from H14 only by `queen_witness.hpp` include
and one final `reviewQueenFirstStepWitness(ct)` call after the unchanged
queen-survival review. The helper, bot.toml, feed and consolidation modules
remain byte-identical to H14.

## Validation

Production builds passed without warnings:

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_queen_witness_v127/main.cpp -o /tmp/r20_queen_witness_bot
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp -o /tmp/r20_h14_control_bot
```

Focused R20 source-bound geometry and H14-control tests passed for both
queen IDs, including the simplified known-open 84006 geometry N to NN,
bent own/enemy predecessor chains, hidden side/tail completeness, ambiguous,
disconnected and cyclic bodies, portal/unknown preservation, fatal originals,
split/donation/nonqueen preservation, no-added-enemy masks even when total
would fall2 to1, hard/soft friendly gates and same length/free-step limits:

```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_queen_witness_v127/verification/queen_witness_tests.cpp.txt -o /tmp/r20_witness_tests
/tmp/r20_witness_tests
g++-10 -std=c++20 -O2 -DH14_CONTROL -x c++ tyhh_test/tyhh_qos_robust_queen_witness_v127/verification/queen_witness_tests.cpp.txt -o /tmp/r20_witness_h14_tests
/tmp/r20_witness_h14_tests
```

Inherited `candidate_tests.cpp.txt`, `queen_guard_tests.cpp.txt`, and
`consolidate_tests.cpp.txt` were each compiled with the same O2 test command
and passed. Include-based test harnesses rename `main`; only those test
compiles emit the inherited no-return-in-renamed-main warning.

Real official round199 raw input SHA256
`6f2229283c3a5b5c06623d6106cff57ff3524420ba0cdcfdc51134478391d416`
is separately parsed by fresh native production bots through their full
pipelines: H14 emits `MOVE N`, R20 emits `MOVE NN` with confirmed count1
to0, one complete visible enemy and6 search nodes. Prior-round bot memory
is deliberately absent in this input fixture. Official1.2.7 action-tape
recreation matches19705 canonical physical/SONAR prefix events and the real
raw input. Applying the emitted native MOVE while holding recorded SONAR
constant proves H14 queen0/enemy14 both die `H`, whereas R20 queen0 retains
length8 and only enemy14 dies `O`. Later ENDTURN deaths are fixture cleanup.

```bash
.venv-battlecode/bin/python tyhh_test/tyhh_qos_robust_queen_witness_v127/verification/queen_witness_engine.py --candidate /tmp/r20_queen_witness_bot --control /tmp/r20_h14_control_bot --output-prefix /tmp/r20_witness_official_new
```

Stored proof: `verification/official_witness.json`,
`verification/official_witness_H14.replay`, and
`verification/official_witness_R20.replay`. These are fixtures, not matches.
The older evidence body's opposite-facing inference is not reused; see
`experiment_results/queen_rules_20261003/QUEEN_BODY_INFERENCE_CORRECTION.md`.

## Scope

The candidate proves local first ordinary-step head-hit possibilities from
complete visible enemies, not globally next actor or global safety. Partial,
hidden, boundary, portal, unknown, longer-route and intervening-turn threats
remain unknown. A reduction1 to0 is not a safety certificate or win-rate
claim. Confirmations, production, H14, maps and opponents are untouched.

## SHA256

- main: `c7682278e8f70609b8edf44423273ec65df4c4729ece4ba1d971eae3308bdcfb`
- queen_witness.hpp: `c760dc62345dda734182787dde599657010364a677c651e72dc6f7ce27ae58ed`
- helper: `6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`
- bot.toml: `d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`
- queen_feed.hpp: `0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`
- consolidate.hpp: `bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`
- native R20 binary: `356adc4a6a0eea16fad96676bf99d3ce097fcfd3f1a2cabfae7301f6028feae8`
- native H14 control binary: `98a8c546c653a6a9ae1aabdae284a22ef460ac3bc525bf842ed31223f3232c9f`
