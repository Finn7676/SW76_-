# R26 Ready

Production O2 compile passed. Source-bound independent controls and all
three inherited suites passed; independent review found no source blocker.
Actual-input native/official rule checks passed for both registered losses.
The dedicated source-bound candidate/H14 suite independently passes all72
controls, including actual-own duplicate and N/E/S/W pairwise tie ordering.
Source frozen2026-10-08 after required checks. No competitive matches yet;
root owns registered68game82001firstscreen. No further source edits.

Main differs from H14 only by a `headtrade.hpp` include and a final
`reviewShorterHeadTrade(ct)` call after all inherited reviews. The corrected
complete-visible body proof is adapted from R20. All other policy modules
remain byte-identical to H14; the final review's emitted MOVE may differ.

Native production command:
`g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_headtrade_v127/main.cpp -o /tmp/r26_headtrade_bot`.
Normal O2 build has no warnings. Extra warning-enabled review builds show
only the inherited unused enemyEscapes warning in both candidate and H14.

Canonical actual-input proof is `r26_candidate_native.json` and its
`r26_candidate_native_fixture.py` script under
`experiment_results/xxqz_robust_20261004`. Fresh native H14 emitsW, R26N
for default82003A id2r20 and Australia83001A id341r352. These decisions link
to the exact official1.2.7 N arms with equal3219/206298-event source prefixes,
raw inputs and predecessor-chain/tail-neighbor proofs. Native state is fresh;
recorded other-actor action/SONAR responses are held fixed, so this is local
rule evidence, not live or competitive survival/win evidence.

Independent verdict/provenance: `r26_independent_review.md` in that evidence
directory. No source edit or competition occurred during review. Hidden,
partial, portal, longer-route and intervening-actor uncertainty remains.

SHA256:
- main: `660363800c8cce16b1717d0f9dbbdf9feba6e02083380ccb1b3e1a5e92cc06d4`
- module: `69bf74738cbf99f655eb9b715eec10f41b8946d184808b41fac28417c04ce4da`
- binary: `72a799b31a4fd26a67eebea5fa90fa1de8f692ae996d10d68a89e50e1223e1fa`
- helper: `6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`
- TOML: `d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`
- feed: `0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`
- consolidation: `bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`
- focused tests: `3f8d8fb15afac7f92cb3e9917290a37683e3ad60993a04cc99cad7bebe4beb3c`
