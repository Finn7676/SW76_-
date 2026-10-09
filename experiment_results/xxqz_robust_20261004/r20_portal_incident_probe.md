# Queen Incoming Portal Evidence

Read-only diagnosis of the changed R20 islands seed82012 B game, official
unswbc1.2.7. No strategy implementation or competitive match ran. Input and
counterfactual evidence are `r20_portal_incident_probe.json` and its two
fixture replays. R20 replay SHA256 is
`7106610fddda33580282fcb5eb69a94f011df1a43d6152d8aba8bd64f813ca2a`.

## Actual Input

At queen1 round32, actual head is `(45,9)`, heading W, length6, population5,
zero messages; body is `[(45,9),(46,9),(46,10),(46,11),(45,11),(44,11)]`.
Official raw input SHA256 is
`6d058185a1329fe26a708c76fb33e986c424f625c2f921719eb9702e4b78a6bc`.
All own body cells are currently visible. The only visible enemy is id16,
with head `(48,9)` and body segment `(48,10)`. No other friendly segment is
visible. Friendly12's remote head `(31,16)` is outside current vision.

The actual `MOVE S` is ordinary and ends at `(45,10)`. Its SOUTH edge is
currently shown PORTAL pid1 in the queen's raw input. Thus the queen can know
that this endpoint has an incoming portal channel without knowing the remote
endpoint, a remote dragon, its action intent, or a globally next actor.

## Executable Alternate

Current-view ordinary `MOVE N` ends at `(45,8)`, retains length6, avoids all
current bodies and has visible ordinary N continuation to `(45,7)`. All four
edges at the new head are known and none is a portal. W to `(44,9)` also
meets those local properties. The local search found nine one/two-free-step
same-length nonportal-incident alternatives; this is not an enemy-reach or
global-safety certificate.

Two official action-tape arms regenerate the same4647 physical/SONAR events
before queen's round32 action, with byte-identical official raw queen input.
Actual S and alternate N use all recorded earlier actions/SONAR, then hold
friendly12's actual `MOVE N` fixed. Actual S causes queen1 and friendly12 to
die `H` at32. Alternate N causes neither to die at32: friendly12 lands through
the portal at `(45,10)` without hitting the queen. Later ENDTURN cleanup is
deliberate fixture behavior, not a live bot policy or long-run result.

## Candidate Boundary

A technically feasible isolated hypothesis is a final queen endpoint review,
not a remote-friendly-intent predictor: when an already living ordinary move
ends next to a currently visible portal edge, prefer an existing same-length,
known ordinary alternative whose endpoint has all edges known/nonportal,
exact projected-body survival, ordinary continuation and no hard/soft friendly
risk. Preserve the original when no proved alternative exists. Do not rerun
the generic scorer with a penalty, which can select an unreviewed fallback.
Keep current split, donation, unknown/portal movement and attacks unchanged.

Portal avoidance is a preference under uncertainty, not proof that an incoming
dragon exists. It can steer queens away from productive portal neighborhoods
or safer chokepoints, and hidden ordinary attackers remain possible. The input
supports endpoint-incidence detection, but this one R20-derived snapshot does
not establish prevalence in H14 or justify broad production adoption. Such a
guard would need separate H14-parent source fixtures and paired multi-map,
both-side, multi-seed evaluation. No reserved seed was inspected.

Reproduction, with exclusive output names handled by the script:

```bash
.venv-battlecode/bin/python experiment_results/xxqz_robust_20261004/r20_portal_incident_probe.py
```

Existing evidence is never overwritten. The fixed script targets the saved
development replay and refuses to replace its JSON/replays.
