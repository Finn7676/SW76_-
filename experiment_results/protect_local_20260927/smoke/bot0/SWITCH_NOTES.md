# Adaptive exploration switch

This experiment is copied from `tyhh_qos_feed_v41_len2_ram`. Version41 target
distance is used while exploration mode is active. Exploration cannot end
before round 260. On maps with area at least 1200 cells, it may end earlier
only after 30 consecutive rounds with a size-scaled low unseen frontier within
radius 220; compact maps use the fixed round-260 floor. The length-2 head-ram
rule remains enabled.

Against `tyhh_qos_feed` (28 games, 14 maps, two sides): **18-10 (64.3%)**.
Replay folder: `replays/20260926-161923-obdxjs17`.

Map win rates: Colosseum 2/2, arena 2/2, autarky 1/2, big_empty 1/2,
default 0/2, default_small 2/2, devil 1/2, dilemma 2/2, help 1/2,
queen_of_spades 0/2, schooltime 0/2, stronghold 2/2, trauma 2/2, trophy 2/2.

Against `tyhh`: **19-9 (67.9%)**. Replay folder:
`replays/20260926-163338-ltvc8441`.

Against `tyhh_qos_feed_v41_len2_ram`: **11-17 (39.3%)**. Replay folder:
`replays/20260926-165348-i0f4c0fc`.

Against `V3_portal`: **14-14 (50.0%)**. Replay folder:
`replays/20260926-165948-uxrlycyg`.
# One-round route reservation experiment

This directory is based on the current `tyhh` implementation. Long dragons
(length at least 8) broadcast the move they actually selected after deciding
it. A short dragon avoids a received first or second destination for one round
only. No predicted pearl/frontier route is used, and the reservation reuses
the existing sonar channel without adding a second broadcast stream.

Latest full-scene tests (10 maps, 2 games per map):

- Against `tyhh`: 11/20 wins (55.0%), replay directory
  `replays/20260927-091816-iflvhwsn`.
- Against `V3_portal`: 8/20 wins (40.0%), replay directory
  `replays/20260927-092143-0d55qdvl`.
