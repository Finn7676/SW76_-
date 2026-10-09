# Long-dragon antiram experiment

Base: `tyhh_qos_protect_route`, unchanged. Only enemy head-ram risk review is
added. No exploration, population, champion timing, feeding, or path-search
threshold changes are included.

## Evidence and hypothesis

`verification/analyze_long_deaths.py` reconstructs the two economy development
Schooltime replays at seed 20260927 and verifies final standings. The raw
output is `verification/long_deaths.json`. At replay round >=200, candidate
dragons of length >=8 suffered 12 deaths: ten enemy-initiated head collisions
(97 lost length), two body collisions (16), and zero self-collisions. Six of
the ten head-collision victims already met the existing champion condition.
These cases motivate an enemy-reachability review, not a new own-body
six-step search or an earlier champion threshold. This is diagnostic evidence
from two development games, not independent validation of the intervention.

## Rule

After original planning and teammate protection, a length >=8 dragon checks
its surviving, known projected head against visible enemies' next ordinary
move and two-step sprint. Known open edges and resolved portals define the
path; unresolved portal exits never receive invented coordinates. A fully
reconstructed enemy of length two can sprint only if its first step eats a
visible pearl; an incomplete body provides only a lower bound, so its sprint
capability is treated conservatively. The installed WASM engine fixture in
`verification/sprint_payment_engine.py` verifies this payment timing against
a matched no-pearl control.
An intermediate tile must be free under current other-dragon occupancy and
our projected post-action body. In particular, the newly occupied neck at the
old head blocks a sprint. Vacated own tail tiles are released by projection.

If the proposed head is threatened, the review selects the highest original
score among ordinary moves that have baseline escape space, pass teammate
protection, and have no detected enemy ram path. If none exists, it retains
the original proposal. The review covers ordinary movement, known portal
movement, and known sprint landings. Splits, unresolved landings, and lethal
ram/donation proposals are outside this module. It predicts currently
observed enemy heads only, not unseen enemies, more than two sprint steps,
or future movement of intervening third-party bodies.

Directional `ANTIRAM` logs record interventions. Scenario tests exercise
one/two-step reachability, enemy length uncertainty, portal topology, blocked
intermediates, our newly formed neck, safe detours, and no-alternative fallback.
No competitive improvement is claimed before matched-seed evaluation.

Validation: native C++20/O2 compilation passed. The reachability, length-two
feeding, portal, intermediate-body, safe-detour, and fallback assertions pass
under ASan/UBSan (leak detection disabled for the sandbox). Source and command
details are recorded in `verification/validation.txt`.
