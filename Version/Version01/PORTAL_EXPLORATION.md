# Team portal exploration — 2026-09-25

This is an intentional strategy change after the behavior-preserving refactor.

## Policy

A portal pair is unexplored until this dragon confirms a successful crossing on its next turn, or receives an explored-portal report from this bot's team protocol. Merely seeing an entrance does not mark it explored. Both endpoints share status by portal ID. Move and known-route sprint crossings are tracked; a different dragon ID or mismatched sprint landing cannot confirm an old action.

After sacrifice, splitting, ramming and contested-pearl sprint decisions, eligible dragons route toward the nearest reachable unexplored portal. The edge-goal BFS explores at most 320 cells to depth 24 through known open edges and resolved portals. It never invents a route through an unknown exit. Late-game champions and dragons with visible enemy heads keep the existing policy. Length-2 dragons may explore and cross portals with a normal one-step move. Known immediate destinations must pass occupancy, onward-exit and space checks. Crossing an unresolved portal intentionally accepts uncertainty about the far side.

Once explored, a portal has no portal-specific movement bonus or scouting priority. Ordinary occupancy, topology, pearl, space and unseen-cell scoring still apply. A portal can remain unresolved locally until the other endpoint or landing becomes known.

## Sonar protocol 3

Official references checked 2026-09-25:
- https://game.battlecode.au/docs/sonar
- https://game.battlecode.au/docs/protocol-upgrade

The existing helper reads uint64 messages and optional ECHOES; main.cpp requests PROTOCOL 3. Sonar sends the selected report in N/E/S/W. Enemy and pearl wire formats are unchanged.

Portal reports now carry a complete connection atomically: x1/y1/x2/y2 (6 bits each), portal ID (13 bits), shared canonical edge orientation E/S (1 bit), explored flag (1 bit), team (1 bit), format marker 0xB7 (8 bits), and checksum (16 bits). Both ends are sorted by canonical edge key so every relay encodes the same pair. The original 64-coordinate and 8192-ID limits remain. No report is sent until exactly two valid matching endpoints are known. Kind-2/3 legacy single-endpoint reports are ignored.

Before either endpoint is written, the receiver checks checksum, team, bounds, canonical ordering, conflicting known terrain and existing portal endpoints. One accepted report establishes traversal from both sides of both edges. The explored flag remains separate: observing both ends alone does not imply a successful crossing. Duplicate beam receipts are coalesced. Markers/checksums identify the protocol and detect corruption; they do not authenticate a sender against intentional spoofing.

Existing urgent-crossing and periodic relay scheduling remains. Messages can be lost or intercepted, so sharing is eventual rather than global instant memory. No new scouting roles, reservations, enemy reports, echo interpretation or movement policy were added.

## Verification

- Clang C++20 optimized builds, warnings as errors, with exploration enabled and disabled.
- Original update: 22 focused tests passed with AddressSanitizer and UndefinedBehaviorSanitizer: routing, crossings, sprint confirmation, length restrictions, blocked landing, sonar encode/decode, team filtering, duplicate receipt, relay and explored-portal behavior.
- End-to-end native protocol fixture passed: initial legacy-format input, protocol-3 negotiation, ECHOES/64-bit messages, four directional outputs, and a second dragon skipping a reported portal.
- Before the length-2 correction, exploration disabled: 2,051 identical MOVE/SPLIT decisions against the saved baseline on arena and queen_of_spades using toolkit 0.3.6, with sonar suppressed in both to compare movement on identical inputs. This is not a sonar or current-engine strength comparison.
- A current toolkit download was declined. Full matches under the latest engine and win-rate/compute-budget measurements remain unverified.

Run focused tests from this directory (test source uses .txt so submission does not compile a second main):

```sh
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I. -x c++ verification/portal_tests.cpp.txt -o /tmp/xxqz-portal-tests
/tmp/xxqz-portal-tests
clang++ -std=c++20 -O2 -Wall -Wextra -Werror main.cpp -o /tmp/xxqz-portal-bot
python3 verification/protocol_test.py /tmp/xxqz-portal-bot
```

Set FEAT_PORTAL_EXPLORE=0 to disable scouting/explored-status reporting and restore the old portal movement bonus. Protocol-3 directional sonar remains enabled.

## Length-2 rule correction

Removed length-based portal exclusions from ordinary scoring, routing, exploration and pearl sprints. Removed the unsupported split restriction on producing a length-2 dragon spanning a portal, plus its unused helper methods. Existing sprint affordability guards, collision checks and `ct.can_split` checks remain.

Regression coverage now includes length-2 exploration, known-portal routing, normal portal scoring, occupied exits, portal-spanning splits and sprint exclusion. The current official movement/execution rules distinguish additional sprint steps from ordinary portal crossings:
https://game.battlecode.au/docs/execution-order

Validation: 27 focused tests under AddressSanitizer/UndefinedBehaviorSanitizer, optimized builds with exploration on/off, and the existing protocol-3 integration fixture. No latest-engine match-strength claim is made.

## Atomic portal sonar update

Only sonar code, verification fixtures and these notes changed. Movement, exploration, splitting and sprint policy files are byte-identical to the preceding version. Better received topology can still affect resulting paths.

Validation: 33 tests passed with AddressSanitizer/UndefinedBehaviorSanitizer, including complete pair relay, both orientations, conflict rejection without partial writes, every single-bit corruption, legacy rejection and unchanged pearl/enemy frames. Protocol-3 input/output integration passed. Optimized builds passed with exploration enabled and disabled. Latest-engine matches and win-rate improvements are not established by these checks.

## Compatibility with the installed benchmark engine

The installed `unswbc 0.3.6` engine used by the local benchmark rejected `PROTOCOL 3` and `SONAR N/E/S/W <64-bit value>` lines in the supplied replays. The helper now records whether an `ECHOES` line actually appeared. Until that happens, the bot sends only supported 32-bit legacy pearl/enemy reports using `SONAR <value>`. When a protocol-3 `ECHOES` line appears, it switches to the complete 64-bit portal-pair report and four directional beams. It still requests protocol 3 on the first turn so a current engine can upgrade it.

The old engine cannot receive one-message complete portal pairs. No portal report is sent through the legacy fallback; it does not fabricate a partial connection. One rejected `PROTOCOL 3` command may still appear per dragon in old-engine logs, but movement and legacy sonar commands remain valid. This adjustment changes only sonar transport compatibility, not movement policy.

Local verification of this compatibility fix: 35 focused tests passed under AddressSanitizer/UndefinedBehaviorSanitizer; the synthetic protocol-3 fixture passed. In a native `unswbc 0.3.6` arena match, the bot emitted 1,590 accepted legacy-form sonar lines and zero directional lines; the old engine still logged one rejected `PROTOCOL 3` line per spawned dragon. On the default map against `/Users/fanlingjun/Desktop/xxqz/main_no_mapdb.cpp`, both the pre-fix bot and this compatibility candidate lost from both A and B in one paired comparison. This does not establish a competitive improvement; it isolates the transport incompatibility from the existing strategy weakness.
