# Champion Role Findings

Evidence: `champion_role_diagnosis.json` reconstructs 102 audited H14 versus
xxqztest replays, three development batches, 17 maps and both seats. Every
replay hash and reconstructed final standing matches the prior audit.
The raw snapshot table is in `champion_role_diagnosis.md`.

## Finding

Initial ID parity is biased, but persistent B-seat deprivation is not
supported by these trajectories. Every map starts A-even/B-odd. Only five
maps give A a nonqueen initial multiple of eight: australia, autarky,
slithery_fight, islands and unsw. B has none initially. Actual split children
do not retain that team parity:

| H14 Seat | Split Births | Even | Odd | ID%8==0 Births |
|---|---:|---:|---:|---:|
| A | 13624 | 6793 | 6831 | 1686 |
| B | 11753 | 5924 | 5829 | 1455 |

The raw-multiple-of-eight share of living nonqueens is A/B 11.4%/12.4% at
250, 12.8%/13.8% at 350, and 15.6%/14.2% at 450. Both sides actually obtain
ID-selected champions. B does not have a systematically smaller raw share
at the two earlier checkpoints. At 450 B has more natural LEN>=12 champions
(60 vs 41 aggregate), and average longest is almost equal (16.54 vs 16.79).
This does not support raw global-ID parity as the primary overall failure
mechanism; growth, survival and map control remain confounded.

Local exceptions remain worth testing rather than silently dismissing:
weakhold B has zero raw champions among only five total nonqueen snapshot
instances at 450, with no natural champions and average longest2. A has8
raw/35 nonqueens, one natural champion and average longest14.67. Conversely,
portals has both A and B raw pools (9/81 and10/86), but neither has any
LEN>=12 nonqueen at450; longest means remain5 and6. Missing resource growth
there is not an absence of modulo champions. Early-ended matches are not
padded forward: only38A/39B games reach450, not all51 per seat.

## Opponent Caveat

xxqztest uses the identical round250, ID%8==0 or LEN>=12 role label. However,
its planner forcibly splits every LEN>=4 dragon back to2 before350 even if
ID-selected, and its donation policy allows selected champions to feed a
strictly longer dragon. Its labels therefore do not imply the same growth
contract as H14's ordinary split suppression. Counts alone cannot establish
an allocation advantage or causal effect on victory.

## Candidate

One isolated candidate would replace only `MYID%8==0` with the fixed
avalanche-mixed global ID slot implemented as `balanced_slot()` in the
diagnosis script. Preserve queen protection, LEN>=12 and round250. The hash
selection does not assume split IDs alternate by team, and is approximately
one-eighth on arbitrary ID cohorts rather than selecting only even IDs.

On unchanged H14 trajectories this counterfactual hash slot is12.0% A and
12.4% B at450, versus raw15.6% and14.2%. These are assignment counts only,
not simulated policy wins; it also removes some accumulated protected
cohort and may harm growth. This is a low-priority mechanism test, not a
demonstrated fix. No bot or opponent was changed, and no matches were run.
