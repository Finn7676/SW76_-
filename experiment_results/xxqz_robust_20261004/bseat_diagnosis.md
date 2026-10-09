# H14 B-Seat Diagnosis

Read-only reconstruction of 51 existing, audited H14 versus unchanged xxqztest
native 1.2.7 games, bases 82001/83001/84001, B seat only: 22 wins, 29 losses.
Replay SHA256 and final standings were checked. These are development seeds,
three bases, descriptive statistics only; no significance or causal win claim.
No bot, opponent, map or competitive match was changed during diagnosis.

## Failure Timing

Of 29 losses, 12 ended by elimination before 500 rounds. Seventeen reached
500; two of those lost to a living enemy queen. Five 500-round losses ended
with H14 total length above xx; four of those lost on longest dragon, one
on queen. Thus resource acquisition and resource concentration are separate
failure slices, not one universal explanation.

The table uses only losses still alive at the checkpoint, before round r.
It does not pad early-ended games or treat different maps as seed replicates.

| Checkpoint | Games | Mean H14-minus-xx Total | Population | Longest | Fewer H14 Pickups | Fewer H14 View-Union Cells |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 20 | 29 | +0.14 | -1.48 | +2.52 | 16/29 | 19/29 |
| 50 | 28 | -10.39 | -6.07 | +2.96 | 22/28 | 17/28 |
| 100 | 28 | -38.71 | -15.57 | +1.75 | 25/28 | 18/28 |

View union is an offline team union of radius-three observations, not a
dragon's private memory. Pickups require an official pearl removal followed
by that acting dragon's successful move to the cell. Neither count proves
that faster births caused subsequent resource advantage.

## Queen Versus Nonqueen Births

| Checkpoint/Loss Games | xx Queen Births | H14 Queen Births | xx Nonqueen Births | H14 Nonqueen Births | xx Deaths | H14 Deaths |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 20 / 29 | 27 | 2 | 116 | 76 | 52 | 30 |
| 50 / 28 | 51 | 3 | 431 | 241 | 187 | 119 |
| 100 / 28 | 64 | 6 | 1285 | 639 | 672 | 404 |

At round 20, queen-origin births account for 25 of the 65-birth gap, and
nonqueen births account for 40. At round 50, the nonqueen gap is 190 of 238.
This is a decomposition of observed births, not attribution to one policy:
earlier queen/resource/death trajectories can influence later nonqueen births.
The proposed experiment leaves queen bootstrap and survival unchanged.

## Three Executable Witnesses

1. **Slithery fight, seed 82009 B, initial nonqueen seven.** xx id2 round0
   SPLIT5/action429/split430 leaves parent `[(8,10),(8,11)]` and child16
   `[(8,16),(8,15),(8,14),(8,13),(8,12)]`. Child16 SPLIT3 in the same
   round/action515/split516, then child23 moves W/action571. H14 initial
   counterpart id3 SPLIT2/action434/split435 leaves parent five at
   `(54,16)..(54,12)` and child17 two at `(54,10),(54,11)`. H14 bulk only
   splits again in round1/action709/split710. Both queens make one birth
   before20 and both die; at20 xx/H14 nonqueen births are29/16, populations
   21/15, totals45/55. This proves allocation/timing differs, not a win effect.

2. **Autarky, seed 83002 B, initial nonqueen fourteen.** Both queens round0
   MOVE N/actions226/230. xx nonqueen2 SPLIT12/action237/split238 produces
   parent2 and child12LEN12; descendants split10/8/6/4/2 at events297/313/
   320/327/334, all round0; final child18 moves N/event340. H14 nonqueen3
   SPLIT2/action242/split243 leaves parentLEN12 at `(35,15)..(35,4)` and
   child13LEN2 `[(35,2),(35,3)]`; parent repeats SPLIT2 only in rounds1..4.
   At20 births11/8, populations9/8, total20/23 (xx/H14). Final H14 total57
   exceeds xx53, yet longest5 loses to8. Initial tail is outside current
   radius-three view; a whole-body-visible candidate would not cover this.

3. **Trauma, seed 84010 B, local terminal-pearl loss with an alternate.**
   Nonqueen75 round188 at `[(34,21),(33,21)]` commits S/action27433 and
   picks `(34,22)`/move27435 toLEN3. Three exits are kelp, the north exit
   is its own neck. It self-dies at round189/event27659. Visible ordinary
   E `(35,21)` then S `(35,22)` is clear, with no visible enemy part at the
   decision. Equivalent repetitions occur337/443/454. Across51 games this
   strict pattern appears17 times: four in this loss, thirteen in wins.
   It is a small local repair opportunity, not the main B-seat explanation.
   Unlike rejected R15's first design, these witnesses have an actual alternate.

## Priority Single Factor

R21: from H14, only inside its already-approved ordinary growth split branch,
before350, nonqueenLEN>=5, prefer childLEN-2 when `can_split(LEN-2)` and
`splitSafe(LEN-2)` pass; otherwise preserve SPLIT2. Keep original wantExpand,
cap, large-map limit, original split2 gate, action order, queen policies,
emergency splits, portal safety, combat and donation untouched. No R20 mix.

| Candidate | Factor | Fixed Protocol | Expected Mechanism |
| --- | --- | --- | --- |
| H14 control | ordinary child size2 | frozen official1.2.7/maps/opponents/seats/seeds | bulk parent waits until next round |
| R21 | qualified early childLEN-2 | same control protocol | same-round rear child continuation/cascade |

In the29-loss slice there are337 actual pre350 nonqueenSPLIT2 events with
official true prelength>=5 (136>=6): portals146, slithery161, maze11,
autarky5, tower_defense6, islands5, stripes/default/trophy one each.
This is a necessary-condition exposure pool; emergency/protection branches
may be present, so it is not an exact ordinary-branch activation count.

Risks: retained parent2 loses length/free-step budget; newborns may split
again without clearing original occupied cells, increasing congestion;
rear reversal redirects exploration; legal split does not prove escape;
faster short swarms may lose safe pearl collectors or later feeding carriers.
xx slithery parents2/16 actually self-die in round1, while H14 bulky parent3
survives at least through4. Growth timing is not universal survival.

Validation: source-bound H14 controls for queens, LEN4, round350, cap,
portal-bridged parent2 and unchanged emergency behavior; official engine
fixtures for LEN7/14 exact arrays and same-round child actions before games.
Root owns the frozen68-game first screen. Expected prior CPU-only budget is
about3.2minutes at four native workers, roughly250MB storage; no API/GPU.
Primary remains wins versus each opponent, separately report A/B, map and
paired promotions/regressions. Mechanism metrics include committed split
allocation, birth timing, parent/child deaths and directly observed pickups.

## Artifacts And Corrections

`bseat_diagnosis.{py,json}` reconstruct checkpoints and terminal witnesses.
`bseat_early_split_probe_v2.json` records copied official split arrays and
queen/nonqueen birth decomposition. The first `bseat_early_split_probe.json`
is preserved: its body arrays aliased mutable live bodies, so do not use its
split snapshots. Birth/death counts did not depend on those references and
were rechecked in corrected v2. Initial pre-round position events can duplicate
an initial head in generic replay reconstruction; true split prebody is rebuilt
as official parent array plus reversed official child array in v2.

No reserved confirmation seeds were read. Existing skills `analyze-results`
and `experiment-design` guided the descriptive scope and single-factor choice.
