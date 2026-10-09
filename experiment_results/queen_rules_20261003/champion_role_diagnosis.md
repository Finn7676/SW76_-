# Champion Role Diagnosis

Read-only reconstruction of H14 versus unchanged xxqztest from three audited
ranked17 development batches (82001, 83001, 84001), both seats. Snapshots
are after 250/350/450 complete rounds. Counts exclude fixed queens.

| Round | Bot | Seat | Games | Units Mean | Longest Mean | Raw ID%8 | Natural LEN12 | Raw Fraction | Zero-Raw Games |
|---:|---|---|---:|---:|---:|---:|---:|---:|---:|
| 250 | H14 | A | 40 | 32.50 | 10.38 | 146 | 27 | 11.4% | 6 |
| 250 | H14 | B | 40 | 29.20 | 7.62 | 144 | 29 | 12.4% | 8 |
| 250 | xxqztest | A | 40 | 35.95 | 4.88 | 180 | 4 | 12.6% | 9 |
| 250 | xxqztest | B | 40 | 31.30 | 4.42 | 172 | 3 | 13.8% | 4 |
| 350 | H14 | A | 39 | 34.38 | 10.97 | 170 | 24 | 12.8% | 7 |
| 350 | H14 | B | 40 | 31.48 | 9.55 | 173 | 43 | 13.8% | 6 |
| 350 | xxqztest | A | 40 | 39.77 | 5.65 | 200 | 9 | 12.6% | 6 |
| 350 | xxqztest | B | 39 | 37.72 | 4.92 | 214 | 7 | 14.6% | 5 |
| 450 | H14 | A | 38 | 30.24 | 16.79 | 178 | 41 | 15.6% | 2 |
| 450 | H14 | B | 39 | 27.05 | 16.54 | 149 | 60 | 14.2% | 7 |
| 450 | xxqztest | A | 39 | 32.10 | 11.44 | 215 | 32 | 17.2% | 4 |
| 450 | xxqztest | B | 38 | 30.53 | 12.32 | 194 | 30 | 16.8% | 6 |

Raw and natural pools overlap; summing them would double count. Longest
includes the queen; nonqueen longest and full ID/length lists are in JSON.
A completed early-elimination match is not silently carried forward as a
250/350/450 snapshot. Its omission is explicit in games-reaching-round.

## Scope

Both H14 and xxqztest use the raw global ID modulo-eight criterion after
round 250, alongside LEN>=12; H14 additionally always protects its queen.
The raw role changes splitting, combat willingness and scoring, but this
diagnosis does not establish that the ID imbalance causes any loss.
Actual split births are recorded rather than assuming every later ID keeps
the initial team parity. Survival itself can also bias residue frequency.

## Single-Factor Candidate

Replace only H14's MYID%8==0 slot with a fixed 32-bit avalanche hash of
global MYID selecting low-three-bit zero. Preserve ROUND>=250, LEN>=12,
queen protection and every other policy. This decouples selection from
initial even/odd allocation without depending on hidden team population or
assuming split IDs alternate by team. The proposed mixer is explicitly
recorded in balanced_slot() in this script; its counterfactual counts on
the existing trajectories are not actual changed-policy results.
Changing the protected cohort can still reduce exploration or cause poor
growth; only paired same-seed frozen matches can determine its benefit.
No bots, opponents or match artifacts were modified. No matches were run.
