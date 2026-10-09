# R24 Source Verification

Production main SHA256
`cea6542e3c5d8b462730456afa0289503f06e7d507098a5a39b38af9cf005857`.
Against frozen H14 the only diff is the resource proof bound24->49 and its
comment. Helper, queen_feed, consolidate and TOML are unchanged copies.

Production `g++-10 -std=c++20 -O2 main.cpp -o /tmp/r24_bot` exited0 cleanly.
Source-bound `room_bound_tests.cpp.txt` compiled/run with candidate bound49
and actual H14 control bound24; both exited0. Cases include exact24/25/49/50,
unknown edges, missing/negative/infinite resource time, future time, no
boundary, same-ID distinct boundary, different-ID boundary, invalid start,
and two incident sides of the same internal canonical portal edge.

The three inherited focused suites are byte-identical to H14, compile/run
exited0: candidate projection/payment/identity/portal, seven queen-guard
assertions, eight consolidation checks. Include-based renamed-main harnesses
emit the inherited no-return warning; production build does not. No harness
calls the renamed main. These are focused tests, not competitive/budget proof.

```bash
g++-10 -std=c++20 -O2 -x c++ verification/room_bound_tests.cpp.txt -o /tmp/r24_room_tests
g++-10 -std=c++20 -O2 -DROOM_BOUND=24 '-DBOT_SOURCE="../../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"' -x c++ verification/room_bound_tests.cpp.txt -o /tmp/r24_room_control
/tmp/r24_room_tests
/tmp/r24_room_control
```

Observable-history controls, official native fixture and independent review
are pending. No competitive game or confirmation seed has been used forR24.
