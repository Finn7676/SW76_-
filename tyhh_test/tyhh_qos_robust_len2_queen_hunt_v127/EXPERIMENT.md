# R16 Length-Two Exact Sprint Eligibility

Registered before competitive games on 2026-10-05. Parent: frozen R13,
`tyhh_qos_robust_queen_combatfirst_v127`; parent main SHA-256:
`0f9e2dd03bae32ce2a9113e283cbf4ed3822ee3f37f48ab36eecdcd178574830`.

## Isolated Factor

The only production difference is the `sprintRamAction` entry rejection:
`LEN < 3` becomes `LEN < 2`. All queen/champion/population/body gates,
target-value preferences, initial free-step calculation, four-step/256-node
limits, paid-step eligibility, exact projection, ordinary-visible topology
restriction and R13 combat-before-split order remain identical.

Hypothesis: a length-two scout can collect a pearl on its first free step,
thereby reaching length three before a paid attack on the next step. The
original entry gate excludes that legal opportunity. Unfunded paid steps
must remain unavailable, including when the attempted paid step itself has
a pearl. This factor also permits immediate length-two attacks admitted by
the unchanged queen/ordinary worth predicate; it is not a queen-only gate.
Risk: additional short-scout trades can reduce collectors or transfer
corpse resources to opponents. Legal extra attacks do not imply more wins.

## Validation And Evaluation

Before games: production compile; candidate and actual frozen R13-control
fixtures for funded and unfunded length-two routes, own/static bodies,
portals/unknown visibility, population/champion/queen/full-body gates,
unchanged ordinary worth, and 200 deterministic length-at-least-three
action controls. Keep inherited exact-sprint, queen-value and combat-order
tests. No copied parent planner supplies the R13 control.

Initial development screen: same ranked 17 maps, A/B, unchanged `xxqztest`
and `V3_portal`, base seed 82001 (68 native games). Compare to R13 and H14
on exact schedule and audited engine/source identities. Only promising
results expand to preregistered 83001/84001. Reserved 91001/92001/93001
remain unread. No opponent, parent or production bot is modified.

Root owns match launching and outcome analysis. This preparation starts no
games. One screen adds approximately a few CPU-only minutes and 0.5 GB;
no GPU/API cost, unchanged shared maximum four matches.
