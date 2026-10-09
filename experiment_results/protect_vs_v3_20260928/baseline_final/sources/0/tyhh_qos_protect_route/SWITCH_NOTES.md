# Current protection implementation — 2026-09-27

The current code replaces the retired one-round reservation described in the
historical notes below. A selected move is executed before its sonar arrives;
that move's destination is not a future reservation.

- Original pearl/enemy/portal/frontier message selection remains active. The
  movement heading carries that legacy message. The other three directions
  can carry a separate protocol-3 uint64 status. Four-direction room claims
  keep all four directions and suppress extra status for that turn.
- Status contains team, sender ID, sending round, post-action head and length.
  Receive timing follows ID action order, and the report is only used if the
  current visible friendly head matches. Unknown portal exits are not invented.
- Guards require a visible teammate at least length 6 and at least two segments
  longer than the acting dragon. Blocking all its known exits is a strong
  conflict; heading overlap with another available exit costs at most 48 score
  points, then 24 after a yield, with no soft penalty after two consecutive yields.
- Every action branch first proposes an action and passes one common review.
  A strong exit conflict causes a detour only when a physically viable ordinary
  move exists. Otherwise the proposed action is retained. Confirmed friendly
  head collisions use an emergency legal split or a non-friendly-head exit.
- Exploration targets, pearl scoring, portal priority and existing switch
  thresholds are not retuned. No map name is used by the new protection layer.
- PROTECT / PROTECT_KEEP / PROTECT_DIRECT / PSTAT replay logs expose interventions.

Full evaluation protocol, source hashes, tests and replays are recorded in
experiment_results/protect_local_20260927/. The production tyhh source is
unchanged by this update.

## Historical notes (not current test results)

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
