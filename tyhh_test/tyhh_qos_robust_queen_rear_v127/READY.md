# R22 Ready

Frozen 2026-10-06 after production and focused verification. Only the five
R21 ordinary-allocation lines were added to frozen R10; no other production
source changes. Helper, feed, consolidate and TOML are byte-identical to R10.
The independent no-blocker source review is recorded in
`experiment_results/xxqz_robust_20261004/r22_independent_review.md`.
The 82001 competitive screen completed 68/68 and passed the official audit:
V3 19/34, xx17/34. Relative to R10, xx has one gained and three lost wins.
This is a retained negative interaction, not a qualified finalist. No further
development or confirmation bases are justified by this screen.

Production R22 and R10 builds with g++-10 C++20 -O2 passed without warnings.
The17 rear_cascade source cases passed on both R22 and unchanged R10 control.
Inherited sprint4 paid/free/body eligibility tests and queen_hunt tests,
including200 frozen ordinary-target controls, passed on R22. Include-based
renamed-main harnesses alone emit the inherited no-return warning.

Official unswbc1.2.7 native fixtures passed4/4 with engine SHA256
26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546.
R22 LEN7 commits children5,3; LEN14 children12,10,8,6,4,2 in round0. R10
controls commit only child2. Actual split arrays and native last-child MOVE
are verified with no round0 deaths. The no-pearl/all-open modified fixture
terminates round1 deliberately; it proves scheduling, not competitive gains.

Raw fixture evidence:
experiment_results/queen_rules_20261003/verification/r22_rear_cascade_native.json
SHA25613bbe9ffb88a693cfed7a9631144b9f242bd1752768881d5a4cec69594708abb.

```bash
g++-10 -std=c++20 -O2 main.cpp -o /tmp/r22_bot
g++-10 -std=c++20 -O2 -x c++ verification/rear_cascade_tests.cpp.txt -o /tmp/r22_cascade_tests
g++-10 -std=c++20 -O2 -DREAR_CASCADE=0 '-DBOT_SOURCE="../../tyhh_qos_robust_queen_hunt_v127/main.cpp"' -x c++ verification/rear_cascade_tests.cpp.txt -o /tmp/r22_cascade_control
.venv-battlecode/bin/python tyhh_test/tyhh_qos_robust_queen_rear_v127/verification/rear_cascade_native.py --candidate /tmp/r22_bot --control /tmp/r22_r10_control --output /tmp/r22_native_fresh.json
```

Risks remain: LEN2 parents cannot use the inherited R10 LEN>=3 sprint gate;
cap/crowding and reversed exploration can change hunts; inherited splitSafe
may accept unavailable neck evidence and does not prove surviving parent or
child. No resource/win/sandbox-budget superiority is established. R10 attack
package and rear allocation form the two factors; this is not pure queen
valuation. No confirmation seed is inspected.

## SHA256

- main:43ed3e523628fb34f74279943f80408f149353d5a3440a6fbadf73cfe4ff8138
- helper:6bd3b258ad4db850bd209279db0d8a836e3ed4fb1589190cfeda0012bbae7bcc
- feed:0f8ba15a48c506bf3b892a28259903a9eea6137d822ca0baff5c063db644db24
- consolidate:bbf0ab3334219c6914cd14a85a60157c3d0697fb16d54e70069719a818905bdf
- TOML:d56d20b38e5961083cba1d7e00ff47397f567bf55c2b4a4748ac0f32fcba9cd5
- native R22:2ae55dbec7cb7860fb22f682847488380c8e5d7839e8d09d64de903ea147926e
- native R10:9c417c402e5742764a5a425571cf51283690d2c41b165b68ed13c1974f2671b7
