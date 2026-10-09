# R3 Ready

Frozen 2026-10-04 after local tests. Parent H14 is unchanged. No matches launched.
Native main build succeeded; focused route and inherited regression tests
passed. Root owns competitive evaluation and retention of all negative results.

Source SHA-256:

```text
main.cpp             91a65ac8a9fb7656f1631d5a0ebda7c1905e7bdafc2ee66240b8b081f398ecee
donor_route.hpp      59088c9202afe8e4e942c9dcdeb1b03a3cb9a2f9619a1ac3839d94a5252ebf99
helper.hpp           6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc
bot.toml             d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5
queen_feed.hpp       0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24
consolidate.hpp      bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf
donor_route tests    71c306742c3cbdc75bae8f61f79d1969d6212cb377369c273845e3907ec37dd8
```

Compilation:

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_donor_route_v127/main.cpp -o /tmp/robust_donor_route_bot_v127
```

Known safety contract: ordinary E_OPEN visible route, no occupied traversal,
depth 6 / 128 expanded nodes, current verified visible recipient heads only,
exact safe first-step projection and no direct/hard/soft friendly-exit risk.
Original donation eligibility, competitor review and receiver hold are unchanged.
