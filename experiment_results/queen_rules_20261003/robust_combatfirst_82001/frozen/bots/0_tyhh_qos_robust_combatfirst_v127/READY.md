# R8 Ready

Parent R4 main SHA-256:
`455fbc4744a2634a9c529a61f6b4ee582d7d6da0ceb85d2854b81aba39b60675`.
The parent was verified unchanged after this implementation. Only the
`sprintRamAction` call block moved: after adjacent `FEAT_RAM_FIRST`, before
routine growth. The attack function and all thresholds are byte-identical
to R4. Helper, config, queen-feed and consolidation files remain identical
to R4. R7 is not a parent. No matches launched.

Production build passed without warnings:

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_combatfirst_v127/main.cpp -o /tmp/robust_combatfirst_bot
```

Focused candidate and parent-control builds and execution passed:

```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_combatfirst_v127/verification/combatfirst_tests.cpp.txt -o /tmp/robust_combatfirst_tests
/tmp/robust_combatfirst_tests
g++-10 -std=c++20 -O2 -DCOMBAT_FIRST=0 '-DBOT_SOURCE="../../tyhh_qos_robust_sprint4_v127/main.cpp"' -x c++ tyhh_test/tyhh_qos_robust_combatfirst_v127/verification/combatfirst_tests.cpp.txt -o /tmp/robust_combatfirst_parent_tests
/tmp/robust_combatfirst_parent_tests
```

Output:

```text
combatfirst: candidate takeTurn order, growth fallback, first-ram, queen and champion assertions passed
combatfirst: R4 parent takeTurn order, growth fallback, first-ram, queen and champion assertions passed
```

These assertions establish that split-eligible scouts attack 2, 3 and 4
steps away before growing only in R8; a length-four pearl-supported attack
does likewise. No-target splitting, adjacent first-ram, queen bootstrap and
late-champion scored movement are unchanged. The four-step target is reached
by a bent path inside the current 7x7 view. The included-main test harness
produces the inherited renamed-main no-return warning in both builds.

Main SHA-256:
`8cabd799a8c289ee42b166e02b16cf497bd54f11b676b7cbdc9bb2ca305b38f4`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
bot.toml SHA-256:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`.
queen_feed.hpp SHA-256:
`0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`.
consolidate.hpp SHA-256:
`bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`.
Focused tests SHA-256:
`d47e583dd8f63fcd3e6090ffff343bba3508ad1270d7613ebbabb5bc46dc12ec`.

Implementation stops at this record. Root owns freeze, independent review,
paired evaluation, audit and all result claims.
