# R10 82001 Queen-Hit Mechanism

Completed audited 82001 development only; no reserved seeds read

Official replay actions/deaths: our head trade requires our active actor and enemy both dying for head in that same turn. An enemy queen dying for head during its own move is not our initiated queen attack. Legality of a recorded hit is supported by successful prefix movement and official head death, not an inferred target or hidden-board input. Temporal association is not proof of winning causality.

| Version | Opponent | Wins | Our Recorded Head Trades | Our Initiated Queen Hits | Enemy Queen Death Reasons |
|---|---|---:|---:|---:|---|
| robust_h14_82001 | V3_portal | 15/34 | 673 | 13 | {"body": 3, "head": 15, "self": 11} |
| robust_h14_82001 | xxqztest | 16/34 | 601 | 5 | {"body": 5, "head": 12, "invalid_action": 1, "self": 12} |
| robust_sprint4_82001 | V3_portal | 16/34 | 687 | 13 | {"body": 3, "head": 15, "self": 11} |
| robust_sprint4_82001 | xxqztest | 18/34 | 624 | 5 | {"body": 5, "head": 12, "invalid_action": 1, "self": 12} |
| robust_queen_hunt_82001_retry1 | V3_portal | 16/34 | 712 | 14 | {"body": 3, "head": 15, "self": 11} |
| robust_queen_hunt_82001_retry1 | xxqztest | 19/34 | 677 | 9 | {"body": 5, "head": 12, "invalid_action": 1, "self": 12} |

## R10_vs_R4

- V3_portal, australia, A, seed 82001: loss -> win; first action difference ((105, 37, 0, (2,), 3), (105, 37, 0, (2, 1), 3)).
  baseline: [{"id": 1, "round": 106, "reason": "head", "our_hit": false}, {"id": 0, "round": 248, "reason": "head", "our_hit": false}]; standings {"A": {"population": 20, "longest": 15, "total": 121, "queen": 0}, "B": {"population": 36, "longest": 23, "total": 201, "queen": 0}}.
  candidate: [{"id": 1, "round": 105, "reason": "head", "our_hit": true}, {"id": 0, "round": 306, "reason": "head", "our_hit": false}]; standings {"A": {"population": 25, "longest": 25, "total": 150, "queen": 0}, "B": {"population": 42, "longest": 17, "total": 190, "queen": 0}}.
- V3_portal, devil, B, seed 82004: win -> loss; first action difference ((46, 21, 0, (1,), 3), (46, 21, 0, (2, 3), 3)).
  baseline: [{"id": 0, "round": 66, "reason": "head", "our_hit": true}]; standings {"A": {"population": 0, "longest": 0, "total": 0, "queen": 0}, "B": {"population": 48, "longest": 11, "total": 122, "queen": 11}}.
  candidate: [{"id": 0, "round": 46, "reason": "head", "our_hit": true}, {"id": 1, "round": 260, "reason": "head", "our_hit": false}]; standings {"A": {"population": 32, "longest": 15, "total": 153, "queen": 0}, "B": {"population": 1, "longest": 2, "total": 2, "queen": 0}}.
- xxqztest, default, A, seed 82003: loss -> win; first action difference ((92, 19, 0, (3,), 3), (92, 19, 0, (2, 3), 3)).
  baseline: [{"id": 0, "round": 48, "reason": "head", "our_hit": false}, {"id": 1, "round": 93, "reason": "head", "our_hit": false}]; standings {"A": {"population": 1, "longest": 2, "total": 2, "queen": 0}, "B": {"population": 55, "longest": 20, "total": 197, "queen": 0}}.
  candidate: [{"id": 0, "round": 48, "reason": "head", "our_hit": false}, {"id": 1, "round": 92, "reason": "head", "our_hit": true}]; standings {"A": {"population": 8, "longest": 10, "total": 43, "queen": 0}, "B": {"population": 37, "longest": 6, "total": 100, "queen": 0}}.
- xxqztest, default, B, seed 82003: loss -> win; first action difference ((27, 3, 0, (3,), 3), (27, 3, 0, (3, 3), 3)).
  baseline: [{"id": 0, "round": 28, "reason": "head", "our_hit": false}, {"id": 1, "round": 53, "reason": "head", "our_hit": false}]; standings {"A": {"population": 40, "longest": 11, "total": 110, "queen": 0}, "B": {"population": 0, "longest": 0, "total": 0, "queen": 0}}.
  candidate: [{"id": 0, "round": 27, "reason": "head", "our_hit": true}, {"id": 1, "round": 53, "reason": "head", "our_hit": false}]; standings {"A": {"population": 10, "longest": 10, "total": 47, "queen": 0}, "B": {"population": 11, "longest": 12, "total": 71, "queen": 0}}.
- xxqztest, trophy, A, seed 82011: win -> loss; first action difference ((74, 2, 0, (0,), 3), (74, 2, 0, (0, 0), 3)).
  baseline: [{"id": 1, "round": 75, "reason": "head", "our_hit": false}, {"id": 0, "round": 94, "reason": "head", "our_hit": false}]; standings {"A": {"population": 47, "longest": 4, "total": 119, "queen": 0}, "B": {"population": 0, "longest": 0, "total": 0, "queen": 0}}.
  candidate: [{"id": 1, "round": 74, "reason": "head", "our_hit": true}, {"id": 0, "round": 91, "reason": "head", "our_hit": false}]; standings {"A": {"population": 0, "longest": 0, "total": 0, "queen": 0}, "B": {"population": 64, "longest": 6, "total": 202, "queen": 0}}.

## R10_vs_H14

- V3_portal, australia, A, seed 82001: loss -> win; first action difference ((105, 37, 0, (2,), 3), (105, 37, 0, (2, 1), 3)).
  baseline: [{"id": 1, "round": 106, "reason": "head", "our_hit": false}, {"id": 0, "round": 248, "reason": "head", "our_hit": false}]; standings {"A": {"population": 15, "longest": 19, "total": 100, "queen": 0}, "B": {"population": 41, "longest": 19, "total": 173, "queen": 0}}.
  candidate: [{"id": 1, "round": 105, "reason": "head", "our_hit": true}, {"id": 0, "round": 306, "reason": "head", "our_hit": false}]; standings {"A": {"population": 25, "longest": 25, "total": 150, "queen": 0}, "B": {"population": 42, "longest": 17, "total": 190, "queen": 0}}.
- V3_portal, autarky, A, seed 82002: loss -> win; first action difference ((495, 165, 0, (1,), 5), (495, 165, 0, (1, 0, 0, 1), 5)).
  baseline: [{"id": 0, "round": 44, "reason": "self", "our_hit": false}, {"id": 1, "round": 130, "reason": "head", "our_hit": true}]; standings {"A": {"population": 10, "longest": 8, "total": 62, "queen": 0}, "B": {"population": 8, "longest": 11, "total": 33, "queen": 0}}.
  candidate: [{"id": 0, "round": 44, "reason": "self", "our_hit": false}, {"id": 1, "round": 130, "reason": "head", "our_hit": true}]; standings {"A": {"population": 9, "longest": 8, "total": 56, "queen": 0}, "B": {"population": 8, "longest": 6, "total": 28, "queen": 0}}.
- V3_portal, devil, B, seed 82004: win -> loss; first action difference ((46, 21, 0, (1,), 3), (46, 21, 0, (2, 3), 3)).
  baseline: [{"id": 0, "round": 66, "reason": "head", "our_hit": true}]; standings {"A": {"population": 0, "longest": 0, "total": 0, "queen": 0}, "B": {"population": 48, "longest": 11, "total": 122, "queen": 11}}.
  candidate: [{"id": 0, "round": 46, "reason": "head", "our_hit": true}, {"id": 1, "round": 260, "reason": "head", "our_hit": false}]; standings {"A": {"population": 32, "longest": 15, "total": 153, "queen": 0}, "B": {"population": 1, "longest": 2, "total": 2, "queen": 0}}.
- xxqztest, default, A, seed 82003: loss -> win; first action difference ((92, 19, 0, (3,), 3), (92, 19, 0, (2, 3), 3)).
  baseline: [{"id": 0, "round": 48, "reason": "head", "our_hit": false}, {"id": 1, "round": 93, "reason": "head", "our_hit": false}]; standings {"A": {"population": 1, "longest": 2, "total": 2, "queen": 0}, "B": {"population": 55, "longest": 20, "total": 197, "queen": 0}}.
  candidate: [{"id": 0, "round": 48, "reason": "head", "our_hit": false}, {"id": 1, "round": 92, "reason": "head", "our_hit": true}]; standings {"A": {"population": 8, "longest": 10, "total": 43, "queen": 0}, "B": {"population": 37, "longest": 6, "total": 100, "queen": 0}}.
- xxqztest, default, B, seed 82003: loss -> win; first action difference ((27, 3, 0, (3,), 3), (27, 3, 0, (3, 3), 3)).
  baseline: [{"id": 0, "round": 28, "reason": "head", "our_hit": false}, {"id": 1, "round": 53, "reason": "head", "our_hit": false}]; standings {"A": {"population": 13, "longest": 20, "total": 114, "queen": 0}, "B": {"population": 8, "longest": 6, "total": 23, "queen": 0}}.
  candidate: [{"id": 0, "round": 27, "reason": "head", "our_hit": true}, {"id": 1, "round": 53, "reason": "head", "our_hit": false}]; standings {"A": {"population": 10, "longest": 10, "total": 47, "queen": 0}, "B": {"population": 11, "longest": 12, "total": 71, "queen": 0}}.
- xxqztest, maze, B, seed 82013: loss -> win; first action difference ((307, 411, 0, (3, 0), 3), (307, 411, 0, (3, 0, 3, 0), 3)).
  baseline: [{"id": 0, "round": 41, "reason": "self", "our_hit": false}, {"id": 1, "round": 125, "reason": "self", "our_hit": false}]; standings {"A": {"population": 19, "longest": 24, "total": 121, "queen": 0}, "B": {"population": 13, "longest": 23, "total": 91, "queen": 0}}.
  candidate: [{"id": 0, "round": 41, "reason": "self", "our_hit": false}, {"id": 1, "round": 125, "reason": "self", "our_hit": false}]; standings {"A": {"population": 41, "longest": 12, "total": 157, "queen": 0}, "B": {"population": 14, "longest": 26, "total": 91, "queen": 0}}.
- xxqztest, slithery_fight, A, seed 82009: loss -> win; first action difference ((153, 116, 0, (0,), 3), (153, 116, 0, (3, 3, 2, 2), 3)).
  baseline: [{"id": 0, "round": 1, "reason": "self", "our_hit": false}, {"id": 1, "round": 1, "reason": "self", "our_hit": false}]; standings {"A": {"population": 16, "longest": 37, "total": 143, "queen": 0}, "B": {"population": 35, "longest": 41, "total": 198, "queen": 0}}.
  candidate: [{"id": 0, "round": 1, "reason": "self", "our_hit": false}, {"id": 1, "round": 1, "reason": "self", "our_hit": false}]; standings {"A": {"population": 31, "longest": 39, "total": 245, "queen": 0}, "B": {"population": 14, "longest": 29, "total": 103, "queen": 0}}.
- xxqztest, trophy, A, seed 82011: win -> loss; first action difference ((74, 2, 0, (0,), 3), (74, 2, 0, (0, 0), 3)).
  baseline: [{"id": 1, "round": 75, "reason": "head", "our_hit": false}, {"id": 0, "round": 94, "reason": "head", "our_hit": false}]; standings {"A": {"population": 47, "longest": 4, "total": 119, "queen": 0}, "B": {"population": 0, "longest": 0, "total": 0, "queen": 0}}.
  candidate: [{"id": 1, "round": 74, "reason": "head", "our_hit": true}, {"id": 0, "round": 91, "reason": "head", "our_hit": false}]; standings {"A": {"population": 0, "longest": 0, "total": 0, "queen": 0}, "B": {"population": 64, "longest": 6, "total": 202, "queen": 0}}.
