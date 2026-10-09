# R15: Take The Terminal Pearl Before Routine Split

Registered 2026-10-05 before competitive matches. Parent is frozen H14
`tyhh_qos_counter_queen_guard_consolidate_v127`, main SHA-256
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.

The design was revised during diagnosis, before matches: a final pearl-move
guard, then a two-step deadpath guard, were proposed but not implemented.
Actual stripes fixtures have no safe ordinary replacement at those points.
R15 instead changes only a narrowly proven ordinary expansion SPLIT 2.

Hypothesis: a length-four scout inside a fully observed terminal pearl branch
splits prematurely, leaves length two, then eats the final pearl to length
three and cannot reverse-split out. Taking that final pearl first reaches
length five; terminal-state arbitration then retains two at the terminal and
reverses three rear segments into an escaping child. The routine SPLIT 2 has
priority over the emergency reverse-split scorer, so relying on the latter
alone was disproved by source inspection before competitive testing. Both
states are handled within the otherwise-approved ordinary SPLIT 2 branch.

Evidence: H14's audited xx stripes games at bases 82001/83001/84001 all lose.
In 82001 side A, id 2 at round index 14 requests SPLIT 2 from body
[(16,10),(16,9),(17,8),(18,8)] (event 771). Round 15 MOVE E collects the pearl
at (17,10) (event 833); round 16 MOVE W dies by self collision (event 894).
All other terminal boundaries are kelp. The former tail (18,8) has observed
ordinary open exits north (18,7) and east (19,8). Side B has the mirrored
sequence, as do later children and other development seeds. Queen death is
separate: all six stripes teams become extinct by round 31 to 197.

Single factor: terminal-pocket ordinary split arbitration. At an
otherwise-approved ordinary split, only a nonqueen of length exactly four,
with complete currently visible own body and no visible enemy or recent
enemy sighting, may take the terminal pearl instead. There
must be exactly one current surviving move; every current exit must be known;
that move must be ordinary, visible, pearl-bearing, and geometrically terminal
(one ordinary entry, three kelp edges). Exact projectAction proves length five
and living pickup. The parent length-two neck must not bridge a portal; the
reversed length-three rear child must have a proven ordinary clear first step
and another distinct clear step, checked against the whole projected body.
At length exactly five already in a proven terminal with an ordinary neck
edge, the same arbitration replaces ordinary SPLIT 2 by SPLIT 3. It rechecks
complete visible body, no enemies, current split capacity, splitSafe(3),
normal parent neck, the current reversed child's two-step clear route, and
hard/soft friend protection. It does not bypass consolidation holding,
champion suppression, the population target, or other earlier actions.
Any unknown/portal uncertainty, missing body, extra live exit, occupied child
route, cap, queen, enemy, invalid current split or no terminal pearl preserves
H14. No emergency split implementation, global growth threshold, queen policy or portal
exploration is changed; there is no new mission state.
Successful arbitration records a short `POCKET_SALVAGE` proposal log with
round, ID, pickup/reverse state and length; final emitted actions must still
be audited rather than counting logs as competitive success.

Risks: dynamic opponents/allies can invalidate the future child route before
the next turn; the split-capacity check is current, not a future guarantee.
The terminal parent may die after releasing the child. This factor preserves
resources and an explorer, not both dragons. Narrow gating may affect too few
games to improve overall wins and cannot solve queen-only head-hit losses.

Pre-match checks: source-bound actual stripes and its mirror validate the
whole pickup -> exact body5 -> split3 -> exact reversed child3 -> two living
child moves. Frozen H14 controls instead reproduce split2 -> pickup length3
-> self death. Official unswbc 1.2.7 native-bot fixtures independently confirm
both sequences, with actual engine observations and no map correction notices.
The candidate terminal parent still dies by self collision one round after
child release; only the rear explorer and its additional segment are saved.
A fifth engine boundary fixture introduces an enemy into the terminal turn;
the rechecked no-enemy gate rejects split3 and preserves ordinary split2.
The fixture deliberately ends all surviving dragons after round 2; its
round-3 invalid-action deaths are test termination, not competitive results.

Root owns frozen native unswbc 1.2.7 ranked17/both-seat tests, 68 games per
development base seed against unchanged xxqztest and V3_portal separately.
First screen is 82001, with 83001/84001 only for a promising candidate. Holdout
seeds remain unused until a frozen finalist. Primary metric remains wins,
draws non-wins; report seed/seat/map slices and matched gained/lost wins. All
negative results and replays remain. No stability claim from developer seeds.
