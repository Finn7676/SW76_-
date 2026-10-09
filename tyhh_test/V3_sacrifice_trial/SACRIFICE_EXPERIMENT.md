# Quiet-sonar donation experiment

This is a separate V3 variant. The original `V3` remains the benchmark.

From round 400 onward, a non-Champion, non-Scout dragon of length at most 6
may donate itself when its team has at least 12 dragons, it has heard or seen
no enemy in the last 18 rounds, and the visible head of a teammate is within
three map steps. A recent valid sonar report from a teammate that has itself
gone at least 18 rounds without enemy intel reduces the donor's own silence
requirement to 7 rounds. The teammate must have at least 10 observed segments and at
least four more observed segments than the donor. The donor outputs no action;
the engine kills only the donor and turns alternating body segments into
pearls. There is no attempt to move into the recipient's head or body.

Enemy intel includes any locally visible enemy segment and valid teammate
enemy sonar reports. Missing sonar is only an absence of evidence; it cannot
prove the enemy is absent. The dying donor's own sonar is not sent on that turn.

Protocol 3 periodically sends a 64-bit enemy-history status in the ordinary
sonar slot. It contains the sender ID, report round, last enemy-intel round,
team, a quiet-eligibility bit, and checksum. The eligibility bit is true only
after the sender has been alive and without enemy intel for at least 18
rounds; newly split dragons cannot claim this immediately. A recipient
ignores its own echoes, wrong-team messages, damaged packets, and reports
older than one round. A qualifying teammate report remains usable for three
rounds. Any more recent enemy intel still prevents premature donation. This
status shares ordinary sonar time with pearl and portal messages; crossed
portal pairs and active claims keep their reserved beams. Protocol 2 retains
its existing fallback and cannot enable the 7-round threshold.
The bot requests Protocol 3 on every turn so a current engine can negotiate
directional 64-bit sonar after the first turn.

`V3_SACRIFICE_START_ROUND`, `V3_SACRIFICE_QUIET_ROUNDS`, and
`V3_SACRIFICE_MIN_ALLY_LENGTH` are tunable
compile-time constants. The original `FEAT_SACRIFICE=0` disables the branch.

## Initial local comparison

The following comparison was run with the earlier 12-round quiet window,
before it was raised to 18 or enemy-history sonar was added. It does not
measure the current strategy.
Against unchanged V3 on fixed bundled maps, swapping sides each time:

| Map | Trial as A | Trial as B | Explicit no-action deaths |
| --- | --- | --- | --- |
| stronghold | loss | loss | 0 / 0 |
| big_empty | loss | win | 4 / 17 |
| default | loss | win | 0 / 2 |
| queen_of_spades | win | loss | 1 / 0 |

Across these eight games, the 400-round variant had the same 3-5 result as the
earlier 250-round variant, with fewer no-action deaths on big_empty. In two
direct big_empty games against the 250-round variant, swapping sides, the
400-round variant won both: longest dragons 36 vs 33 and 36 vs 34. This small
sample favors the later start but does not establish a general win-rate gain.
The branch remains separate from V3.

## Current protocol check

The machine's system `unswbc` 0.3.6 predates Protocol 3 and only ran the
legacy 32-bit fallback. A temporary `unswbc` 1.0.0 environment was used for
the new feature check. In a `big_empty` replay, history packets were sent
and valid 18-round quiet packets reached teammates. Two games against an
otherwise unchanged 18-round variant that also requests Protocol 3 ended
with this variant winning on both sides, but neither game triggered a planned
no-action donation. These games confirm the wire path, not the competitive
value of the 7-round override. Further tests on maps where a donor actually
qualifies are needed before treating the win rate as improved.

Build and focused check:

```sh
clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror main.cpp -o /tmp/xxqz-v3-donation
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I. -x c++ verification/sacrifice.cpp.txt -o /tmp/xxqz-v3-donation-check
/tmp/xxqz-v3-donation-check
```
