# R14 Ready

Frozen 2026-10-05 after local verification. Frozen R12, H14, opponents,
engine and maps were not edited; no competitive matches were launched.
Production main differs from R12 only by the harvest safety declaration
and predicate. Added first-step proof resides in the reservation header.
Helper, configuration, queen feeding and consolidation are byte-identical.

Production build passed without warnings. Expanded source-bound R14,
frozen R12 and H14 fixtures passed. The fixtures show R12 selecting a blind
unresolved portal after reservation penalizes a known safe pearl pickup;
H14 and R14 retain the known surviving pickup through final action review.
They also distinguish an active reservation's free-harvest soft-risk guard.

Positive R12 reservation, current receiver-status verification, sole escape,
early/population/quiet/enemy/unknown-recipient-path rejection, unscored
priority, unchanged split/combat and exact no-receiver pickup controls pass.
Added controls reject hidden/unknown/dead-end admitted first steps,
incomplete own body and hard/soft friendly risk. A portal excluded by the
existing room claim does not unnecessarily disable reservation.

Commands from this directory:

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/robust_pearl_reserve_guarded_bot
g++-10 -std=c++20 -O2 -x c++ verification/pearl_reserve_tests.cpp.txt -o /tmp/robust_pearl_reserve_guarded_tests
/tmp/robust_pearl_reserve_guarded_tests
g++-10 -std=c++20 -O2 -DPEARL_RESERVE=0 -DPEARL_RESERVE_GUARDED=0 '-DBOT_SOURCE="../../tyhh_qos_counter_queen_guard_consolidate_v127/main.cpp"' -x c++ verification/pearl_reserve_tests.cpp.txt -o /tmp/robust_pearl_reserve_guarded_control
/tmp/robust_pearl_reserve_guarded_control
g++-10 -std=c++20 -O2 -DPEARL_RESERVE_GUARDED=0 '-DBOT_SOURCE="../../tyhh_qos_robust_pearl_reserve_v127/main.cpp"' -x c++ verification/pearl_reserve_tests.cpp.txt -o /tmp/robust_pearl_reserve_guarded_r12_control
/tmp/robust_pearl_reserve_guarded_r12_control
```

Final outputs:

```text
pearl reservation candidate assertions passed
pearl reservation H14 control assertions passed
pearl reservation candidate assertions passed
```

Included-main fixtures retain the parent's renamed-main no-return warning;
the production build is clean. These are local assertions, not judge budget
or competitive-performance evidence. Root owns independent audit, freezing,
paired evaluation and retention of negative results.

SHA-256:

```text
main.cpp                       0a780f40b8f4df0e0f45d18dc4f690c8b85aadea79a65a556eb39bbca8a53628
pearl_reserve.hpp              58ca67a84c96b714590978690ee7b94e590d70cdcd703fd7b1aaefabd42ee626
helper.hpp                     6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc
queen_feed.hpp                 0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24
consolidate.hpp                bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf
bot.toml                       d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5
pearl_reserve_tests.cpp.txt     87166f03e7e6f89f6614a99472bc1d52a1c1a477861e431e6f31fb9df32fb097
```

The conservative all-admitted-moves gate may suppress useful reservations.
Reservation proves local action safety, not a recipient's collection choice.
