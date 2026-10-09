# R2 Ready

Frozen isolated receiver-hold factor, parent H14. No matches launched.
Parent main/helper/queen_feed/bot.toml are byte-identical; the only source
difference changes `consolidateHoldUntil = ROUND + 12` to
`consolidateHoldUntil = Constants::MAX_ROUNDS` in consolidate.hpp.

Native g++-10 C++20 O2 build passed cleanly. The source-bound receiver-hold
fixture passed 13 checks, including observer-ID reset, queen exclusion,
late eligibility, persistence through round 500, ordinary split suppression,
and emergency split availability. See verification/validation.txt for commands.
No victory or resource-budget claim is made.

## Frozen SHA-256

main.cpp:
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`

helper.hpp:
`6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc`

bot.toml:
`d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5`

queen_feed.hpp:
`0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24`

consolidate.hpp:
`217c7312fe17c42d97ecd7683fe469ea95a46404c70b584f4e8b6453d5c4fa38`

verification/receiver_hold_tests.cpp.txt:
`1e322ad34ca3e748cff4115c3e6f04ff7155d5ee9dd955c02780e832122c7a75`
