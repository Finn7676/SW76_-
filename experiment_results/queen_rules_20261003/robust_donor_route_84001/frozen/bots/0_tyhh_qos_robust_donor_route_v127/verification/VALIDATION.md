# R3 Local Validation

2026-10-04. Native g++-10 C++20/O2 build and focused executable tests passed.
No competitive or sandbox matches were launched by this implementation task.
Native build success does not establish judge points or memory compliance.

Results:

- donor_route_tests.cpp.txt: 24 routing and exclusion assertions passed.
- inherited queen_guard_tests.cpp.txt: 7 exact-body survival assertions passed.
- inherited consolidate_tests.cpp.txt: 8 recipient, reserve and path checks passed.
- inherited candidate_tests.cpp.txt: projection, queen identity/bootstrap and portal guards passed.

The included-main test harness emits the inherited renamed-main missing-return
warning; the actual main build emitted no warnings. All test executions exited 0.

Read-only source comparison verified unchanged byte-identical helper.hpp,
bot.toml, queen_feed.hpp and consolidate.hpp. The parent main differs only in
the experiment comment, donor_route.hpp include and donorRouteAction branch
following the unchanged feeding/consolidation action calls.

Routing does not promise receiver pickup, reserve cells or prove a win effect.
Receiver movement, turn order, local visibility, resource opportunity cost and
the subsequent inherited freeHarvest/review pipeline can limit the mechanism.
The existing consolidateAction retains responsibility for every actual death.
