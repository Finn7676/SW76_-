# R10 Queen Hunt

Parent: the frozen R4 sprint4 bot from `robust_sprint4_82001`. This version
changes only target value inside `sprintRamAction`; the caller remains after
ordinary split and adjacent attacks. It is not combined with split-priority
or donor-routing experiments.

Hypothesis: a fixed enemy queen is important to the updated lexicographic
score even at length two. R4's unchanged ordinary trade-up thresholds reject
many such queens. Prioritizing a currently visible enemy-queen head along a
proven 1-4-step attack route may improve queen elimination across maps/seeds.
The 612-replay matched H14/R3/R4 mechanism audit observed 206 R4 enemy-head
hits from requests with at least three steps but zero enemy-queen hits from
those requests; this is a motivation, not a win-rate claim.

Mechanism: a target is an enemy queen only when its currently visible occupied
head has the fixed `enemyQueenId()`. That target does not require an observed
size lead. Among queen routes the shortest examined route wins; a queen
route wins over an ordinary head. With no reachable qualifying queen,
ordinary targets retain R4's observed-size priority, shortest-path tie break,
normal/attrition thresholds and traversal order.

Unchanged restrictions: nonqueen, nonchampion attacker, initial length at
least three, population at least four, complete initial own-body proof,
256-transition budget, maximum four steps, visible ordinary-open edges and
cells only, fixed initial free-step count, paid-step eligibility checked
before movement, exact body-aware surviving intermediate projections. No
hidden board state, unseen queen inference, portal route, or caller-order
change is introduced. The attack remains deliberately fatal on its final
enemy-head collision.

Risks: the attacker may be valuable despite not being a champion; abandoning
an ordinary larger-head trade may harm final longest/total length; corpse
collection remains uncoordinated; a queen already vulnerable to another
friend may be an unnecessary sacrifice. Keeping the original split stage
also means a valid queen route can remain unused. These risks require paired
multi-map, both-side, multi-seed testing; focused tests establish only rule
and scope properties.

Focused checks cover visible length-two queen routes at two, three and four
steps; both fixed queen IDs; initial paid-step constraints including pearls;
current own-tail, static enemy/friendly body rejection; queen preference
over an ordinary size-20 head; ordinary fallback with an invisible queen;
shortest queen route; portal/unknown/invisible rejection; original eligibility;
200 ordinary-target comparisons against the renamed frozen R4 function.
The inherited R4 sprint test suite must also pass. No matches are launched
by this implementation task; root owns registration, freeze, audit and
evaluation. No measured win-rate improvement is claimed.
