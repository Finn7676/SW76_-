# R25: Known Terminal Short-Pearl Review

Design only, registered2026-10-08 before source implementation or competitive
matches. Parent is frozen H14 `tyhh_qos_counter_queen_guard_consolidate_v127`,
main SHA `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.
At registration no candidate production source, READY, binary or match
result existed. Implementation now lives separately in
`tyhh_qos_robust_short_terminal_v127`; validation is pending and no
competitive match has been launched.
R24 room-proof bound is a separate factor and must not be combined here.

## Evidence And Question

`h14_terminal_body_probe.{py,json,md}` reconstruct102audited H14/unchanged-xx
games on native1.2.7, bases82001/83001/84001, ranked17 maps and both seats.
1047recorded nonqueenLEN2 ordinary single-step pearl pickups produce a
complete visible LEN3 head whose exits are only kelp or its own postbody.
98have a currently ordinary living alternate with a next-turn continuation;
38also have no current visible enemy part. All38actually self-die next round.
These38cover14games, including7events in3losses. The wider enemy-visible
slice is diagnostic only and is not R25's registered mechanism.

Question: does replacing these proved quiet terminal pickups preserve useful
collectors/resources rather than spending them on a pearl that cannot escape?
Prediction: source-bound witness picks a reviewed ordinary alternate; native
counterfactual removes the immediate neck death, while competition measures
collector survival/pickups and win changes separately. Refutation: no actual
action change after existing protection, harmful displacement/crowding, or
paired wins do not improve. Do not generalize38events to all dead-end losses.

## Isolated Factor

Review only an already chosen action after H14 planning/free-harvest and
protection, before emission. Entry requires nonqueenLEN2, one direction,
no split or donation, a known ordinary currently visible unoccupied pearl
destination, no currently visible enemy part, complete unique ordinary own
body, and exact existing projection toLEN3. For all four posthead directions,
require either known kelp or a current-view ordinary destination in the
projected own body. A portal/unknown exit, other-body blockage, partial own
body or unavailable pearl is not a terminal proof.

Search only one-step alternatives on known ordinary currently visible cells.
Require exact current-body survival, no direct friendly collision, no existing
hard/soft protection risk, and at least one current-view ordinary next-turn
continuation that avoids the projected own body/current other bodies and is
not itself terminal by the same wall/own-body proof. Select directly from the
reviewed alternatives using unchanged scoring/tie order. Do not add a global
score penalty or re-enter an unreviewed scorer/fallback. Preserve the original
proposal when no admitted replacement exists.

Keep queen/portal/unknown movement, early exploration, all split branches,
donation acceptance, attacks, sprint/payment, action order and late-growth
policy unchanged. This is not R15'sLEN4/5 pre-split pocket salvage, R5's
ordinary-split parent gate, a movement-body-model rewrite or an enemy-visible
evasion policy. The first implementation must document the exact final-review
call site and demonstrate that later inherited reviews do not reintroduce the
terminal action. No protection-risk bypass is permitted.

## Earliest Quiet Loss Fixture

Among covered losses, select minimum round, then base/map/seat/event before
implementation: trauma seed84010 B, round188, id75, official action27433.
Input body `[(34,21),(33,21)]`; H14 `MOVE S` eats `(34,22)` and produces
`[(34,22),(34,21),(33,21)]`. All E/S/W exits at `(34,22)` are kelp; N is
the own neck. Recorded next round189 action27658 `MOVE N` self-dies27659.
Observable ordinary alternate E to `(35,21)` leavesLEN2 and has next-turn
S to `(35,22)`. There are no current visible enemy parts. This is not a
same-turn `MOVE ES`: length2 has one initial free step and cannot fund the
second step here.

`r25_terminal_fixture.py` extracts the earliest witness and reuses the official
EngineModule/action-tape observation machinery. It records replay/map/engine
hashes, target spawn/raw input, source physical/SONAR/action prefix, true body
and ordinary local topology. Initial repeated position updates are skipped;
the original full final standings must match the audit. No bot is called.
Fixture outputs are exclusive and refuse replacement. This preparation alone
does not prove that the candidate or H14 chooses the expected action from a
fresh process with missing persistent state.

The source-bound input is saved at
`r25_terminal_actual_state.json`; its target raw-input SHA is
`ef5eff379593964e41aed03e42432a9f5f2e660f08d58e2c84c5705346170163` and the
exact pre-action canonical physical/SONAR/action prefix has27385events. Fresh
native source-bound processes separately returned H14 `MOVE S` and R25
`MOVE E` (`verification/r25_terminal_native.json`). The official scripted
two-arm check is complete in
`tyhh_test/tyhh_qos_robust_short_terminal_v127/verification/r25_terminal_rules.json`:
the original `S,N` arm eats then dies self at round189, while the alternate
`E,S` arm performs two successful ordinary updates at length2 and has no actor
75 death through the evidence window. Both arms share the target round188 raw
input and prefix; post-alternative round189 input is intentionally different
because the body is different. Other actors' recorded actions/SONAR are held
fixed. This is engine legality/local survival evidence, not a live-policy or
win-rate result.

## Experiment Matrix

| Run | Factor | Fixed Protocol | Status |
| --- | --- | --- | --- |
| H14 | retain original terminal pickup | frozen1.2.7/17maps/A-B/unchanged xx and V3 | existing development baseline |
| R25 | reviewed quietLEN2 terminal pickup replacement | identical source except registered review | design and fixture preparation only |

## Validation Before Games

1. Candidate/H14 source-bound fixture on the saved actual input, with true
   source-created trail/occupancy initialization rather than hidden geometry.
   Require H14 originalS and R25 admittedE at the same decision. Log output
   only from focused tests, not a new policy factor.
2. Official action-tape original/alternate arms with byte-identical target
   raw input and exact preceding physical/SONAR/action events. Hold recorded
   other actors fixed through the next id75 turn; compare originalS thenN
   against alternateE thenS. That continuation is a scripted rule fixture,
   not a live-policy prediction or full-game benefit.
3. Controls: queen, donation, split, LEN3/4, nonpearl, safe pearl, no alternate,
   partial/duplicate/portal-bridged own body, unknown/portal postexit, current
   enemy, occupied/friendly-risk alternative, and unchanged inherited tests.
4. ProductionO2 compile and independent review before source freeze or READY.

## Execution And Resources

Preparation uses CPU-only official engine callbacks, no GPU/API. One short
fixture has about30krecorded events and small input/replay output. If all
validation passes, root may register a single68game82001 development screen,
roughly3-6CPU-only wallminutes with globally locked4workers and0.4GB storage.
No83001/84001 expansion without interpretable xx gain; reserve91001/92001/
93001 remain unopened. All negative sources, fixtures and replays stay.

Preparation command:

```bash
.venv-battlecode/bin/python experiment_results/xxqz_robust_20261004/r25_terminal_fixture.py
```

Competitive command is intentionally not executable until a candidate exists
and is frozen; then use the unchanged run_suite.py label/candidate convention.

## Analysis Plan

Primary: paired wins versusxx/34, draws nonwins; V3 regression separately.
Report map, A/B, actual failure/audit status and gained/lost wins vsH14.
Mechanism: committed terminal replacements, immediate/next-round deaths,
later pickups and resource retention; distinguish actual activation from the
offline necessary-condition exposure98/38. No causal winner claim from a
saved local fixture; no held-out stability claim from development seeds.
If expanded to three development bases, only means/sample std/range, no
p-values. Do not pool distinct candidates or relax the predeclared gates.
