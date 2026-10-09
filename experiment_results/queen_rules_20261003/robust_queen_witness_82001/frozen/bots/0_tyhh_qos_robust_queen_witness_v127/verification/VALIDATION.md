# Source-bound R20 Regression Checks

The three inherited H14 fixtures were copied without test-policy changes
and compiled against this R20 main.cpp using g++-10, C++20, -O2.
All three binaries exited0 before R20 freeze:

- queen_guard_tests.cpp.txt:7 exact-body/continuation assertions passed.
- consolidate_tests.cpp.txt:8 recipient/reserve/path checks passed.
- candidate_tests.cpp.txt:projection, queen identity/bootstrap and portal guards passed.

Test hashes respectively:
`9dea7beefc3b48de61e12133c33b42bdcafb80f9ecfe47fd6645e62b9d913def`,
`5de377877b7bbada6b7f8f2197d8db9544c2bec26b213251ca3bb7bbacdfa3e4`,
`a575e2e4b3eff22ad8b2e90c1a35355fd5d6d4c0af01956551af98adf42cd3e2`.

New `queen_witness_tests.cpp.txt` and its `H14_CONTROL` arm also exited0.
They use simplified known geometry for source gates, including bent and
partial body completeness and per-enemy mask-subset checks. Separate
`queen_witness_engine.py` fed actual official round199 input through fresh
R20/H14 production binaries and official unswbc1.2.7 action-tape arms; see
`official_witness.json` and READY for exact actions, hashes and limitations.

Renaming main for test includes produces an inherited no-return warning
in bot_program_main; no test calls that function. Normal bot builds passed
without warnings. Helper/feed/consolidate stay byte-identical to H14.
These fixtures establish no competitive win-rate or unknown-enemy safety;
body-inference correction and intervening-turn limits are documented in
EXPERIMENT and READY. No production or confirmation seed was modified.
