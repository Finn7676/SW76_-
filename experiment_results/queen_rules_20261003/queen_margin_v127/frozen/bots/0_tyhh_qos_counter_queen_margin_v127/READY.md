# H7 Ready

Stopped editing H7 after successful focused native tests and standalone build.
No H7 matches have been launched. Root owns full-tree freezing and evaluation.

Main SHA-256:
`1f173452ada82bc9c1e99e6fbc519b0bad32c00d939dd7c836f0eca1f25a3695`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
Test SHA-256:
`07566d9599735f822978c0a451e93ef49c9d8b9fb3a1ef3f852e7572ce1818b8`.

Verification:
```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_counter_queen_margin_v127/verification/queen_margin_tests.cpp.txt -o /tmp/queen_margin_tests
/tmp/queen_margin_tests
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_counter_queen_margin_v127/main.cpp -o /tmp/queen_margin_bot
```
Test output: `queen_margin: graded distance, kelp, unknown, occupancy and sprint budget passed`.
Standalone bot compile is clean. Included parent main renamed in the fixture
has the inherited no-return warning. No win-rate or sandbox-compliance claim.

Diff against parent adds queenEnemyMarginPenalty and replaces exactly one
queen score term; no other weights or action calls were changed.
