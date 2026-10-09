# Direct portal modules experiment

Candidate: `tyhh_test/tyhh_portal_direct` (Relay + Scout + Claim directly added to root `tyhh`).
Control: root `tyhh` versus an identical copy.
Protocol: 12 maps x both sides, one deterministic game per condition.

| Comparison | Wins | Losses | Win rate |
|---|---:|---:|---:|
| candidate | 13 | 11 | 54.2% |
| control | 12 | 12 | 50.0% |

## Interpretation

The candidate scored 13/24 while the identical-copy control scored 12/24. This is a +1 win (+4.2 percentage points) in one deterministic sweep, with 11 of 24 conditions changing outcome relative to the control. The result is directional evidence only; it is not enough to claim a stable improvement.

## Condition flips

- Colosseum.map side B: loss -> win
- big_empty.map side A: win -> loss
- big_empty.map side B: loss -> win
- default.map side A: win -> loss
- default_small.map side B: loss -> win
- devil.map side A: loss -> win
- queen_of_spades.map side A: loss -> win
- queen_of_spades.map side B: win -> loss
- stronghold.map side A: loss -> win
- trauma.map side A: win -> loss
- trophy.map side A: win -> loss

## Per-map outcomes

| Map | Control wins | Candidate wins | Flips |
|---|---:|---:|---:|
| Colosseum.map | 1/2 | 2/2 | 1 |
| arena.map | 1/2 | 1/2 | 0 |
| big_empty.map | 1/2 | 1/2 | 2 |
| default.map | 1/2 | 0/2 | 1 |
| default_small.map | 1/2 | 2/2 | 1 |
| devil.map | 1/2 | 2/2 | 1 |
| help.map | 1/2 | 1/2 | 0 |
| queen_of_spades.map | 1/2 | 1/2 | 2 |
| schooltime.map | 1/2 | 1/2 | 0 |
| stronghold.map | 1/2 | 2/2 | 1 |
| trauma.map | 1/2 | 0/2 | 1 |
| trophy.map | 1/2 | 0/2 | 1 |

## Decision

Keep this version as an experiment, not as a replacement baseline. The next useful run is a repeated matched-condition test after fixing Scout/Claim interactions and then an ablation of Relay, Scout, and Claim in this direct-root implementation.