# H11 Ready

H11 contains the unchanged H5 consolidation/recipient-hold implementation
plus only the normal expansion threshold470 ->350. No other candidate
guards or margin factors are added. Root owns whole-tree freeze and matches.

Verification:
```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_counter_queen_consolidate_mature350_v127/main.cpp -o /tmp/queen_consolidate_mature350_bot
```
No matches have been launched here.

Native g++-10 build passed cleanly. Independent diff confirms the sole source
change is the expansion threshold. Source edits stop after this record.

Main SHA-256:
`42f41abffe36a7a4f637c73e6c1a164eaad86c83415a71ba31449a0de0856450`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
bot.toml SHA-256:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`.
queen_feed.hpp SHA-256:
`0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`.
consolidate.hpp SHA-256:
`bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf`.
