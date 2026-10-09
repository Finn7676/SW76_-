# Sprint And Donation Mechanisms

Official replay events from matched, source-frozen native 1.2.7 suites; all selected games previously passed scoring and manifest audits.

Requests with >=3 steps include free harvest and queen-survival actions. They are not counted as proactive attacks unless the official same-turn death events record an actor/enemy head collision. A body death is the engine's category; this script does not invent the other body's owner.

| Version | Opponent | Games | >=2 Requests | >=2 Head Hits | >=3 Requests | >=3 Head Hits | >=3 Body Deaths | >=3 Self Deaths |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| H14 | V3_portal | 102 | 9155 | 264 | 2383 | 0 | 0 | 30 |
| H14 | xxqztest | 102 | 11862 | 171 | 3189 | 0 | 0 | 42 |
| R3_donor_route | V3_portal | 102 | 9269 | 242 | 2556 | 0 | 0 | 29 |
| R3_donor_route | xxqztest | 102 | 11577 | 155 | 3168 | 0 | 0 | 30 |
| R4_sprint4 | V3_portal | 102 | 9230 | 365 | 2472 | 113 | 0 | 36 |
| R4_sprint4 | xxqztest | 102 | 12279 | 253 | 3324 | 93 | 0 | 38 |

## Head-Hit Timing

| Version | Steps | Rounds 0-99 | 100-199 | 200-299 | 300-399 | 400-499 | Initial Lengths |
|---|---|---:|---:|---:|---:|---:|---|
| H14 | >=2 | 12 | 70 | 112 | 109 | 132 | {"3": 389, "4": 32, "5": 8, "6": 5, "8": 1} |
| H14 | >=3 | 0 | 0 | 0 | 0 | 0 | {} |
| R3_donor_route | >=2 | 12 | 70 | 112 | 101 | 102 | {"3": 352, "4": 27, "5": 7, "6": 5, "7": 3, "8": 3} |
| R3_donor_route | >=3 | 0 | 0 | 0 | 0 | 0 | {} |
| R4_sprint4 | >=2 | 19 | 98 | 145 | 173 | 183 | {"3": 503, "4": 85, "5": 19, "6": 7, "7": 2, "9": 1, "11": 1} |
| R4_sprint4 | >=3 | 7 | 25 | 47 | 60 | 67 | {"3": 126, "4": 59, "5": 13, "6": 5, "7": 2, "11": 1} |

## Consolidation Pickup

Collections require directly observed donated-pearl creation, later removal, and collector movement to that tile during the same turn. These are pearl cells, not guaranteed total length gain or complete-corpse consumption.

| Version | Opponent | Feed Logs | Intended Collections | Intended Head Collections | Feeds With Pickup | Other-Friendly Collections | Enemy Collections | Route Logs | Route Donor-Game Pairs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| H14 | V3_portal | 2389 | 2762 | 2141 | 2168 | 465 | 3 | 0 | 0 |
| H14 | xxqztest | 3130 | 3586 | 2799 | 2835 | 600 | 1 | 0 | 0 |
| R3_donor_route | V3_portal | 2553 | 2887 | 2233 | 2268 | 514 | 10 | 5107 | 2146 |
| R3_donor_route | xxqztest | 3280 | 3654 | 2865 | 2897 | 668 | 3 | 6844 | 2671 |
| R4_sprint4 | V3_portal | 2397 | 2769 | 2148 | 2179 | 469 | 4 | 0 | 0 |
| R4_sprint4 | xxqztest | 3166 | 3669 | 2831 | 2866 | 609 | 3 | 0 | 0 |

## Route Association

| Version | Prior-Route Feeds | Intended Collections | Feeds With Pickup |
|---|---:|---:|---:|
| H14 | 0 | 0 | 0 |
| R3_donor_route | 3027 | 3463 | 2734 |
| R4_sprint4 | 0 | 0 | 0 |

Association means the same donor logged DONOR_ROUTE toward that receiver in the previous 20 rounds. It is not a counterfactual causal estimate.

## Long Attack Events

### H14

No >=3-step request caused an observed enemy-head collision.

### R3_donor_route

No >=3-step request caused an observed enemy-head collision.

### R4_sprint4

Map hit counts: {"australia": 22, "autarky": 4, "default": 4, "devil": 3, "islands": 4, "maze": 5, "schooltime": 29, "slithery_fight": 44, "trophy": 2, "unsw": 89}
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

