# Sprint And Donation Mechanisms

Official replay events from matched, source-frozen native 1.2.7 suites; all selected games previously passed scoring and manifest audits.

Requests with >=3 steps include free harvest and queen-survival actions. They are not counted as proactive attacks unless the official same-turn death events record an actor/enemy head collision. A body death is the engine's category; this script does not invent the other body's owner.

| Version | Opponent | Games | >=2 Requests | >=2 Head Hits | >=3 Requests | >=3 Head Hits | >=3 Body Deaths | >=3 Self Deaths |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| H14 | V3_portal | 34 | 3081 | 83 | 907 | 0 | 0 | 11 |
| H14 | xxqztest | 34 | 3437 | 62 | 886 | 0 | 0 | 7 |
| R3_donor_route | V3_portal | 34 | 3135 | 72 | 947 | 0 | 0 | 10 |
| R3_donor_route | xxqztest | 34 | 3423 | 60 | 831 | 0 | 0 | 6 |
| R4_sprint4 | V3_portal | 34 | 3099 | 94 | 903 | 21 | 0 | 12 |
| R4_sprint4 | xxqztest | 34 | 3896 | 89 | 946 | 34 | 0 | 7 |

## Head-Hit Timing

| Version | Steps | Rounds 0-99 | 100-199 | 200-299 | 300-399 | 400-499 | Initial Lengths |
|---|---|---:|---:|---:|---:|---:|---|
| H14 | >=2 | 3 | 28 | 42 | 33 | 39 | {"3": 127, "4": 10, "5": 5, "6": 2, "8": 1} |
| H14 | >=3 | 0 | 0 | 0 | 0 | 0 | {} |
| R3_donor_route | >=2 | 3 | 28 | 42 | 32 | 27 | {"3": 113, "4": 8, "5": 5, "6": 2, "7": 2, "8": 2} |
| R3_donor_route | >=3 | 0 | 0 | 0 | 0 | 0 | {} |
| R4_sprint4 | >=2 | 5 | 34 | 47 | 43 | 54 | {"3": 156, "4": 18, "5": 7, "9": 1, "11": 1} |
| R4_sprint4 | >=3 | 2 | 11 | 11 | 14 | 17 | {"3": 38, "4": 13, "5": 3, "11": 1} |

## Consolidation Pickup

Collections require directly observed donated-pearl creation, later removal, and collector movement to that tile during the same turn. These are pearl cells, not guaranteed total length gain or complete-corpse consumption.

| Version | Opponent | Feed Logs | Intended Collections | Intended Head Collections | Feeds With Pickup | Other-Friendly Collections | Enemy Collections | Route Logs | Route Donor-Game Pairs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| H14 | V3_portal | 760 | 912 | 702 | 709 | 127 | 0 | 0 | 0 |
| H14 | xxqztest | 864 | 964 | 787 | 795 | 158 | 0 | 0 | 0 |
| R3_donor_route | V3_portal | 792 | 928 | 704 | 716 | 136 | 1 | 1716 | 678 |
| R3_donor_route | xxqztest | 888 | 970 | 784 | 788 | 167 | 1 | 1988 | 725 |
| R4_sprint4 | V3_portal | 767 | 917 | 704 | 710 | 126 | 0 | 0 | 0 |
| R4_sprint4 | xxqztest | 892 | 1051 | 805 | 818 | 162 | 1 | 0 | 0 |

## Route Association

| Version | Prior-Route Feeds | Intended Collections | Feeds With Pickup |
|---|---:|---:|---:|
| H14 | 0 | 0 | 0 |
| R3_donor_route | 905 | 1041 | 822 |
| R4_sprint4 | 0 | 0 | 0 |

Association means the same donor logged DONOR_ROUTE toward that receiver in the previous 20 rounds. It is not a counterfactual causal estimate.

## Long Attack Events

### H14

No >=3-step request caused an observed enemy-head collision.

### R3_donor_route

No >=3-step request caused an observed enemy-head collision.

### R4_sprint4

Map hit counts: {"australia": 7, "autarky": 2, "default": 1, "maze": 2, "schooltime": 9, "slithery_fight": 12, "unsw": 22}
- robust_sprint4_82001 game 1, V3_portal, australia, side A, round index 293: actor 14 initial length 3, requested 3, succeeded 2, enemy [{"id": 397, "side": "B", "reason": "head", "length": 4}]; game loss.
- robust_sprint4_82001 game 1, V3_portal, australia, side A, round index 406: actor 557 initial length 3, requested 3, succeeded 2, enemy [{"id": 496, "side": "B", "reason": "head", "length": 4}]; game loss.
- robust_sprint4_82001 game 2, V3_portal, australia, side B, round index 477: actor 339 initial length 3, requested 4, succeeded 3, enemy [{"id": 272, "side": "A", "reason": "head", "length": 7}]; game loss.
- robust_sprint4_82001 game 3, V3_portal, autarky, side A, round index 495: actor 165 initial length 5, requested 4, succeeded 3, enemy [{"id": 144, "side": "B", "reason": "head", "length": 10}]; game win.
- robust_sprint4_82001 game 4, V3_portal, autarky, side B, round index 494: actor 63 initial length 3, requested 4, succeeded 3, enemy [{"id": 96, "side": "A", "reason": "head", "length": 4}]; game loss.
- robust_sprint4_82001 game 17, V3_portal, slithery_fight, side A, round index 195: actor 514 initial length 3, requested 3, succeeded 2, enemy [{"id": 393, "side": "B", "reason": "head", "length": 4}]; game loss.
- robust_sprint4_82001 game 18, V3_portal, slithery_fight, side B, round index 150: actor 203 initial length 4, requested 3, succeeded 2, enemy [{"id": 427, "side": "A", "reason": "head", "length": 8}]; game loss.
- robust_sprint4_82001 game 18, V3_portal, slithery_fight, side B, round index 179: actor 89 initial length 3, requested 3, succeeded 2, enemy [{"id": 482, "side": "A", "reason": "head", "length": 4}]; game loss.
- robust_sprint4_82001 game 18, V3_portal, slithery_fight, side B, round index 204: actor 241 initial length 3, requested 3, succeeded 2, enemy [{"id": 599, "side": "A", "reason": "head", "length": 4}]; game loss.
- robust_sprint4_82001 game 18, V3_portal, slithery_fight, side B, round index 265: actor 501 initial length 3, requested 3, succeeded 2, enemy [{"id": 72, "side": "A", "reason": "head", "length": 4}]; game loss.
- robust_sprint4_82001 game 31, V3_portal, unsw, side A, round index 23: actor 16 initial length 3, requested 3, succeeded 2, enemy [{"id": 36, "side": "B", "reason": "head", "length": 4}]; game win.
- robust_sprint4_82001 game 31, V3_portal, unsw, side A, round index 338: actor 630 initial length 4, requested 4, succeeded 3, enemy [{"id": 591, "side": "B", "reason": "head", "length": 9}]; game win.

