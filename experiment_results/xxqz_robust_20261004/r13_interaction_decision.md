# R13 Development Decision

All four factorial cells completed and audited on bases 82001/83001/84001.
Ranked17 maps, both seats, native unswbc1.2.7, unchanged opponents and fixed
maps/engine are verified by checkpoint_r13.json. Draws remain non-wins.
Three development batches: descriptive statistics only, no p-value.

| Target Value | Order | xx Wins | V3 Wins | xx A | xx B |
|---|---|---:|---:|---:|---:|
| Ordinary length worth (R4) | Growth before sprint | 55/102 | 53/102 | 33/51 | 22/51 |
| Ordinary length worth (R8) | Sprint before growth | 55/102 | 54/102 | 32/51 | 23/51 |
| Queen priority (R10) | Growth before sprint | 56/102 | 53/102 | 33/51 | 23/51 |
| Queen priority (R13) | Sprint before growth | 56/102 | 54/102 | 32/51 | 24/51 |

Moving sprint earlier adds zero pooled xx wins under either target-value
condition. Queen priority adds one pooled xx win under either order.
The pooled count interaction is zero; this does not prove no individual
map/seed interaction. R13 first-batch20/34 falls to18/34 in both remaining
batches. R10 is19/34,19/34,18/34. The first-batch apparent additive gain does
not establish a robust pooled gain. Relative to H14, both obtain four more
xx wins, with paired gains/losses R10 six/two and R13 seven/three.

Neither clears the predefined preference: pooled54.9% is below>55%, and
R13 B24/51 (R10 B23/51) is below>50%. Do not promote either as an established
cross-seed winner or relax the gates. Reserved91001/92001/93001 remain unused.

Mechanism evidence at82001 shows more initiated queen hits, but identical
overall opponent-queen death distribution. Earlier attack changes downstream
body/corpse/resource control, not reliably winner selection. See
combatfirst_r10_82001_mechanism.md for exact recorded events and regressions.

Next isolated tests: R16 relaxes only length-two entry to the same exact
attack search; R15 addresses a separately diagnosed terminal-pocket split
failure. R11 remote rally screened negative (xx15/34 vs H14 16/34), R12
local reservation unchanged (16/34); neither expands based on this screen.
R14 corrects reservation safety before considering any performance benefit.
