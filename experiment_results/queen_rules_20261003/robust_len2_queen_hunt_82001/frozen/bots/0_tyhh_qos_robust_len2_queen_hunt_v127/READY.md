# R16 Ready

Production compiled cleanly with `g++-10 -std=c++20 -O2`. Candidate and
actual frozen R13 control tests passed. Parent main was rehashed unchanged;
main diff is exactly `LEN < 3` to `LEN < 2` in `sprintRamAction`.
All three headers and `bot.toml` are identical to R13. No matches launched.

Length-two tests distinguish first-pearl-funded two-step queen attacks from
R13 rejection, reject unfunded/current-paid-step-only pearls, preserve fixed
free budget and depth four, and check own/static bodies, release timing,
portals, unknown edges/cells, visibility, population, champion, queen,
incomplete-body gates and initial-length ordinary worth. Wide four-step
queen priority also passed. The source-bound `takeTurn` fixture selects
two-step attack in R16, versus R13's one-step scored collector fallback.

All 200 deterministic length-at-least-three random attack outputs matched
the actual R13 binary byte-for-byte; 74 were nonempty. The inherited exact
sprint, queen-value (200 ordinary R4 controls) and combat-order tests passed.
Include-main tests emit the inherited renamed-main no-return warning only.

Run from this candidate directory:

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/robust_len2_queen_hunt_bot
g++-10 -std=c++20 -O2 -x c++ verification/len2_hunt_tests.cpp.txt -o /tmp/robust_len2_queen_hunt_tests
g++-10 -std=c++20 -O2 -DLEN2_ALLOWED=0 '-DBOT_SOURCE="../../tyhh_qos_robust_queen_combatfirst_v127/main.cpp"' -x c++ verification/len2_hunt_tests.cpp.txt -o /tmp/robust_len2_queen_hunt_control
/tmp/robust_len2_queen_hunt_tests
/tmp/robust_len2_queen_hunt_control
diff -u <(/tmp/robust_len2_queen_hunt_tests --controls) <(/tmp/robust_len2_queen_hunt_control --controls)
g++-10 -std=c++20 -O2 -DLEN2_ALLOWED=1 -x c++ verification/combatfirst_tests.cpp.txt -o /tmp/robust_len2_queen_hunt_order
g++-10 -std=c++20 -O2 -DLEN2_ALLOWED=0 '-DBOT_SOURCE="../../tyhh_qos_robust_queen_combatfirst_v127/main.cpp"' -x c++ verification/combatfirst_tests.cpp.txt -o /tmp/robust_len2_queen_hunt_order_control
/tmp/robust_len2_queen_hunt_order
/tmp/robust_len2_queen_hunt_order_control
g++-10 -std=c++20 -O2 -x c++ verification/queen_hunt_tests.cpp.txt -o /tmp/robust_len2_queen_hunt_value
/tmp/robust_len2_queen_hunt_value
g++-10 -std=c++20 -O2 -x c++ verification/sprint4_tests.cpp.txt -o /tmp/robust_len2_queen_hunt_sprint
/tmp/robust_len2_queen_hunt_sprint
```

## SHA-256

| File | SHA-256 |
|---|---|
| Parent R13 main | `0f9e2dd03bae32ce2a9113e283cbf4ed3822ee3f37f48ab36eecdcd178574830` |
| R16 main.cpp | `6dab223fc5765440f6dd598bd5919bc4bb95d24afcdae328738b5664810a1f0f` |
| helper.hpp | `6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc` |
| consolidate.hpp | `bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf` |
| queen_feed.hpp | `0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24` |
| bot.toml | `d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5` |
| len2_hunt_tests.cpp.txt | `0d2aea464a8151b96437ee3d8cbb9dcdb50db87a5bfcc87d78f92d6884e41ec1` |
| combatfirst_tests.cpp.txt | `04b5838eb7a662aa63d4dc6d22a447da3f69fb8b3c8374e7dd69ce360f87b9b5` |
| queen_hunt_tests.cpp.txt | `f93e77e4c2e8312dfb7063894640f87d67522342c4eec96d9e23cc7a6927c703` |
| sprint4_tests.cpp.txt | `5f2e385767f8869fe74bc3de53556fc8c6242c38444e3eae16e5f76429d2a958` |
