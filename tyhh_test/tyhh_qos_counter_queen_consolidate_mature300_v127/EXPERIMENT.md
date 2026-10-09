# H13: H5 and Round 300 Maturity Sensitivity

Registered 2026-10-03 before H13 matches. Isolated H5 consolidate_v127 child;
copies main/helper/bot.toml/queen_feed.hpp/consolidate.hpp only. Sole change:
ordinary expansion threshold470 ->300. H5 queen feeding, nonqueen consolidation
and recipient-hold contract stay unchanged. Population fallback `<8` and
queen bootstrap `<120` are unchanged. No guards or margin factors are added.

This tests the neighboring round300 window against H5 and H11 round350.
Potential complementarity is not assumed additive: early growth may increase
recipient length while reducing donor availability, map coverage and recovery.
Both beneficial and negative results are retained. These are development
tuning comparisons, not held-out evidence. Root owns frozen ranked17/both-side
matches against V3_portal and xxqztest separately. No matches launched here.
