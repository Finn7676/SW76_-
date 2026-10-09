# R11 Ready

Parent R3 remains unchanged; no matches launched. Production main has only
the new header include, observation call and post-R3 navigation fallback.
Helper, config, queen feeding, consolidation and visible donor routing are
byte-identical to frozen R3. Cached status does not enter `friendLength`,
occupancy or donation acceptance. No source edit follows this record.

Production build passed without warnings:

```bash
g++-10 -std=c++20 -O2 tyhh_test/tyhh_qos_robust_remote_rally_v127/main.cpp -o /tmp/robust_remote_rally_bot
```

Focused candidate and R3-control tests and inherited donor routing passed:

```bash
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_remote_rally_v127/verification/remote_rally_tests.cpp.txt -o /tmp/robust_remote_rally_tests
/tmp/robust_remote_rally_tests
g++-10 -std=c++20 -O2 -DREMOTE_RALLY=0 '-DBOT_SOURCE="../../tyhh_qos_robust_donor_route_v127/main.cpp"' -x c++ tyhh_test/tyhh_qos_robust_remote_rally_v127/verification/remote_rally_tests.cpp.txt -o /tmp/robust_remote_rally_control_tests
/tmp/robust_remote_rally_control_tests
g++-10 -std=c++20 -O2 -x c++ tyhh_test/tyhh_qos_robust_remote_rally_v127/verification/donor_route_tests.cpp.txt -o /tmp/robust_remote_rally_donor_regression
/tmp/robust_remote_rally_donor_regression
```

Final output lines:

```text
remote_rally: candidate cue-only navigation, safety, freshness, progress and fallback assertions passed
remote_rally: R3 control cue-only navigation, safety, freshness, progress and fallback assertions passed
donor_route: 24 routing and exclusion assertions passed
```

Candidate and parent tests distinguish hidden status navigation from R3's
unchanged split fallback. Hidden reports never authorize suicide or current
friend-length validation. Tests cover team/stale decoding, TTL six versus
seven rounds, visible report contradictions, queen/population/round/enemy/
length/lower-ID gates, unknown edges and portals, hidden/occupied first
steps, incomplete body, absent continuation, hard/soft friend risk, resolved
short-donor portals, protected unsafe portals, length-two single-step
survival, six-round no-progress ignore, 12-round retry, and progress refresh.
The included-main fixtures emit the inherited renamed-main no-return
warning; production compilation is warning-free.

SHA-256:

```text
main.cpp                    56515c1f9519a8c1583fd61abff2725b3ba2ba48cee477ccc6e35f7620b9fbe0
remote_rally.hpp            7fb1f1d3ddc6e99b21ef551e2c63c49d9fbc36b10478364deda7325478278d57
helper.hpp                  6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc
bot.toml                    d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5
queen_feed.hpp              0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24
consolidate.hpp             bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf
donor_route.hpp             59088c9202afe8e4e942c9dcdeb1b03a3cb9a2f9619a1ac3839d94a5252ebf99
remote_rally_tests.cpp.txt  cd9a86317708bdcfdaa207ad30b34f11ce08d270799e2c080ad11f0cd9ff3151
donor_route_tests.cpp.txt   71c306742c3cbdc75bae8f61f79d1969d6212cb377369c273845e3907ec37dd8
```

Root owns source freeze, independent review, paired evaluation, retention of
negative results and performance claims. Six-round reports are approximate
navigation hints; stale target motion and recipient death remain possible.
