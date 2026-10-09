# Evidence to Claim

| Claim | Evidence | Status |
|---|---|---|
| Two isolated bots adapt the fixed queen and free-step rules | main.cpp frozen trees and code review | Supported implementation |
| Correct selected queen/sprint/portal semantics | native fixtures + engine_fixtures.json27 cases | Supported within test scope |
| More wins in updated17-map development sample | combined_verified.json:26 ->30/31 out of68 | Supported descriptively |
| Better on historical ten-map subset |19 ->17/18 out of40 | Not supported |
| Queen-first safety works reliably | final queen alive6 ->7/6 out of68 | Not supported |
| Queen donation reliably transfers whole corpses | local route checks, no queen action reservation | Not established |
| Donation causes observed queen pearl recovery | feed_mechanism.json:63 directly observed pearls after40/41 feed deaths across6 games | Supported observed subset, not all corpse pearls |
| A single frozen candidate exceeds50% total wins against each unchanged opponent | H14 threeunusedseed confirmation:V3 56/102,xx54/102; h14_confirmation_final.json | Supported within preregistered pooled-side native protocol, not every map/side or significance |
| Judge resource compliance across all ranked maps |8 separate sandbox smoke games | Not established; smoke only |

Decision: registered joint win-rate gate is reached by H14. Preserve all
experimental bots and evidence; do not replace production. Queen survival,
sideB anddefault/portals weaknesses remain, so the success has limited scope.

## Continuing Development

H3 improved queen survival7/68 to10/68 but wins only15/34 V3 and16/34 xx.
H4 bodylookahead and H7 threatmargin did not improve the joint objective;
H6 cap32 reduced wins. Preserve these audited negative results.
H8 maturity350 improved wins to18/34 V3 and16/34 xx, old10 subset23/40.
H5 nonqueen consolidation improved feedparent to16/34 V3 and21/34 xx,
old10 subset22/40. These are separate policies and their gains are not additive.
Registered factorial combinations are evaluated before any independent seeds.

Direct H5 transfer audit: consolidate_mechanism.json contains2025donor deaths
and2326 intended-recipient pearl collections;1837feeds have at leastone
recipient pickup. The transfer mechanism is observed, not a win guarantee.
H9/H11 combinations fail the strict joint>50% development gate. No reserved
independent seed was used to tune the frozen H14. Confirmation now completed:
all408candidate/reference games audited;45paired gained wins and17lost wins.
Threebatch rates:H14 V3 54.9%+/-7.4%,xx52.9%+/-5.1%, descriptive only.
H14 final queen survival38/204 versus reference34/204 remains low. Sandbox
4portals games have0wins and no runtimefailure, not whole-map compliance.
H15 is prepared and untested competitively; no victory claim for that bot.
