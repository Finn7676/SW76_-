# R26 Result Audit

Read-only audit completed 2026-10-08 for the frozen development screen
`robust_headtrade_82001` against `robust_h14_82001`. No source, replay, map,
engine, opponent, or result artifact was modified. The audit checks the saved
manifests, frozen trees and binaries, all replay/log hashes, schedule coverage,
winner reconstruction from replay standings, and the paired comparison keys.

Both packages contain all 68 planned games and pass the saved verifier with
zero audit failures, manifest failures, failed matches, or draws. The two
packages use the same unswbc 1.2.7 engine, CLI/module tree, V3_portal and
xxqztest frozen opponent sources/binaries, 17 map files and seeds, and 68
opponent/map/seed/side pairing keys. The candidate source and binary hashes
match the independently reviewed R26 freeze:

- H14 main: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`
- R26 main: `660363800c8cce16b1717d0f9dbbdf9feba6e02083380ccb1b3e1a5e92cc06d4`
- R26 `headtrade.hpp`: `69bf74738cbf99f655eb9b715eec10f41b8946d184808b41fac28417c04ce4da`
- H14 binary: `98a8c546c653a6a9ae1aabdae284a22ef460ac3bc525bf842ed31223f3232c9f`
- R26 binary: `72a799b31a4fd26a67eebea5fa90fa1de8f692ae996d10d68a89e50e1223e1fa`
- Engine WASM: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`
- H14 `run.json`: `4ff193df2b5a302d7ea4f71b9d9f3196c4caaedfac4c947646cbebe5ebd54391`
- H14 `verified_audit.json`: `41459021d6d4a679a221cf4fffcde9d30ef8fc7ff32f613a8f47bd1c9445086a`
- R26 `run.json`: `8dc9649eec09d9ad143c9e2f0ce4de569abfb03f02f97898980b87df163fede3`
- R26 `verified_audit.json`: `b9bcbb8f94214a4f21fdb1fd93bded8307743884802c7d13a2ef7dba4bc79d2b`
- Paired comparison JSON: `c9dd13229e87b89e4281ddbe4233bab1d33e70850b1a285519cba467b474f191`

The raw standings are:

| Opponent/seat | H14 wins | R26 wins | Games |
|---|---:|---:|---:|
| V3_portal, all | 15 | 16 | 34 |
| V3_portal, A | 7 | 9 | 17 |
| V3_portal, B | 8 | 7 | 17 |
| xxqztest, all | 16 | 17 | 34 |
| xxqztest, A | 10 | 10 | 17 |
| xxqztest, B | 6 | 7 | 17 |
| **All games** | **31** | **33** | **68** |

There are no draws. The paired comparison has seven promotions from loss to
win and five regressions from win to non-win. Promotions are V3_portal
australia A, autarky A, slithery_fight B; and xxqztest australia A/B, maze B,
slithery_fight A. Regressions are V3_portal devil B and schooltime B; and
xxqztest autarky A/B and schooltime A.

The unchanged old-map subset is also retained as a negative slice: H14 is
V3 7/20 and xx 10/20, while R26 is V3 7/20 and xx 8/20. Thus the ranked-17
xx result is 17/34, exactly 50%, and the legacy xx subset declines by two wins.
This is a single deterministic development batch; the counts are descriptive
paired outcomes, with no significance or stability claim.

The result audit does not establish mechanism activation. The CLI log files
contain no `SHORTER_HEAD_TRADE` strings, which cannot be interpreted as zero
activations because committed actions and diagnostic output are not necessarily
retained in those logs. A separate full 68-game committed-action/prefix scan
is still pending. Expansion beyond this 68-game screen remains conditional on
that scan finding an interpretable R26 action mechanism; no confirmation or
robustness claim is authorized by this audit alone.
