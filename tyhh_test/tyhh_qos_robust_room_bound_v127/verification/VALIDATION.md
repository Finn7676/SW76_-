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

The four observable-history cases passed on both candidate and actual H14.
The official fresh-process native1.2.7 fixture passed4/4; inputs, outputs,
maps and replay hashes are in room_native.json/artifacts. Native layouts
preserve default topology but use scripted respawn intervals/other actors,
not historical production memory. H14 Nrecross vs R24 Spickup in r55;
r39 bothN is an explicit negative control. Deliberate round1 ENDTURN is
fixture cleanup, not competitive survival evidence.

Independent no-blocker implementation review completed2026-10-08 in
experiment_results/xxqz_robust_20261004/r24_independent_review.md, before
competitive launch. The registered82001screen has now completed68/68:
V3 15/34,xx16/34. Official outcome/manifest audit passed68/68 without failed
matches, manifest failures or audit failures. Paired outcomes versus H14
are unchanged. Committed physical/SONAR/action analysis is still separate.
No confirmation seed has been used forR24.
