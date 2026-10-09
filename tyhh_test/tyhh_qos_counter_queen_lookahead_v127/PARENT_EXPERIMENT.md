# Historical Parent Record: NOT Results of Counter Queen

Registered 2026-09-29 before candidate match results. Parent is the unchanged
tyhh_qos_protect_relay (main.cpp SHA-256
c65cfa425e09b38662610fd19fc309b419e5c82edc2c6e7cddadeab8337b4961).

Single intervention: replace the unconditional proactive length-two head ram
with a gate requiring at least three currently observed enemy segments and
more than one surviving teammate. Observed size is a lower bound: fewer than
three visible segments is insufficient evidence of a trade-up, not proof that
the enemy is short. A length-two ID-selected champion can still make this
profitable trade. When no safe exit exists the original ram fallback remains.

No changes to portal decisions, sonar, score weights, sprint planning, split
policy, champion timing, donation, or teammate protection. This is not the old
long-dragon antiram module. The altered gate is a single proactive trade
selection mechanism; its size and last-unit clauses are not independently
causally identified in this experiment.

Evidence: baseline_dev on seed 73001 completed 40 games (V3 7/20 wins, xxqztest
6/20). Four xxqztest loss slices on Queen Of Spades and Trophy contain 147
relay head-collision deaths out of 176 total deaths, reconstructed with split,
length conservation and final standings checks. These aggregate deaths do not
identify who initiated each collision or prove equal-size trades caused the
losses. Source evidence is more specific: xxqztest/planner.hpp adjacentRam
requires visible size >=3 and population >1; relay bypasses both for length 2.

Prediction: fewer voluntary length-two trades with no observed size advantage
will preserve resource collectors against the xxqz swarm while retaining
immediate attacks on larger heads. Counter-hypothesis: equal trades may deny
valuable enemy collectors or create food for allies; abstaining could weaken
territorial combat, including against V3. Test against both unchanged bots.

Validation: verification/counter_tests.cpp.txt tests equal/uncertain trades,
profitable trades, the last/penultimate unit, ID-selected short champions,
non-applicability to longer dragons and the unchanged trapped fallback.
Native C++20/O2 compilation and ASan/UBSan assertions passed; exact commands,
inherited warning notes and binary hashes are in verification/validation.txt.
Root alone schedules full matches: development seeds 73001/73002, ten official
maps, both sides, identical opponents. No seed 74001+ data was read or used.

## Inherited relay record — not results of this counter candidate

The following copied provenance describes the parent only. Its historical
validation and unchanged-combat statements must not be attributed to H3.

# Portal communication / claim experiment

Derived from `tyhh_qos_protect_route`, without modifying the original directory.
Movement scores, exploration parameters, combat, split thresholds, and the
local teammate protection review remain unchanged.

Hypothesis: the baseline's persistent last-crossed-portal claim monopolizes
all four sonar beams and reserves ordinary connected terrain as if it were a
small resource room. This suppresses topology and resource propagation.

This variant claims a room only when a flood fill using known open edges
proves a component of at most 24 cells, exactly one canonical portal boundary,
and a known pearl source. Unknown edges or another portal boundary prevent a
claim. The proof is evaluated at the current and projected post-action head;
an unresolved landing does not produce a claim. Received claims retain the
baseline's six-round expiry.

Complete portal endpoint pairs use a team-tagged, checksummed 64-bit message.
One beam carries ordinary information, another a rotating complete portal
pair, another a proven room claim, and another verified-state-format status
when available. Unused beams repeat ordinary information. The beam assignment
rotates every turn. Thus a claimed room cannot suppress all other information.
The legacy single-end portal encoding is limited to IDs below 4096, its actual
12-bit payload capacity; complete pair messages support all 13-bit IDs.

`verification/relay_tests.cpp.txt` checks claim evidence, rejection of unknown
or oversized regions, complete-pair round trips including high portal IDs,
team and checksum validation, and rejection of conflicting map information.
Match results are recorded separately by the experiment runner; no performance
claim is made before matched-seed, side-balanced evaluation.

Validation completed: native `clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic`
build passed (one inherited unused `enemyEscapes` warning); all relay/room
assertions passed under AddressSanitizer and UndefinedBehaviorSanitizer, with
leak detection disabled for the sandbox. The test include renames the bot
`main`, producing an inherited missing-return warning in that unused function.

## Frozen evaluation outcome

Development: 11 wins / 9 losses on seed 20260927, the ten official maps, and
both sides. Eight development candidates were retained and compared. Relay
was selected under the recorded tie rule and frozen before final evaluation.

Final matched evaluation: 25 wins / 35 losses (41.7%) against V3_portal; the
unchanged route baseline scored 17 wins / 43 losses (28.3%) under the same
seeds 20261011, 20261012, 20261013, ten maps and both sides. These integers are
random seeds, not experiment dates. Thirteen paired conditions changed from
loss to win and five from win to loss. No draws or run failures were reported
in either 60-game suite. This improves the matched sample result but does not
establish a winning overall record against V3.

Per-seed wins are 6/9/10 for relay and 6/7/4 for route. These are descriptive
results over three seeds, not a significance test. Autarky and Slithery Fight
each lost one win relative to route; Default remained 0/6. Production sources
were not changed after final evaluation.

Full evidence, protocol correction, replays, source snapshots and verification
are under ../../experiment_results/protect_vs_v3_20260928/. See REPORT.md and
final_summary.json there. Do not combine development and final cohorts, or
tune on these final seeds while describing them as held-out.

Audit clarification: the V3 opponent had 11 no-valid-action deaths in five
relay-suite games and seven in six baseline-suite games, all on Portals.
Neither relay nor route had such deaths. V3 intentionally supports ActionType
NONE for self-sacrifice / teammate-head protection; its tests expect NONE in
these scenarios. This engine death reason alone is not evidence of a crash or
failed match. Each recorded death has not been traced to its exact decision
branch. All completed outcomes remain included without reweighting.

Final artifact integrity and runtime-failure checks pass. The stronger audit
flag requiring zero no-action deaths on both teams remains false and is
retained as such. See final_verification.json and REPORT.md for the distinction.
