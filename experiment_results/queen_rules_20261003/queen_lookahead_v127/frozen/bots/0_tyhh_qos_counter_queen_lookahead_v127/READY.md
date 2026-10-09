# H4 Freeze and Focused Tests

main.cpp SHA-256:
`fcaab823b9dbe472efb3fc356e606c6c971a62dc65c52a36e6d55b5563f83736`.
helper.hpp unchanged:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
Focused test SHA-256:
`5c7916093deaa7af1883be624c21b077bb919451e286223386d0bf99881b1e5f`.

`g++-10 -std=c++20 -O2 -x c++ verification/queen_body_tests.cpp.txt`
compiled; executable reported seven continuation checks passed. It covers
collision with neck, open paths, known dead-end redirect, uncertainty,
single-consumption pearl loop and current ally blocking. This is not a
full-game survival guarantee or sandbox resource test. Sources frozen.

Historical parent evidence follows, not H4 results.

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
