# R13 Ready

Production build is clean. Candidate ordering and R10 parent-control
ordering fixtures passed, as did queen target-value legality and200 frozen
R4 ordinary target controls. The inherited control ordering fixture prints
`R4 parent` but BOT_SOURCE explicitly compiled R10, as documented below.

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/robust_queen_combatfirst_bot
g++-10 -std=c++20 -O2 -x c++ verification/combatfirst_tests.cpp.txt -o /tmp/robust_queen_combatfirst_order
/tmp/robust_queen_combatfirst_order
g++-10 -std=c++20 -O2 -DCOMBAT_FIRST=0 '-DBOT_SOURCE="../../tyhh_qos_robust_queen_hunt_v127/main.cpp"' -x c++ verification/combatfirst_tests.cpp.txt -o /tmp/robust_queen_combatfirst_order_control
/tmp/robust_queen_combatfirst_order_control
g++-10 -std=c++20 -O2 -x c++ verification/queen_hunt_tests.cpp.txt -o /tmp/robust_queen_combatfirst_value
/tmp/robust_queen_combatfirst_value
```

Include-main tests share the inherited renamed-main no-return warning.
Main SHA-256: `0f9e2dd03bae32ce2a9113e283cbf4ed3822ee3f37f48ab36eecdcd178574830`.
All four dependencies are copied unchanged from R10. No parent edited.
