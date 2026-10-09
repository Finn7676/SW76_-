# Source-bound H14 Regression Checks

After frozen confirmation, existing tests were copied without policy edits
and compiled against this H14 main.cpp, not a parent. g++-10, C++20, -O2.
All three binaries exited0:

- queen_guard_tests.cpp.txt:7 exact-body/continuation assertions passed.
- consolidate_tests.cpp.txt:8 recipient/reserve/path checks passed.
- candidate_tests.cpp.txt:projection, queen identity/bootstrap and portal guards passed.

Test hashes respectively:
`9dea7beefc3b48de61e12133c33b42bdcafb80f9ecfe47fd6645e62b9d913def`,
`5de377877b7bbada6b7f8f2197d8db9544c2bec26b213251ca3bb7bbacdfa3e4`,
`a575e2e4b3eff22ad8b2e90c1a35355fd5d6d4c0af01956551af98adf42cd3e2`.

Renaming main for the test include produces an inherited no-return warning
in bot_program_main; the test harness never calls that function. Normal bot
build passed. Main/helper/feed/consolidate hashes remain frozen READY values.
These fixtures do not prove unknown-enemy safety or whole-map sandbox limits.
