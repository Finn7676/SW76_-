# Completed Registered Goal

Objective: both unchanged opponents beaten under1.2.7 queen rules. Development
qualification: separately over50% wins vs each opponent in fullranked17 A/B
schedule. Verification: freeze then at least3 unused matched seed batches,
separately above50% overall against each; report per-seed range/mean/std.

No production replacement or opponent mutation. Keep all defeated candidates
and replays. A safe correction is not automatically a winning strategy.

Initial bests before newgoal: queen_v127 V3 14/34, queen_feed_v127 xx18/34.
Neither qualifies jointly. Hypotheses H3/H4/H5 registered in NEXT_EXPERIMENTS.

Current development: H3 audited15/34 V3,16/34 xx; H5 consolidation16/34
V3,21/34 xx; H8 maturity35018/34 V3,16/34 xx. H9 feed+maturity18/34,
17/34; H11 consolidation+maturity17/34,18/34. None is jointly over50%.
H4,H6,H7 and maturity300 sensitivity fail the joint gate; retained audited.
H14 final queen guard + consolidation qualified on audited development:
18/34V3 and19/34xx, old10subset23/40. Source is frozen; reserved independent
seeds79001,80001,81001 now enter confirmation with original counter reference.
See CONFIRMATION_PLAN.md. Confirmation has now passed the registered aggregate
gate: frozen H14 V3 56/102(54.9%) andxx54/102(52.9%), compared with original
counter37/102 and45/102. All408 confirmation candidate/reference games audited
without failures. Perbatch rates mean+/-std:54.9%+/-7.4%,52.9%+/-5.1%.
No statistical test or reliable all-map/side victory claim. SideB24/51 each;
portals versusxx0/6 anddefault remains weak. See h14_confirmation_final.md.
H5 portals sandbox smoke passed4games without runtimefailure, all4losses.

Actual H5 mechanism observed in official replay events:2025feed donor deaths,
1837with at leastone pearl collected by intended recipient,2326intended
recipient pearl collections. See consolidate_mechanism.json for the evidence;
no whole-corpse or reliable queen-survival inference is made.

H14 sandbox4portals games also audited:0wins, no runtimefailure; native and
sandbox scoring/death state match. Full-map sandbox compliance remains unknown.
H14 source-bound guard, consolidation and updated-rule regression tests pass.
H15 independent scout-route candidate is compiled/tested but NOT competitively
evaluated; retained as follow-up. No production replacement occurred.

Development seed base78001; independent verification seed bases
79001,80001,81001 (one fixedseed offset per ranked map and bothsides).
Do not inspect verification results before candidatefreeze or describe later
tuning on these seeds as heldout. If a candidatefails confirmation, reuse is
development only; reserve new unused bases for a later frozen finalist.
