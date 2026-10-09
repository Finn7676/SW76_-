# R5 Validation

All tests below included this R5 main.cpp and used g++-10, C++20, -O2.
All four binaries exited0:

- routine_split_tests.cpp.txt:10 cases covering safe ordinary expansion,
  U-shaped parent trap retaining child cells, child escape rejection,
  unknown-edge and hidden-landing parent rejection, legal length2 nonqueen
  parent, unchanged queen bootstrap, emergency reverse split, emergency
  last-resort two-tail split, and unchanged portal-bridge rejection.
- candidate_tests.cpp.txt:projection, queen identity/bootstrap and portal guards.
- queen_guard_tests.cpp.txt:7 exact-body/continuation assertions.
- consolidate_tests.cpp.txt:8 recipient/reserve/path checks.

New fixture SHA-256:
`70a13152105b35cda6be66b82ced0ee8e695714ee20369f45371bd36c558f434`.
The three inherited H14 fixtures remain byte-identical. Renaming main for the
test include produces the inherited no-return warning in bot_program_main;
the harness does not call that function. The normal bot build passed cleanly.

A source diff contains only the new helper and one ordinary-expansion gate.
Queen bootstrap bypasses that gate; queen final review remains untouched.
Both emergency scorer split statements, splitSafe and tailCanEscape are
untouched. These tests do not establish competitive gains, whole-map points
or memory limits, unknown enemy safety, or survival beyond one continuation.
