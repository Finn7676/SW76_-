# R10 Ready

Parent is frozen R4 from `robust_sprint4_82001`, main SHA-256
`455fbc4744a2634a9c529a61f6b4ee582d7d6da0ceb85d2854b81aba39b60675`.
Only `sprintRamAction` target value changed: a visible fixed enemy queen
does not require a size lead and outranks ordinary head targets. Shortest
examined queen route wins. Ordinary target value/fallback and the function's
caller position, filters, traversal, budget and exact prefix projection are
unchanged. The remaining four production dependencies are byte-identical.
No matches were launched. Source edits stop after this record.

Native build passed without warnings:

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_queen_hunt_v127/main.cpp -o /tmp/robust_queen_hunt_bot
```

Focused test build and run passed:

```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_queen_hunt_v127/verification/queen_hunt_tests.cpp.txt -o /tmp/robust_queen_hunt_tests
/tmp/robust_queen_hunt_tests
```

Output: `queen_hunt: short-queen reach, exact legality, priority and 200 frozen R4 ordinary controls passed`.
The R4 control function is a renamed byte-identical copy of its frozen
original, and 200 ordinary-enemy fixtures compare both implementations.

The inherited unchanged R4 sprint suite also passed:

```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_queen_hunt_v127/verification/sprint4_tests.cpp.txt -o /tmp/robust_queen_hunt_sprint4_tests
/tmp/robust_queen_hunt_sprint4_tests
```

Output: `sprint4: reach, paid/free rules, exact body and eligibility assertions passed`.
Both include-main test harnesses have the inherited renamed-main no-return
warning; production main has no such warning.

SHA-256:

| File | Digest |
|---|---|
| main.cpp | de27f3b2abb1440d75395c9676eb4d80967947bd7adb7e66f9570005f24ca8ba |
| helper.hpp | 6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc |
| bot.toml | d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5 |
| queen_feed.hpp | 0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24 |
| consolidate.hpp | bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf |
| verification/queen_hunt_tests.cpp.txt | 431522e135dde8ef2900b743272b60cf5e04e121ac75dd17c9790e38ed8c68e2 |
| verification/sprint4_tests.cpp.txt | 5f2e385767f8869fe74bc3de53556fc8c6242c38444e3eae16e5f76429d2a958 |

Focused tests are not a win-rate evaluation or full sandbox compliance proof.
Root owns independent review, frozen paired schedules, negative-result
retention, official replay audit and any promotion decision.
