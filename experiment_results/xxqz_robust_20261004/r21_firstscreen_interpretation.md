# R21 First-Screen Interpretation And Event Examples

Scope: existing complete audited native1.2.7 base82001 suites, ranked17 maps,
both seats, unchanged V3_portal and xxqztest. This supplement makes no
cross-seed, held-out, or production-promotion claim. No match was launched
for this analysis and no frozen candidate was edited.

## Source And Parsing Binding

Removing exactly the five added ordinary-branch lines from frozen R21
`main.cpp` gives frozen H14 `main.cpp`; shared `helper.hpp`, `queen_feed.hpp`,
`consolidate.hpp` and `bot.toml` hashes are identical. Frozen main hashes:

- H14: `ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`
- R21: `eb364e6e9ccf138a4b3b12d071e356a2ecee6300a4b0209675fe6888382f5573`

Independent parser audit and reproducible geometry check are in
`r21_firstscreen_parser_audit.md` and `r21_firstscreen_geometry_check.py/json`.
The check covers17 maps and78,324 map-connection comparisons,14 initial
position records and23 exact early H14 split/action/prebody records.

Full mechanism parser SHA256:
`b0c28698c4752c0991b9463a239d793f028b6080913f4abf3cca4e11399112dd`.
Full output JSON SHA256:
`ef32f64c94662485517285fc1f098a4fe5dde2235bb756fa3b6a57dc728bcd83`.

## Findings And Limits

Thirty matched games have identical complete committed-action and
physical/SONAR histories. The other38 each first differ at an actual
H14 SPLIT2 versus R21 SPLIT(LEN-2), with identical preceding physical/SONAR
history, actor, true prebody and population. These38 observed first effects
are a game-level lower bound, not a complete ordinary-branch activation
count. Sixteen first effects occur in round0; all16 have actual same-round
newborn descendant splits. Later numerical child IDs are only used for
lineage within their own game.

The raw early nonqueen LEN>=5 split pool is2545 H14 events and2718 R21
events. It includes emergency/protection behavior and must not be called an
ordinary activation pool. Among the parent-LEN2 allocations in this pool,
all1175 H14 and2589 R21 pairs have actual ordinary-edge connections, with
zero map-matched portal links. This screen contains no observed parent-LEN2
portal-straddle allocation; it does not prove missing private neck knowledge
or unseen portal geometry universally safe.

The first-screen round20 economic-growth hypothesis is not supported by the
pooled checkpoint means. Own R21-minus-H14 population/total means are
-0.06/-0.12 versus xx seatA and-0.41/-0.88 versus xx seatB (17 matched games
each). The seatB means remain-1.29/-1.65 at50 and-1.12/-3.31 at100, with16
matched games reaching100. These are checkpoint observations, not estimates
of a universal strategy effect. Early-ended trajectories are not padded.

Death tallies in the mechanism report are allocation-linked actual death
witnesses, not automatically distinct deaths. Deduplication by
(opponent,map,seed,seat,death-event) happens to give the same totals here:
1035 H14 (1004 self,25 body,6 head) and1192 R21 (1150 self,30 body,12 head)
within the allocation's same or following round. No invalid-action label is
used as evidence of a portal death. The parser's `NO_VALID_ACTION` label
literally represents a missing action pointer, not an inferred death cause;
engine/dragon log records retain text/event ordering but omit their raw ID.
These schema-fidelity limitations are documented by the independent audit.

## Representative Paired Events

### xx Slithery, Seed82009, SeatB

Both games share the entire prefix through action434, round0, original
nonqueen3 of LEN7. H14 actual split435 allocates parent5/child17LEN2; R21
actual split435 allocates parent2/child17LEN5. In R21 child17 actually
SPLIT3 at action523/split524 in the same round. Its descendant24 actually
MOVE W at action579/update580, head(53,14), LEN3. H14 child17 instead
MOVE N at action523/update524, head(54,9), LEN2. This proves a different
allocation and same-round moving-scout location, not a causal win narrative.

H14 ends with own longest15/total159 versus xx28/166; R21 ends40/215 versus
xx35/151, an observed loss-to-win conversion. At round20 R21 has13 dragons
and total52, versus H14's15 and55. The winning trajectory therefore cannot
be described as an already established round20 population/resource increase.
The corresponding seatA slithery game also changes loss-to-win and has a
round0 LEN7 SPLIT2-to-SPLIT5 first effect, but is a separate trajectory.

### xx Schooltime, Seed82008, SeatB

The common prefix lasts until action40492, round140, nonqueen65 of LEN15.
H14 split40493 leaves parent13/child185LEN2; R21 split40493 leaves
parent2/child185LEN13. No enemy part is currently visible at that turn.
H14 child185 MOVE W at action40988/update40990 and grows to3. R21 child185
instead actually SPLIT11 at action40988/split40989; descendant188 MOVE E
at action41009/update41011, head(21,15), and grows to12. Descendant188 later
dies with actual self reason at round142/event42417 after further evolution;
this is not a portal or invalid-action death. R21 original parent65 dies
with self reason at round150/event47631; H14 original parent65 dies with
head reason at round158/event53274. These later paths are not cross-ID
matched after the first divergence.

Both queens finish LEN3. H14 own longest43 exceeds xx31 and wins; R21 own
longest32 is below xx44 and loses. Own totals332 versus210 also differ.
The earlier round20 checkpoint is identical in both arms, since the first
factor effect does not occur until140. The observed late-medium allocation
regression is a boundary to inspect, not evidence to retune schooltime alone.

### xx UNSW, Seed82016, SeatB

First action464 in round0 changes original nonqueen19 LEN10 from H14
SPLIT2 to R21 SPLIT8; actual split465 matches both requests. R21 newborn26
SPLIT6 at action516/split517, then descendant28 SPLIT4 at split532, all in
round0. Original parent19 is left at(33,22)/(33,21); at round1 action727 it
requests MOVE N and actually self-dies at event728, with no successful
position update. The pair is an ordinary edge. H14's corresponding initial
parent is reduced over several rounds and self-dies only at round4/event1423.

The complete paired outcome changes win-to-loss. At round20 own R21-minus-
H14 population/total are-3/-5. Earlier parent death is a real local witness,
not by itself a complete explanation of the terminal result.

## Decision Boundary

The single factor visibly changes allocations, scout timing and location.
The observed paired outcomes include both promotions and regressions, and
the early checkpoint means do not establish faster aggregate resource
growth. Further matched development seeds can test whether the result
persists; neither this mechanism analysis nor its reviewer qualifies R21
as robust or authorizes production promotion.
