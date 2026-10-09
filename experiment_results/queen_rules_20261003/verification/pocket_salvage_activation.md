# R15 Actual-Action Audit

Scope: the already completed xxqztest stripes seed 82014 games, both seats.
No new competitive games were run. Packed replay events, not proposal log
counts alone, establish the actions below. Event indices are zero-based.

## Candidate Seat A

All four `POCKET_SALVAGE` proposals became the intended committed action.

| Actor | Round | Committed action | Replay evidence |
| --- | --- | --- | --- |
| 2 | 14 | MOVE E, LEN4 to LEN5 | tag 771, action 772, move 774 |
| 2 | 15 | SPLIT 3, parent2 / child3 | tag 828, action 829, split 830, child 6 |
| 6 | 20 | MOVE E, LEN4 to LEN5 | tag 1112, action 1113, move 1115 |
| 6 | 21 | SPLIT 3, parent2 / child3 | tag 1171, action 1172, split 1173, child 8 |

The first rear child, actor 6, started at `(18,8),(17,8),(16,9)`.
Its first three turns committed N, W, S in rounds 15, 16, 17, respectively,
and produced living move events. Its parent 2 nevertheless died at round
16/event 889. Actor 6 later made the second pickup and split, and its own
parent half died at round 22/event 1244.

The second rear child, actor 8, started at `(17,7),(17,8),(16,9)`.
It committed W, S, E in rounds 21, 22, 23. The round 23 move traversed a
portal and grew it to LEN4 (move event 1317). It died at round 24/event 1375.
The audit therefore confirms short-horizon rear-child escape, not permanent
survival or a solution to the terminal parent half.

## H14 Control And Identity Boundary

Only the original actor 2 at round 14 has a matched pre-action body in both
games: `(16,10),(16,9),(17,8),(18,8)`.
H14 committed SPLIT 2 (action 771/split 772), creating LEN2 child 5 at
`(18,8),(17,8)`. This H14 child also escaped N, W, S in rounds 14, 15, 16.
The R15 evidence is thus an extra retained rear segment after pearl pickup
and changed split allocation; it does not establish that H14's rear child
was unable to escape. H14 parent 2 died in round 16/event 894.

Child numeric IDs after the first divergent action are not matched identities
across games. The earlier raw `pocket_salvage_activation.json` included
numeric-ID-only follow-ups; these are not valid causal controls. The corrected
`pocket_salvage_activation_v2.json` retains only the initial same-body control.

## Seat B And Outcome Limits

Seat B had zero proposal tags and zero mechanism activations. Both bots lost
after 124 rounds. Seat A also remained a loss: R15 was eliminated after 141
rounds, H14 after 106. The round difference is descriptive and cannot be
attributed wholly to this local factor from two games. The parent-owned full
68-game panel had the same win counts as H14; this audit does not overturn it.

## Evidence

- Generator: `pocket_salvage_activation.py`.
- Corrected structured output: `pocket_salvage_activation_v2.json`.
- Candidate A replay SHA256: `9c1e4b203cbf55a80dc074d65bf2f72e9ea206016dca82d4e60c06b34ada5880`.
- H14 A replay SHA256: `fc1f87d4b2b46f7e1fba43fb0f6559a943c4366f56c2e2e653a4ae929c540ee4`.
- Candidate B replay SHA256: `816ba589c7202f796d0e4baeb15e2a95d945c4609fa4cf51fef995a977d238b0`.
- H14 B replay SHA256: `412826d8c6c628f6e3d229c2b9a11d81dbccd17844dc37eff98a2f0d7929c515`.
