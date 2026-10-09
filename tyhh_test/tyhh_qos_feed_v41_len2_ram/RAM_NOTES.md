# Length-2 aggressive head-to-head experiment

Baseline: `tyhh_qos_feed_v41`. Added rule in `main.cpp`: when `LEN == 2` and
an adjacent visible enemy head exists, move into it immediately, before the
normal split, safety and ram-value checks. No other length receives this rule.

All 14 maps, two games per map, one on each side:

| Opponent | Wins | Losses | Win rate | Results |
|---|---:|---:|---:|---|
| `tyhh_qos_feed` | 16 | 12 | 57.1% | `replays/20260926-094210-b392e2c6` |
| `tyhh` | 16 | 12 | 57.1% | `replays/20260926-094742-e8h0ri5n` |

The rule improved the matchup score relative to the earlier V41 run against
`tyhh_qos_feed` (12/28), but this is one 28-game sample. It was strong on
`devil`, `stronghold` and `trophy` against the feed baseline, while remaining
0/2 on `big_empty`, `default` and `schooltime`. Against `tyhh`, it was 2/2 on
`default_small`, `devil`, `help`, `schooltime` and `trauma`, but 0/2 on
`big_empty`, `default` and `queen_of_spades`.
