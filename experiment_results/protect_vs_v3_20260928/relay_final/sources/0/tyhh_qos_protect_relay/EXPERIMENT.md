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
