# H14 Economy Diagnosis

Read-only analysis of completed, audited native engine 1.2.7 development
replays. H14 growth uses three fixed bases 82001/83001/84001 against unchanged
xxqztest: 102 games, 17 maps and both seats. R12/H14 behavior comparison uses
the complete 82001 batch against both opponents: 68 paired games. Every
replay SHA and reconstructed final standing was checked. No bot, opponent,
match schedule or reserved-seed result was modified or used. Descriptive
counts only; three bases, no significance test and no causal win claim.

## Reservation Had No Observable Effect

R12/H14 82001 actions, pearl collections, world changes and final standings
are identical in all 68 pairs. Each has 624,428 own actions and 42,816 own
collections. No first action divergence exists. R12 lacks reservation logs,
so this proves zero output effect in this batch, not an empty internal list.

The wide necessary-condition upper bound (visible quiet history, visible
pearl and full actual-length receiver) has only 2,292 opportunities among
189,544 late own actions; 179 are preempted by existing queen/consolidation
feed. Exact sonar quiet, knowable recipient length and topology/escape
proof would only reduce this bound. In xx default A/B there are none; xx
portals B has none, A has two at rounds 498/499. Waiting for a length-eight
receiver before reserving pearls mostly misses these trajectories.

See `mechanisms/r12_h14_82001_action_compare.{md,json,py}` for exact counts,
paired streams and source hashes.

## Growth And Fragmentation

Across 102 H14/xx games there are 5,482 recorded late splits starting at
length four and 1,098 at length five (round at least 350). Of 1,720 nonqueens
first reaching length eight at or after 350, 253 split in their first
following action: 123 request split two, 130 request a larger split. Larger
splits often implement the existing reverse-body escape, not routine growth.

Portals has six games, one win. There are 500 late length-four splits, 163
length-five splits and 120 length-at-least-six splits. Seventy-one distinct
nonqueens ever reach eight: 53 through movement and 18 as split children;
65 first reach eight after 350. Twenty-five split on their first following
action (12 split two, 13 split six). Eleven of those 25 are ID-selected
champions, a reason not to label all 25 as routine fragmentation.

Concrete zero-based round timelines:

- Portals 82001 A, ID 704: grows to eight at 435, reverse-splits six at 436.
  This is not routine growth; a phase gate should preserve this fallback.
- Portals 84001 A, ID 804: born at eight at 427, first own action that same
  round splits two into lengths six/two, POP 26 and no currently visible
  enemy. Its eight-quiet-round age gate is false because it is newborn.
- Portals 84001 B, ID 948: analogous newborn eight at 399, immediately
  splits two at POP 27, no current enemy. A waiting-period gate misses it.

These are opportunities for a counterfactual growth policy, not proof that
retaining the original body survives or changes the winner.

## Gate Coverage

The following official split-two pool requires `350 <= r < 470`, `POP >= 8`
and no `PROTECT_DIRECT` log. It remains an upper bound for routine splitting:
emergency split-two can occur without that log. Champion records are
separately retained, not silently relabeled routine growth.

| Slice | Split-Two Pool | No Current Enemy | No Enemy/Queen Visible | Visible Quiet Eight/No Queen |
|---|---:|---:|---:|---:|
| All 102 games | 7,101 | 4,330 | 23 | 1,976 |
| Portals six games | 668 | 111 | 3 | 10 |

Of the 4,330 no-current-enemy records, 4,277 are nonchampions; 3,075 start
at four and 465 at five. Of portals' 111 records, 110 are nonchampions:
69 start at four, 33 at five, seven at six and two at eight. Visible quiet
eight uses past visible enemy parts only; sonar can further restrict it.

| Portals Base | Seat | Outcome | Pool | Current Enemy | No Current Enemy | Quiet Eight/No Queen |
|---|---|---|---:|---:|---:|---:|
| 82001 | A | Loss | 86 | 73 | 13 | 0 |
| 82001 | B | Loss | 107 | 94 | 13 | 0 |
| 83001 | A | Loss | 93 | 74 | 19 | 5 |
| 83001 | B | Win | 127 | 103 | 24 | 3 |
| 84001 | A | Loss | 144 | 118 | 26 | 1 |
| 84001 | B | Loss | 111 | 95 | 16 | 1 |

A no-current-enemy phase gate covers 11.1 times the portals split pool of
the quiet-eight/no-queen gate, and 2.19 times globally. It still leaves
557/668 portals split-two records with a visible enemy untouched, so it
cannot be presented as a complete portals repair. Neither gate reaches
early-ended stripes or queen_of_spades losses in these batches.

## Single-Factor Recommendation

Test from H14: suppress only ordinary `wantExpand` at round at least 350,
POP at least eight and no current visible enemy part. Preserve queen
bootstrap, low-population replenishment, combat branches, emergency splits,
all portal protection and existing donation/death acceptance. Current
absence of an enemy is a growth preference, not a safe-donation assertion.
This differs from R2, which holds receivers only after witnessed donor
eligibility; the proposed gate acts before a receiver has been grown.

H14 is the clean parent for this evidence and isolation. A later R10 child
can test combination, but since R10's sprint branch follows ordinary split,
suppressing growth also exposes additional attacks: report that interaction
rather than calling it a pure length-preservation effect.

Risks: fewer collectors, poorer exploration and resource intake, transient
out-of-view enemies, unsafe future body geometry, and less swarm recovery.
The first development screen must compare identical schedules and frozen
sources, and retain losses. Legal retained growth is not guaranteed survival.

## Provenance

`h14_late_growth.{py,json,md}` contains growth histories and split-body
snapshots. The initial `v1_alias_bug` analysis is retained: it accidentally
kept mutable body lists in split-result output. The script was corrected
and all 102 games rerun; action/length counts remained unchanged. Do not
use that initial file's split-body snapshots.

`h14_late_split_layers.{py,json,md}` independently reconstructs the requested
split strata and all portal per-game records. Script SHA-256:
`9c59381cd965b3cf4c1f557c05e9b21d823afc682da0fa3423c077a5e10abf70`.
Growth script SHA-256:
`34933aaf19cf1a5da80f4744b0db884eb43e22b61807fcd98bd2ad03e03d87e1`.
