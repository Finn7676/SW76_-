# H3 Queen Guard Ready

The H3 implementation is ready and will not be edited after this registration.
Root will freeze the entire candidate tree before scheduled matches.

Current main SHA-256:
`e51392be8df1b5a0350750a64dcb2a99ffe4a6c8f7e33c27321d84229b48dab4`.
Current helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
Focused test SHA-256:
`9dea7beefc3b48de61e12133c33b42bdcafb80f9ecfe47fd6645e62b9d913def`.

Verification command:
```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_counter_queen_guard_v127/verification/queen_guard_tests.cpp.txt -o /tmp/queen_guard_tests
/tmp/queen_guard_tests
```
Output: `queen_guard: 7 exact-body survival assertions passed`.
The included parent main renamed by the fixture triggers a no-return warning;
the normal bot entry point remains `main`, where implicit return is permitted.

The test covers neck/tail collision, known safe alternative, friendly collision,
safe split preservation, safe normal proposal preservation, trapped split
preservation when a child can escape but the queen cannot be rescued, and synthetic
geometry from queen_v127 portals / xxqztest / B indices110-111. It is not a
full state replay reproduction, engine integration run or win-rate claim.

Parent historical validation follows, not H3 evidence:

# Queen v1.2.7 Freeze and Validation

Code evaluated in `experiment_results/queen_rules_20261003/queen_v127/frozen/`.
Current main.cpp SHA-256:
`c97b183f858af950d3cf31e2dfc0bfa5d080caaf77852f32c5ef6e60630b456a`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.

Native g++-10 build and C++ focused assertions passed. The 1.2.7 engine's
27 microfixtures passed. All68 native matches and4 separate portals sandbox
matches completed without runtime failure and passed replay/source audits.
Post-evaluation documentation additions do not alter frozen source evidence.

No joint-victory or full-map sandbox-compliance claim; see RESULTS.md.
