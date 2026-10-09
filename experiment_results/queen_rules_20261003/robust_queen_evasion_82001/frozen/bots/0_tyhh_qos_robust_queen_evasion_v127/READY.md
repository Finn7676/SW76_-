# R6 Ready

Frozen 2026-10-05 after registration on 2026-10-04 and focused validation.
Parent H14 main remains SHA-256
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
The only main diff is QueenApproach, queenApproach, reviewQueenEvasion and
its pipeline call immediately after reviewQueenSurvival. Nonqueen behavior,
no-enemy turns and all copied helper/feed/consolidation files are unchanged.

Search is bounded to 340 nonempty 1-4-step direction sequences, with at most
128 popped ordinary-visible BFS nodes per enemy head per evaluated sequence.
Depth is four. This is a static proximity heuristic, not exact legal enemy
attack reach. Unknown/hidden/portal frontiers cannot certify a better risk.
Paid queen movement remains governed by original exact projectAction.

Main build and R6/control fixture execution passed. No match was launched.
Root owns resource validation and paired frozen evaluation. Source edits
stop after this record.

SHA-256:

- main.cpp: `8b80b77c3fba16b22d2fbbbd948a52f98d315fdd2e1accd6df8dde12dff927aa`
- helper.hpp: `6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`
- queen_feed.hpp: `0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`
- consolidate.hpp: `bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`
- bot.toml: `d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`
- verification/evasion_tests.cpp.txt: `6995346d1a62c15e42cf20d1e71b5c24f990a9120331b9193eb7a23df8543b42`
