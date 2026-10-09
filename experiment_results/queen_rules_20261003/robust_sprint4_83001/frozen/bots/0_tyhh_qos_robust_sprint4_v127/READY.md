# R4 Ready

Parent H14 main SHA-256:
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
Only the proactive sprint-ram factor changed. All five parent dependencies
were copied exactly before this edit; helper, config, feed and consolidation
remain byte-identical. Queen safety, first-ram and early queen-ram policies
remain at their original pipeline stages. No matches launched.

Native build passed cleanly:

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_sprint4_v127/main.cpp -o /tmp/robust_sprint4_bot
```

Focused test build and execution passed:

```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_sprint4_v127/verification/sprint4_tests.cpp.txt -o /tmp/robust_sprint4_tests
/tmp/robust_sprint4_tests
```

Output: `sprint4: reach, paid/free rules, exact body and eligibility assertions passed`.
The included-main test harness produces the inherited renamed-main
no-return warning; the production main build is warning-free.

Test assertions cover 4-step reach; fixed free steps at length4 and length8;
paid-step eligibility before a collision pearl; eat-before-payment; current
tail collision and genuinely released old segments; static other bodies;
queen/champion/low-population/length2 rejection; original worth thresholds;
observed-size then path-length selection; portal/unknown/invisible rejection;
and incomplete initial body rejection.

Main SHA-256:
`455fbc4744a2634a9c529a61f6b4ee582d7d6da0ceb85d2854b81aba39b60675`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
bot.toml SHA-256:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`.
queen_feed.hpp SHA-256:
`0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`.
consolidate.hpp SHA-256:
`bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`.
Focused tests SHA-256:
`5f2e385767f8869fe74bc3de53556fc8c6242c38444e3eae16e5f76429d2a958`.

Source edits stop after this record. Root owns freeze, review and evaluation.
