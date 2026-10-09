# H2 crown: recipient-aware endgame consolidation

Registered 2026-09-29 before any candidate matches. Parent is unchanged
`tyhh_qos_protect_relay`, main.cpp SHA256
`c65cfa425e09b38662610fd19fc309b419e5c82edc2c6e7cddadeab8337b4961`.

## Hypothesis

The existing rule (after round 465, donor length <=8, non-Champion,
adjacent larger friendly body) does not establish that the recipient's
head can collect the dropped pearls. Meanwhile ordinary expansion can
re-split collected length whenever population falls below eight. A
single coherent consolidation module will improve late maximum length
by coordinating a donor with a locally verified larger recipient,
prioritizing observed friendly death pearls, and preventing ordinary
splitting during a locally quiet consolidation phase. This is one module;
its internal steps are not independent causal claims.

The root baseline slice supports studying distribution, not assuming a
resource deficit: its recorded Portals sample has larger relay total
length but smaller maximum length. The comparison and raw records are
owned by root in `experiment_results/relay_dual_20260929/`.

## Fixed policy and boundaries

- Stages start at rounds 400 / 430 / 460: maximum donor lengths 10 / 20 /
  32, population reserves 6 / 4 / 2, local quiet windows 12 / 10 / 6.
- Relative size, not an ID-selected Champion label, selects donors. A
  recipient must be a currently visible friendly head, at least length
  eight and strictly longer. Visible segment counts are lower bounds;
  longer sizes require the existing verified status packet.
- No-action donation is allowed only with a known visible route for the
  recipient to the donor head within two steps, a confirmed onward exit,
  enough time for approach plus recovery, and no local or recent enemy
  information. It never collides with a friendly head.
- Donors may approach a visible receiver through known visible safe
  terrain, at most six steps, without using unknown exits. Stalled
  rendezvous attempts expire; no remote or unverified sonar head creates
  permission to die.
- Receivers preferentially collect currently visible pearls where a
  previously observed friendly body disappeared. Mere disappearance
  without a pearl is not death evidence.
- Ordinary population expansion stops in this quiet phase when population
  meets the stage reserve; emergency safety splits remain available.
  Quiet recipients get the existing Champion movement-risk treatment.
- Early exploration, portal protocol, ordinary combat scoring, and
  existing friend-protection mechanics remain unchanged except explicit
  preservation of an already validated donation action.

## Attribution and evaluation

Recipient-relative donation, staged reserves, quiet windows and friendly
death-pearl collection are informed by the local `xxqztest/donation.hpp`,
`roles.hpp`, `planner.hpp`, `observer.hpp`, and `sonar.hpp`. The module is
implemented against relay's existing state and action/protection system;
it does not replace relay with the opponent. The engine fixture adapts
`tyhh_qos_protect_antiram/verification/sprint_payment_engine.py`.

Root alone schedules competitive matches under PLAN.md: both fixed
opponents, official ten maps, swapped sides, matched development seeds;
three unused final seeds after one candidate is frozen. Unit tests and
engine microfixtures establish semantics, not competitive superiority.
Intended no-action death is a match event, not a process failure.

Falsifiers: no late maximum-length improvement, unrecovered donor pearls,
unproductive pursuit, or losses from reducing local population. All
negative runs must remain in the experiment archive.
