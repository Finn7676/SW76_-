# Escape: avoid proven unsplittable zero-onward entries

Registered 2026-09-29 before implementation or any escape match. Parent is
the original frozen tyhh_qos_protect_relay, not counter, mature or strike.
Parent main.cpp SHA-256:
c65cfa425e09b38662610fd19fc309b419e5c82edc2c6e7cddadeab8337b4961.

Single intervention: filter an entry into a known zero-onward destination
when the length after its currently visible pearl is below four, but only
when another known, immediately collision-free direction is not the same
kind of unsplittable dead end. Apply one shared predicate to earlyPortal
and the ordinary scorer. This is one entry-eligibility factor, not two
independent algorithm changes.

Conservative observation boundary: the entry edge is known OPEN or PORTAL,
the landing is resolved and currently visible, all landing edge types are
known, and the existing onwardSafe result equals zero. Unresolved portals
and unknown terrain are not proven dead ends. An alternative must have a
resolved, currently visible landing through a known edge, pass existing
blockedAt and hitsLongFriend checks, and respect the ordinary scorer's
portal-claim exclusion. Unknown landings never establish an alternative.
Unknown onward topology may remain eligible under the original policy;
this change does not claim it is safe beyond the immediate move.

Post-entry length is LEN + (visible pearl ? 1 : 0). A length-three dragon
eating to four remains eligible. With no eligible alternative, preserve
the original choice and terminal fallback. Population-limit, body-shape,
and length-four-plus split feasibility are deliberately out of scope.

Source evidence: relay gives an immediate pearl +500, applies its
opens<=1 penalty only to non-eating moves, and still selects candidates
whose safeMoves flag is false. earlyPortal checks immediate occupancy
without an onward check. This supports a narrow testable hypothesis; it
does not establish that all observed self-deaths arose from pearl pockets
or that preventing such entries improves overall win rate.

Expected benefit: reduce avoidable entry into locally proven traps for
short dragons while preserving portal exploration under uncertainty.
Risks: occupancy can change before the next turn, the alternative can
become blocked, and foregoing pearls can reduce growth. No superiority
claim is made before matched evaluation.

Unchanged: split rules and thresholds, attacks, donation, scoring weights,
portal inference/communication, teammate protection, original fallback,
and all longer-dragon decisions. No special map or opponent knowledge.

Validation: native C++20/O2 build; inherited relay tests; ASan/UBSan focused
cases for 2->3 pearl dead end with alternative, 3->4 retained entry, no
alternative retaining original behavior, unknown-state boundaries, and
ordinary/known-portal parity. Root alone schedules paired full matches
against V3_portal and xxqztest using the fixed development protocol.
Final seeds 74001+ remain unread and are not used for this change.

Implementation and validation completed on 2026-09-29: native build,
focused ASan/UBSan assertions and inherited relay assertions all passed.
Exact commands, compiler warnings, source/binary hashes and scope checks
are recorded in verification/validation.txt. Production source is frozen
at the hash in READY.md before root's competitive evaluation.
