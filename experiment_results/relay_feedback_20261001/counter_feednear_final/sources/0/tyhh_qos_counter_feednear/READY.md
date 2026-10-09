# READY: feednear

Frozen for root scheduling. No competitive matches were run by this agent.

Native compile (Clang C++20) passed. Focused ASan/UBSan policy test passed:
`feednear policy boundaries passed`. Warnings are inherited unused
`enemyEscapes`, unused `crownAllowExpansion`, and the included bot test's
renamed-main return warning; no sanitizer diagnostics occurred.

Source hashes:

```
c41cf6e028b766296c28835d0b289c503b66cdd9c9f64be898734fdb9700285d  main.cpp
9bf906055bcf440bc196a73cdc17fb2a8290a1da08028d616ae3c08e05157204  feednear.hpp
6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc  helper.hpp
d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5  bot.toml
```

Validation binaries (reproducible paths): `/tmp/feednear` and
`/tmp/feednear_tests`.
