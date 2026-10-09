# H8: Earlier Swarm Maturity (Round 350)

Registered 2026-10-03 before any H8 match. Isolated child of
`tyhh_qos_counter_queen_v127`; all historical bots, opponents and results are
unchanged. No replay scan, auxiliary module, or matches are part of this
registration.

Single variable: the ordinary non-champion expansion window changes from
`ROUND < 470` to `ROUND < 350`. The existing `POP < 8` fallback remains, and
the queen's one-time bootstrap condition `ROUND < 120` is untouched. No queen
scoring, portal, combat, sprint, feeding, or action-review code changes.

Hypothesis: ending routine scout splitting earlier preserves more length in
the mature population for the final queen-first tiebreak, reducing late small
dragon clutter and improving longest/total resources. Risk: on large portal
maps, stopping routine scouts at round350 can leave unexplored resource rooms
and reduce population recovery; the `POP < 8` fallback may still split.

Primary evaluation remains matched ranked17 maps, both sides, separately
against V3_portal and xxqztest. Wins are primary; queen length, longest,
total, population, portal-room access and death causes are secondary. This is
single-seed development evidence only.
