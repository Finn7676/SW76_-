# Small-map sector experiment

This version enables the sector preference on maps smaller than 1200 cells.
Small maps use two broad north/south responsibilities, a weaker alignment
weight, and a shorter 120-round window. Large-map behavior is inherited from
`tyhh_qos_feed_sector`.

Initial `default_small` test (two games, one on each side):

| Opponent | Sector-small wins | Opponent wins |
|---|---:|---:|
| `tyhh_qos_feed` | 0 | 2 |
| `tyhh` | 0 | 2 |

Results: `replays/20260926-084846-fpgc34e3` and
`replays/20260926-084918-j4cmt1ky`.

This first sample is negative. The small-map sector preference should remain
isolated until more games establish whether it is a repeatable effect.
