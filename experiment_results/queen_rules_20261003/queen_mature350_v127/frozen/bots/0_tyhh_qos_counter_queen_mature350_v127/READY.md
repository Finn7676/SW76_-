# H8 Ready

H8 source is frozen after the one-line experiment change. No matches have
been launched. Root owns whole-tree freeze and suite scheduling.

The only source change against `tyhh_qos_counter_queen_v127/main.cpp` is:
`ROUND < 470` -> `ROUND < 350` in the ordinary non-champion `wantExpand`
branch. Queen bootstrap `< 120` and population fallback `< 8` are unchanged.

Verification:
```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_counter_queen_mature350_v127/main.cpp -o /tmp/queen_mature350_bot
```
Standalone compilation passed with no output and exit0. Independent
`git diff --no-index` confirms exactly one changed constant. No performance
claim is made before matched evaluation.

Main SHA-256:
`19d45e529d689c93e62951c4da0438f9f8501bd358c1d0d301db6e2eb1feb585`.
Helper SHA-256:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`.
bot.toml SHA-256:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`.
