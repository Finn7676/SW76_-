# V3 portal and role experiment

V3 starts from the frozen V2 source. It changes strategy; no win-rate gain is assumed. `helper.hpp` and `bot.toml` are unchanged. The original Champion, Core, Fragile, and Attrition gates remain in force. Sacrifice no longer targets a teammate's head.

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
