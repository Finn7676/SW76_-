# H9: Queen Feeding and Earlier Swarm Maturity

Registered 2026-10-03 before H9 matches. Isolated child of
`tyhh_qos_counter_queen_feed_v127`, copying only main/helper/bot.toml and
queen_feed.hpp. Only new source change against this parent is the ordinary
non-champion expansion window `ROUND < 470` -> `ROUND < 350`. Population
fallback `POP < 8`, queen bootstrap `< 120` and the feeding module are unchanged.
No H3/H4/H5/H7 changes are included.

This completes the development2x2 factorial comparison: queen parent, feeding
only, maturity350 only, and feeding plus maturity350. Prior registered samples
were parent14/34 V3 and16/34 xxqztest, feeding13/34 and18/34, maturity18/34 and
16/34. The combination has no measured result at registration.

Hypothesis: feeding may benefit queen growth against xxqztest while earlier
swarm maturity may preserve longest/total resources against V3_portal.
Complementarity is a hypothesis, not an additive-effect assumption: reduced
late scouts can also reduce donor availability, exploration and recovery, so
the interaction may be negative.

Matched ranked17 maps, both sides and frozen seed schedule evaluate wins
separately against each opponent. Queen survival/length, longest, total,
population and feeding triggers are secondary. Single-map-seed development
results are descriptive; no held-out or significance claim is implied. Root
owns source freeze and suite scheduling. No matches are launched here.
