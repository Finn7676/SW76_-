# H10 Ready

Only the ordinary expansion threshold changes470 ->350 relative to guard
parent. The final guard functions remain unchanged. No feed or margin code
is added. Root owns whole-tree freeze and evaluation; no matches launched.

Verification:
```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_counter_queen_guard_mature350_v127/main.cpp -o /tmp/queen_guard_mature350_bot
```

Normal native build passed cleanly. Independent diff confirms one threshold
change and unchanged guard functions. Source edits stop after this record.

Main SHA-256:
`3b8b2bb96dd0511ad0e8ca856b5356a75f062aa65391f11ade2f79825c6d98de`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
bot.toml SHA-256:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`.
