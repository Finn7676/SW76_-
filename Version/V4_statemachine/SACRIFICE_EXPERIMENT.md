# V4 aggressive donation policy

V4 starts team-length consolidation at round 300, once no enemy dragon or recent enemy report has been seen. The gate uses the existing enemy-history sonar: a fresh quiet teammate report lowers the local quiet window to seven rounds (six in the final phase). A newly observed dragon must itself have been alive for the same quiet window before it can donate.

| Rounds | Minimum team left | Maximum donor length | Recipient length | Quiet rounds |
| --- | ---: | ---: | ---: | ---: |
| 400–429 | 8 | 10 | 4 and at least 2 longer than donor | 18 (7 with quiet ally report) |
| 430–459 | 4 | 20 | 3 and at least 1 longer than donor | 10 (7 with quiet ally report) |
| 460–495 | 2 | 32 | 3 and at least 1 longer than donor | 6 |

A donor may be selected regardless of Champion ID. The relative size rule protects the largest dragons naturally: only a teammate sufficiently longer than the donor can receive it. The donor population floor and shrinking ordinary split threshold stop late expansion from repeatedly breaking the donated length back into short dragons. Donation stops when the remaining rounds cannot plausibly cover the donor's own travel, the pearls' collection, and the recipient's next turn.

Quiet teammates of recipient size broadcast their predicted next head cell, ID, length, and round on a reserved sonar beam. Reports are checksummed, team-checked, and retained for three rounds. IDs above the packet's 12-bit range rely on visible ally observations. A donor follows a viable recipient for up to six rounds without progress, prefers a larger recipient, and only dies when the visible receiver can collect the head pearl within two moves and has a safe onward cell. The route is recomputed as the receiver moves. Unknown portal exits are not used for a donation.

A long dragon that sees confirmed pearls from a dead teammate gets a short safe BFS route to collect them before returning to exploration. Friendly death clues last up to 12 rounds, while other death clues keep their previous two-round lifetime. Only observed pearls count as targets. Ordinary combat, emergency escape, and portal priorities remain ahead of the donation branch.

The old V3 experiment and its local match table below this policy are historical data for the earlier round-400, length-6 donor rule; they do not measure this V4 policy. No competitive result is claimed until V4 is played on `big_empty` and on maps with dispersed resources.
