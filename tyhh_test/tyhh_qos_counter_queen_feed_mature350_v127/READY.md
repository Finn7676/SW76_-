# H9 Ready

H9 consists only of the unchanged feeding parent plus the expansion threshold
constant470 ->350. Queen bootstrap and population fallback remain untouched.
No prior candidate guards or margin modules are included.

Verification command:
```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_counter_queen_feed_mature350_v127/main.cpp -o /tmp/queen_feed_mature350_bot
```
Root owns whole-tree freeze and evaluation. No matches launched here.

Native compilation passed cleanly. Independent source diff confirms exactly
one threshold constant changed against the feeding parent. Source edits stop
after this registration.

Main SHA-256:
`49d488532bab19474214cc139f5a02e4bbca5881cfd72a05b9daf6c28b9a7824`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
bot.toml SHA-256:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`.
queen_feed.hpp SHA-256:
`0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`.
