# R21 Independent Source Review

Reviewed 2026-10-06 after the author froze `R21 READY`.
Candidate: `tyhh_test/tyhh_qos_robust_rear_cascade_v127`.
Control: frozen H14 `tyhh_qos_counter_queen_guard_consolidate_v127`.
No bot source was edited, no competitive game was run, and no data was removed.

## Verdict

No blocking implementation issue was found for the preregistered development
screen. The frozen candidate implements one ordinary-growth allocation change:
an otherwise eligible early nonqueen leaves parent2 and childLEN-2 instead of
parentLEN-2 and child2. This review does not establish parent/child survival,
resource advantage, competitive improvement, or judge-budget compliance.

## Source And Risk Findings

- `main.cpp:1235` through the five added lines are the only production-source
  differences from H14. The original `wantExpand`, consolidation hold,
  TARGET, length, large-map, `can_split(2)`, and `splitSafe(2)` admission
  gates remain at line 1233. The new allocation is restricted to nonqueens,
  ROUND<350, LEN>=5, `can_split(LEN-2)` and `splitSafe(LEN-2)`; otherwise
  the unchanged split2 fallback at line 1240 runs.
- Queen bootstrap at `main.cpp:1232`, early portal priority, combat priority,
  tactical/emergency splits, and the final safety/protection ordering are
  unchanged. The existing emergency reverse split at line 1457 still uses
  its original tailCanEscape condition. No R20 witness code is composed.
  Helper, TOML, feed, and consolidate hashes match frozen H14 exactly.
- Residual policy risk, not a scope violation: `main.cpp:725` returns false
  from `bridged` when either segment is -1, so inherited `splitSafe` at line
  731 can accept incomplete own-body evidence. It checks the two-segment
  portal geometry only when the relevant cells are available. R21 adds no
  complete-body proof or tail-escape gate to ordinary splitting. The new
  parent2 and larger reversed child can still die, encounter hidden portals,
  congest each other, consume population capacity, lose free movement, or
  explore a less useful direction. In particular, an unavailable neck can
  allow parent2 to straddle a portal, which is a known engine death mechanism
  documented beside the inherited guard at line 730. The experiment measures
  these effects; an accepted split must not be called input-proven safe or
  complete-visible. The official LEN14 fixture itself does not imply the
  entire initial body fits the 7x7 view.
- Nonblocking documentation risk: `verification/VALIDATION.md:1` retains
  copied H14 wording and asserts compilation against H14 and frozen H14
  main hashes. Those statements are stale for this directory. R21 READY,
  the actual R21 source-bound tests, and the independent reruns below are
  authoritative. Correct the validation record separately without changing
  or pooling frozen production source results.

## Independent Verification

The READY source/support/test/script hashes were rechecked before testing
and remain unchanged after the reruns. All executed checks exited 0:

- g++-10, C++20, -O2 production R21 build, without warnings. The already
  independently built frozen H14 control has the expected parent source and
  binary hashes below.
- `rear_cascade_tests.cpp.txt` compiled against R21 and separately against
  H14 with REAR_CASCADE=0. Both passed the 17 asserted boundaries: LEN4/5/7/14,
  ROUND349/350, champion, large-map length gate, consolidate hold, portal
  priority, queen/bootstrap, TARGET and unit cap, new-size portal fallback,
  original split2 rejection, and unchanged trapped reverse split. These are
  direct takeTurn/source assertions, not competitive outcomes.
- The inherited projection, queen-guard, and consolidation tests were each
  independently compiled against R21 and passed. Include-based renamed-main
  tests alone emit the inherited no-return warning; no test calls that main.
- `rear_cascade_native.py` was independently run with the new native R21
  binary and frozen H14 native binary under `.venv-battlecode/bin/python`.
  It checks unswbc 1.2.7 and records the official engine hash. All four cases
  passed. H14 LEN7/LEN14 split only child2, retaining parent5/12. R21 LEN7
  produces children5,3; R21 LEN14 produces12,10,8,6,4,2. Each new parent has
  length2, split body arrays match prefix/reversed-suffix allocation, and
  newborn native processes split or move in the same round. Every case has
  no round0 death and no engine notices.

Permanent independent raw rerun record:
`r21_review_evidence/r21_independent_native.json`, SHA256
`bc8e4f4b177ac98db424e303a4b321e1ec59fd4cbaf6cc1f0df54c8208b110e2`.
This independently reproduced byte-identical JSON to the author's record;
it retains native replies, binary/map hashes, split arrays, move events,
deaths, and notices. The packed replay is parsed by the fixture script but
is not exported. No original evidence was overwritten.

## Remaining Coverage And Screen Advice

The official native fixture is a deliberately empty modified autarky map:
pearls are disabled and edges are ordinary. It establishes allocation and
same-round scheduling, including native final-review behavior in those four
cases. It does not reproduce the motivating competitive snapshots or test
hidden portal/crowding consequences. All survivors are intentionally sent
ENDTURN at round1; those deaths are fixture cleanup, not strategy outcomes.
The direct focused fixtures give the body a synthetic full trail, so their
portal rejection cases do not prove rejection of missing-body evidence.

The new gate can be judged against the unchanged H14 gate by source and
mechanics evidence, but success needs matched development games. Audit
eligible ordinary splits and same-round child cascades separately from
queen bootstrap, late/tactical splits, parent2 deaths, population cap,
and subsequent total/queen lengths. Keep positive and negative trajectories
and do not pool R20 with R21. No native test here certifies judge points or
memory; sandbox validation remains required before production use.

## Frozen Hashes

- H14 main: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`
- R21 main: `eb364e6e9ccf138a4b3b12d071e356a2ecee6300a4b0209675fe6888382f5573`
- Focused tests: `21e204671dec880b48a8c5d66e2353a12c731710c68ada6658fec6d68ba311b0`
- Native fixture script: `c7236fd697193401d2e79bb39b39bf1ab93d1ce2d218bba0a6eb9bb1a281259f`
- Native R21 binary: `95ae4b8773761e37f2bc85e8c2cd7016c3b79dd2885db3fe7d9eaa9c4d8ba9f5`
- Native H14 binary: `98a8c546c653a6a9ae1aabdae284a22ef460ac3bc525bf842ed31223f3232c9f`
- Official 1.2.7 engine: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`
