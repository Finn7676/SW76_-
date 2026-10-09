# R15 Ready

Production g++-10 C++20 O2 compilation passed cleanly. Source-bound candidate
and frozen H14 controls passed. Official unswbc 1.2.7 native-bot deterministic
smoke passed all five cases, including actual stripes and mirror body/split
semantics plus an enemy intervention boundary. No competitive games launched.

Single factor: terminal-pocket ordinary split arbitration. Only the existing
approved routine SPLIT 2 branch can call this policy. A fully visible nonqueen
length-four body with one proven terminal pearl move collects first. A fully
visible length-five body already at a proven terminal rechecks current cap,
splitSafe(3), normal parent2 neck, clear reversed child3 two-step continuation,
no visible/recent enemy and hard/soft friend risk, then selects SPLIT 3. Other
queen, enemy, unknown, body, cap, holding and ordinary-split cases retain H14.
Main changes are exactly one include and one call; the emergency scorer,
global split thresholds, portal exploration and queen rules are untouched.

Source-bound full two-turn fixtures reproduce candidate LEN4 pickup -> exact
LEN5 body -> SPLIT3 -> exact reversed child3 -> two surviving child moves on
both stripes orientations. H14 controls reproduce premature SPLIT2 -> LEN3
terminal pickup -> self death. Engine records confirm the extra escaping rear
segment, not survival of both dragons: terminal parent2 still self-dies. With
a newly visible enemy at the next turn, split3 is rejected and ordinary split2
retained. This is a rechecked local policy, not a guarantee about future allies
or cap changes. The short POCKET_SALVAGE tag denotes a proposal, not a win.

Commands from this directory:

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/robust_pocket_salvage_bot
g++-10 -std=c++20 -O2 -x c++ verification/pocket_salvage_tests.cpp.txt -o /tmp/robust_pocket_salvage_tests
/tmp/robust_pocket_salvage_tests
g++-10 -std=c++20 -O2 -DPOCKET_SALVAGE=0 '-DBOT_SOURCE="../../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"' -x c++ verification/pocket_salvage_tests.cpp.txt -o /tmp/robust_pocket_salvage_control
/tmp/robust_pocket_salvage_control
g++-10 -std=c++20 -O2 ../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp -o /tmp/robust_pocket_h14_bot
../../.venv-battlecode/bin/python verification/pocket_engine_smoke.py --candidate /tmp/robust_pocket_salvage_bot --control /tmp/robust_pocket_h14_bot --output /tmp/pocket_engine_smoke_repeat.json
```

Included-main tests have the inherited renamed-main no-return warning in both
candidate and control. Production has no warnings. Engine smoke output uses
exclusive creation; use a fresh output path for each rerun. Smoke ends survivors
with ENDTURN at round3 deliberately; those A deaths are fixture termination.

SHA-256:

| Artifact | Hash |
| --- | --- |
| main.cpp | 492d5a8b17286f12ee0d471103363a51f1cc84fed799de1d0697cdd9ce5686d3 |
| pocket_salvage.hpp | a6b2e733ecc0abf706ecd05fa9aa6dbad9054b9aa44e52619c6b94adb417397e |
| source-bound tests | c0b980ac730d54ff89356f68435d04d9c54f087174fc722a9da54f1777ccabcf |
| engine smoke script | c9c93b33585c8d603135acdfd26fcf90a77a3f700e480f06c0da66ab21cd5ea5 |
| engine smoke JSON | 65b461c242dedd08632a2d2f6264274c175d45a1a8ab2dd8fc436ff05d5cf4ef |
| official 1.2.7 engine | 26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546 |
| stripes map | 31b22a24c467edb146f8c6fc291beccce1493617d90ff5bb6eff538c3af24715 |
| frozen H14 main | ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc |

Helper/queen_feed/consolidate/bot.toml remain byte-identical to frozen H14.
Opponent main/planner hashes remain unchanged after implementation:
V3_portal 3323482722c3c01e85d3fe7d7366e02a46ee1047ebd1d7c24bd07b109f246632 /
5517d00108434109939163d604c78b7d5e31c389e60c3e56bc909d18f3181788;
xxqztest 83f85b056b482d7488762bf73dcd07de09cd1f1485dc9f61e4e8b991f43a815c /
f0f4a570085bf5efedb8abf22c2dea63ac004f1d3494dac0dbe86c86a2667c09.
