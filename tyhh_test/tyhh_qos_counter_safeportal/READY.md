# H1 safeportal candidate — frozen

Parent: `tyhh_test/tyhh_qos_relay_counter`.
No competitive match was run before this freeze. Only the registered H1
safe-portal gate was changed; short-dragon exploration and counter combat,
sacrifice, scoring, split, sonar, and protection logic are inherited.

The gate applies at length >= 8 and is checked in earlyPortal, scored portal
selection, and final `reviewProtection` (including tactical and emergency
fallback proposals). A landing must be confirmed in `learnedExit` by this
dragon, visible, free, and have usable continuation/space. Logs use
`SAFEPORTAL <round> <id> reject <dir> <reason>`, `allow`, `reroute`, `split`,
or `trapped_exception`; the last tag records the unavoidable all-portal
liveness boundary and does not claim that crossing is safe.

Validation commands:

```sh
PATH=/tmp/clang-toolchain:$PATH clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic tyhh_test/tyhh_qos_counter_safeportal/main.cpp -o /tmp/tyhh_qos_counter_safeportal_native
PATH=/tmp/clang-toolchain:$PATH clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -x c++ tyhh_test/tyhh_qos_counter_safeportal/verification/safeportal_tests.cpp.txt -o /tmp/safeportal_tests
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/safeportal_tests
```

Focused boundary assertions passed. The native build has one inherited
unused-function warning (`enemyEscapes`); no compiler or sanitizer error.

Frozen source hashes:

```text
main.cpp  e1ad101e4d06d8eb7d81173b5b262ce450d1e14202b7509bdd88048bb2cfdcbf
helper.hpp 6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc
bot.toml d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5
native /tmp/tyhh_qos_counter_safeportal_native 7e34d277d7c7ecd455550661c82a0868678f5f8b7136cc6f566e47d54ee230ef
test source sha256 5e6d3ea9f4506ce5fc35625db3ef4caeae13d73f7058313de679b3e20fe53415
test /tmp/safeportal_tests 1b957ca3284b2678ab851d0a2de27691a663d54215614cad2a06943ea3ae5fb6

The shared candidate later received additive unresolved/fallback guards from
the root coordinator. Current main SHA-256 is
`3bc4c82b080574f7691325751c4fec874f09a200cb303ae82a1791889d399e86`; rerun
the source hash at suite launch. The focused test was rerun on that tree and
passed (current test binary SHA-256
`b6223b6dae1f96c9a5c0a87040b718090ec17c46bf652f702d88e102cb142a72`).
```
