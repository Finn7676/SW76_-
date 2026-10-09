# R21 Native Allocation Fixture

Four deterministic native bot fixtures passed with installed official
`unswbc 1.2.7`, engine SHA-256
`26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`.
These are local mechanism tests, not competitive matches or win-rate data.

The fixture retains the autarky map dimensions but replaces every edge with
ordinary open terrain and removes every pearl. Two isolated queens receive
scripted ordinary moves. The length-seven or length-fourteen nonqueen and
its children run the unmodified production candidate/control main loops.

| Source | Initial Nonqueen Length | Committed Round-Zero Child Sizes |
|---|---:|---|
| H14 control | 7 | 2 |
| R21 candidate | 7 | 5, 3 |
| H14 control | 14 | 2 |
| R21 candidate | 14 | 12, 10, 8, 6, 4, 2 |

Every official split event was checked against the exact parent-prefix and
reversed rear-child suffix. Every candidate parent retains two geometrically
adjacent ordinary body cells. Each final rear child commits an actual MOVE
and receives an official successful movement update in the same round.
There were no round-zero deaths. All survivors deliberately receive only
ENDTURN at round one to terminate the fixture; the resulting invalid-action
deaths are not competitive outcomes and are not evidence of failed policy.

This establishes same-round allocation and action scheduling. It does not
establish safe future motion, resource improvement, reduced congestion, or
victory on the original autarky/slithery maps. The straight length-fourteen
body is not fully within the actor's initial view; no full-body assumption
is added to the bot or to the claimed candidate eligibility.

## Reproduction

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_rear_cascade_v127/main.cpp -o /tmp/rear_cascade_bot
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp -o /tmp/rear_cascade_bot2
.venv-battlecode/bin/python tyhh_test/tyhh_qos_robust_rear_cascade_v127/verification/rear_cascade_native.py --candidate /tmp/rear_cascade_bot --control /tmp/rear_cascade_bot2 --output /tmp/rear_cascade_native_fresh.json
```

Run from the repository root. Output creation is exclusive; use a fresh
output path rather than overwriting the retained JSON.

## Binding

| Artifact | SHA-256 |
|---|---|
| Candidate main | `eb364e6e9ccf138a4b3b12d071e356a2ecee6300a4b0209675fe6888382f5573` |
| H14 main | `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc` |
| Candidate native binary | `95ae4b8773761e37f2bc85e8c2cd7016c3b79dd2885db3fe7d9eaa9c4d8ba9f5` |
| H14 native binary | `98a8c546c653a6a9ae1aabdae284a22ef460ac3bc525bf842ed31223f3232c9f` |
| Native fixture script | `c7236fd697193401d2e79bb39b39bf1ab93d1ce2d218bba0a6eb9bb1a281259f` |
| Retained fixture JSON | `bc8e4f4b177ac98db424e303a4b321e1ec59fd4cbaf6cc1f0df54c8208b110e2` |

Retained output: `rear_cascade_native.json` beside this report. Runner code
is adapted from R15 `verification/pocket_engine_smoke.py` with attribution
in its module docstring. No production, opponent, map, or engine file was
modified, and no competitive seeds were launched by this validation.
