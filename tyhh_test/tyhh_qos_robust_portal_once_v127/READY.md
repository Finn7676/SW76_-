# R18 Ready

Frozen H14 child, single factor: actual same-dragon portal-ID crossing history
removes repeated round<80 forced exploration privilege. Repeat portals remain
available to the unchanged scorer and safety reviews. Recording uses the
already emitted pendingPortalKey only after the next actual landing updates
learnedExit; changed MYID clears history and prevents parent/sonar inheritance.

Production g++-10 C++20 O2 clean build passed. Source-bound candidate/H14
controls passed. Real default r55 current observation changes final MOVE N
out of room to MOVE S collecting a visible pearl, surviving at length3. The
same result holds with reconstructed self-observed historical cell geometry.
Real r39 remains MOVE N after the ordinary scorer, retained as a negative
control. Exact sonar/old pearl-timer memory is not reconstructed; these tests
do not claim deterministic full-memory replay or any competitive win gain.

Tests also cover first crossing unchanged, confirmed earlier entry through a
different side of the same PID, duplicate confirmation, ordinary move/invalid
landing not marking, new dragon clearing history, sonar not marking, another
PID preserving exploration priority, round80 falling through, repeat portal
as the only survival exit and protected long-dragon unseen landing rejection.
No claim-cap/global split/feeding/scoring/queen/portal safety rule was changed.
No competitive games launched by this implementation task.

Commands from this directory:

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/robust_portal_once_bot
g++-10 -std=c++20 -O2 -x c++ verification/portal_once_tests.cpp.txt -o /tmp/robust_portal_once_tests
/tmp/robust_portal_once_tests
g++-10 -std=c++20 -O2 -DPORTAL_ONCE=0 '-DBOT_SOURCE="../../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"' -x c++ verification/portal_once_tests.cpp.txt -o /tmp/robust_portal_once_control
/tmp/robust_portal_once_control
```

The included-main tests have inherited renamed-main no-return warnings in
both variants; production has none. Complete recorded outputs are in
verification/VALIDATION.md. Helpers/feed/consolidation/config are byte-identical
to H14 and no opponents, maps, engine or production bot were edited.

SHA-256:

| Artifact | Hash |
| --- | --- |
| main.cpp | 3d602294f6b6c9224c0a2d6be825d9333fb1c9bd5f27d73c77e2a8c2ed52ad0c |
| portal_once.hpp | e1a6498903345453a8ad0fc6a59db0795bf5d3916669f0fb245b0a8d9491ed86 |
| source-bound tests | 162f6f5fdb86710efe230722e9f39c6dc35febf9bd965023bbce6e7095f2847e |
| self-observed cell fixture | 0d6e84e394747c7c0b8edf196cc2dcf59a71df812ab4497ffecdf5ca42f3530e |
| frozen H14 main | ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc |
