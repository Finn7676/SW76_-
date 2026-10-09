# R5 Ready

Source frozen after native C++20/O2 build and four source-bound regression
binaries passed. Root owns competitive evaluation; no matches launched here.
Only the routine nonqueen expansion gate differs from frozen H14. H14 itself
and all opponents were not modified.

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_splitguard_v127/main.cpp -o /tmp/robust_splitguard_bot
```

Main SHA-256:
`136e243e03fa6d1a099e4dac00e2c7263c1a9faea987142f548c23af9a08f0ae`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
bot.toml SHA-256:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`.
queen_feed.hpp SHA-256:
`0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`.
consolidate.hpp SHA-256:
`bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`.
Native validation binary SHA-256:
`1cc71affc02e6ab7dad6536c2c78b9c43ed3dacbb7f1e896322ce2e5b71aae61`.

Non-main module hashes match H14. The ranked-match runner will compile its own
frozen candidate binary and record that runtime hash independently.
