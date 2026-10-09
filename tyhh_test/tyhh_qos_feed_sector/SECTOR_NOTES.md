# Runtime sector exploration experiment

This version copies `tyhh_qos_feed` and adds a conservative radial-sector bias.
The sector is assigned from a deterministic hash of the dragon ID, so sibling
dragons do not all prefer the same direction. The board centre is computed from
the runtime `MAP` dimensions. For large maps during the first 220 rounds, each
candidate destination receives a small score based on its alignment with the
dragon's sector (north/east/south/west); existing pearl, safety, portal,
enemy and occupancy scores remain in control. The sector term is a preference,
not a forced move, and no map name or fixed coordinate is used.

All-map evaluation used the 14 maps returned by the current `benchmark.py
--maps all` (including `autarky` and `dilemma`), with two games per map and one
game on each side.

| Opponent | Wins | Losses | Win rate | Result directory |
|---|---:|---:|---:|---|
| `tyhh_qos_feed` | 15 | 13 | 53.6% | `replays/20260926-063838-5f5_k4yp` |
| `V3_portal` | 15 | 13 | 53.6% | `replays/20260926-064428-9sw0vnhl` |
| `tyhh` | 14 | 14 | 50.0% | `replays/20260926-065509-ejjp3nzg` |

The sector version was not clearly better than either opponent in this small
sample. Against `tyhh_qos_feed`, it did well on `arena`, `autarky`, `help` and
`stronghold`, but went 0/2 on `default`, `devil` and `schooltime`. Against
`V3_portal`, it went 0/2 on `Colosseum`, `schooltime` and `trophy`, while going
2/2 on `autarky`, `big_empty`, `default_small` and `queen_of_spades`.

The experiment remains isolated in `tyhh_test`; no changes were made to
`tyhh_qos_feed` or `tyhh`.
