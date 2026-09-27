# V3 portal and role experiment

V3 starts from the frozen V2 source. It changes strategy; no win-rate gain is assumed. `helper.hpp` and `bot.toml` are unchanged. The original Champion, Core, Fragile, and Attrition gates remain in force. Donation eligibility in the V4 baseline uses relative receiver size and quiet-phase population reserves; it no longer protects dragons solely by Champion ID.

## Roles and portal decisions

- A Scout can have length **2–8** when population is at least 6, there is a locally known but team-unexplored portal, and no enemy head is visible. The assignment stops after round 219 on maps below 1,200 cells or after round 299 on larger maps. Champion, Core, and active Hunter labels take precedence. There is no fixed ID quota.
- A short Hunter still needs length at most 5, population above 6, and no Champion label. In V3 it also needs a visible larger target or a recent nearby sonar sighting. This leaves a short dragon free to Scout when no hunt is available.
- An active Scout searches up to six known steps for an unexplored portal and defers its normal expansion split while following that route. Combat and contested-pearl sprint decisions keep their earlier priority. Unknown portal exits are goals, never assumed corridors.
- Before following a portal route, the Scout checks visible, reachable pearls within four known steps. It temporarily picks the pearl if it is adjacent, no farther away than the portal, or part of a group of at least two reachable pearls with a detour of at most three steps relative to the portal. It resumes portal scouting after the pearls are collected. Portal, nearby-pearl, and ordinary pearl/unseen-cell BFS searches now compare all equal-distance routes within their existing depth/node limits: first by visible pearls collected on the path, then by additive tile scores for new cells (+130), nearby friendly parts (-35 each), and narrow exits (-80). BFS retains the best equal-distance path to each cell rather than enumerating paths individually. Dynamic one-step terms such as controlled space and current pearl distance remain in ordinary move scoring. Scout searches use the existing first-step safety gate and replan each turn; ordinary routing still provides a direction bonus to normal move scoring rather than forcing its chosen step.
- A Scout broadcasting an active route sends a short-lived portal-ID claim. If another Scout hears it, the lowest still-live dragon ID keeps the claim until its report is more than two rounds old; a newer higher-ID message no longer steals it. Visible nearby friendly heads also yield to a teammate that is closer to the entrance, with lower ID breaking distance ties. This is coordination, not a lock; simultaneous claims, lost sonar, or teammates outside vision can still cause duplication.
- A portal becomes explored after this dragon confirms crossing it, after a teammate's complete explored-pair sonar report, or when an observed teammate **body segment points through the portal edge toward its head**. A teammate merely standing beside the entrance is not enough evidence. Once explored, the portal leaves the Scout target set and receives only a small **+18** ordinary-move score bonus. The usual occupancy and space checks still apply. The body evidence marks local status immediately; a complete entrance/exit report still waits until both endpoints are known.

## Safety and pocket economics

Before entering a known small one-entrance pocket, the bot checks its observed edges and counts visible pearls inside. A cycle, unknown edge, portal, or more than 24 cells makes the classification uncertain rather than triggering a hard veto. A proven pocket with one pearl is normally avoided. Three or more pearls can justify entry only when the team has room to split and the tail has a plausible escape. Once inside with no alternative safe move, the dragon may finish the remaining pearls and use the existing reverse-tail emergency split when trapped. Profitable pockets receive a bounded score bonus; the split is a salvage tactic, not proof that the original head survives.

Known teammate-head destinations are excluded from sacrifice, planned moves, sprints, and fallback. A final action guard catches any missed known teammate head; if no legal alternative or split exists, the bot emits no action and only that dragon dies. Unknown portal exits and teammate movement later in the round remain inherently uncertain.

At every own length, a dragon discourages moves close to a visible enemy head whose exact body length is at least **three segments shorter** (enemy length at most own length minus 3). If only a partial body is visible, an observed length no greater than that threshold is treated conservatively as possibly short. The extra normal-move penalties are 1,200 within one step, 350 at two steps, and 80 at three steps. The two existing sprint opportunities are skipped if either traversed cell is within two steps of such a head. These are preferences in scoring, not a guarantee of escape when every move is dangerous.

Before round 250, a dragon of length at least 8 may make a deliberate length-2 combat split if it can legally split, its **observed actual tail** is adjacent across a normal edge to an enemy head of lower ID that has already acted this round, at least five enemy segments are visible, and at least three would be within the child's new vision. It does not spend the turn splitting if any enemy head is within two steps of the parent's head. The child starts at the old tail, gets a fresh observation later in the same round, and uses the existing adjacent-head ram rule; the parent cannot command it directly. Other dragons may still intervene before the child acts. The pre-250 limit avoids a new child receiving the existing late-game Champion label, which would suppress that ram.

If a nearby dragon seen last turn disappears and its former head cell now contains a pearl, or two of its formerly seen body cells now contain pearls, the bot treats that as a likely death. For two rounds it considers the old body cells that are still unobserved or contain pearls as search goals within six steps. A safe first step towards that area receives a +250 route preference in ordinary move scoring; occupied cells, poor pockets, nearby enemy heads, and the existing enemy-risk penalties still apply. Visible empty former body cells are dropped from the goal set. No new sonar message is used, and an out-of-view death with no visible pearl evidence cannot be detected.

## Sonar protocol

Protocol 3 keeps the V2 complete-pair message: both canonical portal endpoints, their **shared** portal ID, edge orientation, explored status, team, format marker, and checksum in one 64-bit report. It never presents a one-endpoint claim as a full connection. A dragon that has itself crossed a portal rotates through its locally crossed pairs and reserves at least one directional beam for a complete pair every turn. An active Scout reserves two or three directional beams for its expiring claim, depending on whether a crossed pair also needs a beam; the remaining beam carries the existing enemy, pearl, urgent-pair, or periodic-pair report. Beam directions rotate each round. Reception checks checksum, team, round, and conflicting topology before applying a report.

On an engine that has not negotiated Protocol 3, the existing 32-bit pearl/enemy fallback remains. It cannot carry the complete portal pair or the claim. Thus team portal coordination requires the current sonar system and received messages remain opportunistic rather than perfectly synchronized.

## Build and focused checks

From this directory:

```sh
clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror main.cpp -o /tmp/xxqz-v3-bot
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I. -x c++ verification/v3_tests.cpp.txt -o /tmp/xxqz-v3-tests
/tmp/xxqz-v3-tests
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I. -x c++ verification/short_avoid.cpp.txt -o /tmp/xxqz-v3-short-tests
/tmp/xxqz-v3-short-tests
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I. -x c++ verification/death_pearls.cpp.txt -o /tmp/xxqz-v3-death-tests
/tmp/xxqz-v3-death-tests
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I. -x c++ verification/tactical_split.cpp.txt -o /tmp/xxqz-v3-tactical-tests
/tmp/xxqz-v3-tactical-tests
```

The `.cpp.txt` suffix keeps test `main` functions out of the submission source glob. The eight `FEAT_V3_*` macros in `core.hpp` default to 1 and allow individual ablations. `V3_SCOUT_MAX_PATH`, `V3_PEARL_MAX_PATH`, `V3_PEARL_CLUSTER_DETOUR`, and `V3_EXPLORED_BONUS` are tunable constants. Focused checks cover length-2 Scout activation, split deferral, claims and local election, confirmed crossings, body evidence versus mere adjacency, pair/claim beam scheduling, pearl priority, equal-length route ties, one-pearl versus multi-pearl pockets, teammate-head avoidance, emergency splitting, the length-9/10/11 and three-segment avoidance boundaries, fresh death clues versus unsafe approaches, and a same-round tail-spawned ram. Competitive results under the current engine have not yet been measured.

## Combat and sprint corrections (2026-09-27)

Profitable adjacent head trades now run before proximity-only emergency scoring. An ID-selected Champion of length at most 5 may trade into a visibly longer enemy; the final friendly dragon and longer Champions remain protected. Sprint attacks search at most four currently observed steps and simulate the body after each step. Extra-step payment is checked before that step's collision and pearl eating. Earlier pearls can fund a length-2 dragon's sprint; a pearl on an unaffordable later step cannot.

Pearl-pocket splits retain two parent segments and give the entire rear body to the child. The earlier fixed child-size-5 threshold and unsupported parent portal restriction were removed. The example of harvesting five pearls from length 2 still produces a length-5 child, while smaller valid pockets can produce smaller children. Friendly-head guards redirect when possible and never preserve a friendly-head collision merely because no alternative exists. Adjacent pearl collection does not advertise a portal claim just because the movement direction also matches a portal route.

The new `verification/combat_priority.cpp.txt` checks the reproduced round-302 decision, short Champion trades, population protection, pearl-funded second/third sprint steps, unpaid steps and self-collision. `verification/resource_pocket.cpp.txt` follows complete collection sequences that produce children of lengths 3 and 5. Both use the same ASan/UBSan compilation pattern as the focused checks above. These are planner regressions, not a replay of the user's original match or competitive map benchmarks.

## Reverted route commitment and BugNav (2026-09-27)

Testing showed that persistent frontier routing, a strong committed-route score bonus, and the BugNav wall-follow fallback hurt map performance, so they were removed. Exploration is selected afresh with bounded BFS each turn. BFS can still select a visible pearl for collection when its wrapped displacement is in the same forward half-plane as the BFS-selected frontier; the pearl direction rule is unchanged, and a selected pearl remains the collection target while the collection mission is active. Candidate moves compare short-enemy attack reachability before ordinary scores: confirmed reachable attacks rank below uncertain reach, which ranks below routes with no observed attack path. Sprint reach is bounded to four steps, and incomplete enemy body/visibility evidence is treated as uncertain. The final teammate guard redirects when it can; if the only remaining choice traps an ally, it sacrifices the shorter dragon, while avoiding direct friendly-head collisions that could kill both.


## Two-step teammate escape check (2026-09-27)

Before a move, sprint, or split, the planner now compares each not-yet-acted teammate's known two-step escape paths before and after the action. It rejects the action if it removes every previously available two-step path, while keeping the existing immediate sole-exit check. Unknown edges remain uncertain and do not trigger a speculative rejection. If no safe redirection exists, the final action guard skips the unsafe action rather than executing it.

## V4 baseline: enemy attack reachability refinement (2026-09-27)

`V4_statemachine` is the selected performance baseline. This refinement changes only the short-enemy threat evaluator; it preserves V4's state transitions, action priority, and lexicographic threat-before-move-score selection. For each candidate landing cell, it simulates an enemy's normal move or up to four sprint steps with body occupancy, per-step sprint cost, visible pearl pickups that can fund later steps, portal transitions, and collision-before-pearl ordering. An exact body and fully visible route produce confirmed risk; incomplete body or route evidence stays uncertain. The evaluator also marks the early tail-spawned tactical split as uncertain when its visible prerequisites hold, since the opponent's population capacity is hidden. This remains a bounded reachability model, not a prediction of which action the opponent will choose.


## V4 aggressive donation update (2026-09-27)

See `SACRIFICE_EXPERIMENT.md` for the staged donation schedule. Quiet-phase donors now use ally-position sonar to rendezvous, pool pearls with an identified larger teammate, and stop late expansion splits above the phase population reserve. A surviving length-8+ dragon prioritizes confirmed pearls from friendly deaths; those clues remain for up to 12 rounds. This is local resource-concentration behavior within the existing state-machine phases. Strict C++ builds pass; competitive match results are still needed.
