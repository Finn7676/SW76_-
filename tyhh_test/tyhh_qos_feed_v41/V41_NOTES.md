# Version41-inspired exploration experiment

The reference implementation uses a persistent exploration target rather than
reselecting a local frontier every turn. `BRExplore` keeps an outward direction
and repeatedly chooses a target in that direction; `RKExplore` keeps a random
map target for about 30 rounds, tracks visited quadrants, and replaces a target
when it is sensed or its timer expires. The adapted C++ experiment keeps the
same transferable idea while respecting this game's toroidal map: each dragon
gets a deterministic outward target derived from its ID and runtime map size,
keeps it for 75 rounds, and receives a soft score for reducing toroidal
distance to that target during the first 300 rounds. Pearl and safety routes
still take priority, so this is not a forced movement rule.

The version is copied from `tyhh_qos_feed`; it is isolated under `tyhh_test`.
It compiled and was evaluated against `tyhh_qos_feed` on all 14 maps, two games
per map, one on each side:

| Version | Wins | Losses | Win rate | Results |
|---|---:|---:|---:|---|
| `tyhh_qos_feed_v41` | 12 | 16 | 42.9% | `replays/20260926-090955-bdnqzuh5` |

The result is below the `tyhh_qos_feed` opponent in this initial sample. The
`default_small` result was 2/2 in this run, while the earlier isolated two-game
run was 0/2; this variation confirms that two games are too few for a stable
map-level conclusion.
