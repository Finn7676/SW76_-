# R11 Remote Rally Navigation

Registered before source changes or competitive evaluation. Parent is frozen
R3 `tyhh_qos_robust_donor_route_v127`, main SHA-256
`91a65ac8a9fb7656f1631d5a0ebda7c1905e7bdafc2ee66240b8b081f398ecee`.

Hypothesis: R3 requires a recipient head in the current 7x7 view, so sparse
quiet regions cannot rendezvous with an existing longer teammate. Fresh
team status may supply a navigation goal without relaxing donation safety.

The isolated factor is a remote navigation fallback after unchanged R3
queen feeding, consolidation and visible-donor routing. Existing status
reports decoded by R3 are cached for at most six rounds in a separate store.
They do not enter occupancy, `friendLength`, protection verification or
suicide eligibility. A visible report cell must still contain the reported
friendly head; visible recipient length uses unchanged `friendLength`.
Remote recipients must be nonqueen, lower ID than the donor, and have
reported length at least eight and at least donor length plus two. Lower-ID
ordering avoids reciprocal pursuit even with stale length estimates.

Routing searches known ordinary edges or resolved portals to a reported
head or its known adjacent cell, at depth at most 12 and 256 expanded nodes.
Unknown edges are never guessed. Hidden future occupancy is not asserted
free: only the first currently visible step is emitted, then the graph is
recomputed next turn. The emitted step requires complete own-body positions,
exact known survival, portal permission, no direct/hard/soft friendly risk,
and a proven visible continuation. Protected dragons retain the original
strict portal landing rule. Every remote proposal is one MOVE, never a paid
sprint; unchanged post-planning free harvest can still extend a proposal
using verified free steps.

The same target must show a reduced graph routing distance within six
rounds; otherwise it is ignored for 12 rounds and baseline behavior resumes.
No enemy part may currently be visible, and original quiet/population,
length and collection-horizon conditions remain required. No qualifying
status or no safe route leaves R3's proposal unchanged. Emergency survival,
queen scoring, ordinary split schedule and all donation gates are unchanged.

Focused source-bound candidate and R3-control tests cover hidden-cue routing
without donation, current-visible contradiction, freshness/team decoding,
queen/length/ID/enemy rejection, unknown and unresolved portals, protected
portal rejection, complete body/current collision/continuation/friend risk,
single-step output, no-progress timeout and baseline split fallback.
Inherited R3 donor-route regression tests and native compilation are required.
No matches are launched here; root owns frozen paired evaluation and audit.
