# Initial loss analysis

Run: `replays/20260926-090955-bdnqzuh5`. The 16 losses are listed below. Counts
are deaths of the experimental bot only; a game can contain many deaths before
the final length decision.

| Map | Side | Result | Own death causes | Interpretation |
|---|---|---|---|---|
| Colosseum | B | elimination, r112 | 37 head-to-head, 3 other-body, 2 self | early collision-heavy loss |
| arena | B | elimination, r49 | 19 head-to-head | direct head-to-head deficit |
| autarky | B | length, r500 | 84 head-to-head, 22 self, 3 other-body | attrition and trades |
| big_empty | A | length, r500 | 423 head-to-head, 36 other-body, 22 self | long attrition loss |
| default | A/B | length, r500 | 109/83 head-to-head plus self/other-body | both sides lost length race |
| dilemma | B | elimination, r488 | 13 head-to-head, 8 self | late elimination |
| help | A | length, r500 | 1,744 self, 510 head-to-head, 490 other-body | severe self-collision and crowding |
| queen_of_spades | A/B | elimination/length | 20 head-to-head; 29 head-to-head plus 9 self and 4 other-body | head-to-head weakness, especially B late |
| schooltime | A/B | length, r500 | 194/201 head-to-head plus 178/154 self | persistent collision-heavy attrition |
| stronghold | A/B | length, r500 | 92/86 head-to-head plus 117/123 self | self-collision and trades |
| trophy | A/B | elimination/length | 68/133 head-to-head plus 21/34 self | head-to-head deficit |

The dominant signal is not failure to find a target. It is that the persistent
target bias changes movement enough to create bad encounters. The adaptation
uses a target derived from the map centre and ID hash, rotates it every 75
rounds, and applies a large distance-reduction bonus. That differs from the
reference's finite-map outward target: this game wraps at map boundaries, so a
centre-relative target does not represent an unexplored map edge. On `help`,
`schooltime`, and `stronghold`, the resulting movement coincides with very high
self and other-body collisions. On `Colosseum`, `arena`, `big_empty`, and
`trophy`, the main loss mode is head-to-head trading.

The replay format records actions and deaths but not the internal target score,
so this is a behavioral diagnosis rather than a proof that every listed death
was directly caused by the target bonus. The implementation should not be
merged without weakening or removing the target bonus near enemy heads and
crowded friendly regions, and without using frontier evidence to validate a
target before committing to it.
