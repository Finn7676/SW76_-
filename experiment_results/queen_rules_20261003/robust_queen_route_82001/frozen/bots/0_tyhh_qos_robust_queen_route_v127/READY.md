# R9 Ready

Production g++-10 -std=c++20 -O2 build passed cleanly. Focused candidate and
H14 parent-control assertions passed. The fixture distinguishes the new
approach from the parent's routine split; when the queen is already within
the verified pickup range both versions use the original donation gate.
Candidate exclusions cover queen/nonqueen identity, round/population/length,
quiet history, current target visibility/occupancy, unknown edges, portals,
blocked steps, hard/soft friendly exit protection and remaining time.

Commands:

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/robust_queen_route_bot
g++-10 -std=c++20 -O2 -x c++ verification/queen_route_tests.cpp.txt -o /tmp/robust_queen_route_tests
/tmp/robust_queen_route_tests
g++-10 -std=c++20 -O2 -DQUEEN_ROUTE=0 '-DBOT_SOURCE="../../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"' -x c++ verification/queen_route_tests.cpp.txt -o /tmp/robust_queen_route_control
/tmp/robust_queen_route_control
```

The included-main fixture has the inherited renamed-main no-return warning
in both candidate and control, not in the production build.

Main SHA-256: `7ff82872b0e616d37a8d59fb6468131e257b5496099f444cd9e4c275b4f71b80`.
New header SHA-256: `01247e6c48b7bb57df4455afe0e08e6358c79c6f866e8c521e38b2d05520a497`.
Helper/queen_feed/consolidate/config remain byte-identical to H14.
Parent main remains `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
Inherited H14 READY/RESULTS were moved to provenance to avoid misattribution.
