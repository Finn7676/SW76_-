# H1 gate: acquire and exploit portal-connected resource regions

Registered before any candidate matches on 2026-09-29. Parent: current
`tyhh_qos_protect_relay`, main.cpp SHA256
`c65cfa425e09b38662610fd19fc309b419e5c82edc2c6e7cddadeab8337b4961`.

## Evidence and hypothesis

Default's actual open-edge graph has one 830-cell start component, two isolated
25-cell rooms, and nine 16-cell rooms connected through portals. This fact is
diagnostic only; the candidate must not read map names, map files, coordinates,
or opponent identity. Existing relay's room-claim cap is 24, so its truthful
claim test cannot recognize the 25-cell rooms. This module does not silently
change the communication claim protocol.

The retained 20260927 Default replay analysis reports relay first crossings at
round 42 (A) and 23 (B), versus scout/v2 round 8 (A) and 15 (B). Yet scout/v2
lost both Default games. Before round 80 relay B had 8 same-dragon/same-portal
repeats within eight rounds without positive growth; scout A had 11 and v2 A
had 6. Those are crossing proxies, not proof that every crossing was wasted.
Source: `protect_vs_v3_20260928/portal_repeat_diagnostics.json`.

Hypothesis H1: early portal access is insufficient when the policy immediately
leaves a useful room or treats a learned portal pair as an acquisition result.
A bounded acquire-and-exploit controller, using only observed open-region
topology and fresh pearl timers, should turn portal entry into resource growth.
It will retain the base combat, champion, late donation, communication, and
teammate-protection rules.

## Single intervention and boundaries

One early portal resource-acquisition module will distinguish:
1. portal endpoint/pair information (topology, not exploration);
2. an actual surviving traversal, confirmed at the following own update;
3. a fully observed open-connected resource region and a legal local pearl route;
4. repeated same-pair crossings, which do not receive repeated exploration priority.

Length 2–8, population at least 6, no visible enemy head, and round <220 bound
the intervention. Known-open approach paths may target an untraversed portal
or a visibly useful small destination region; unresolved exits terminate the
plan rather than becoming fictional intermediate cells. Inside a fully known
small resource region, a local no-portal route to a fresh ripe/soon-ripe pearl
takes precedence over exit exploration. Acquisition travel may postpone the
routine split; resource harvesting retains the original split rule. All moves
still pass the base local teammate-protection review.

The module is evaluated as one factor, not independent causal claims for each
internal step. No full games are run by this child agent. Root owns all paired
development and held-out evaluation under `relay_dual_20260929/PLAN.md`.

## Prespecified falsification

Targeted tests must distinguish pair information from actual traversal, reject
unknown-edge shortcuts, prefer a reachable local resource over immediate portal
return, keep mandatory combat/endgame behavior outside the gate bounds, and
maintain legal first-step occupancy checks.

In paired matches, measure actual room entry, time remaining in a region,
positive growth, and repeat crossings separately. Earlier/more portal crossings
alone do not support H1. The root's dual-opponent win criterion is unchanged;
performance claims require its complete matching data.
