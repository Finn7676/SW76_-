# R14: Guarded Local Pearl Reservation

Registered 2026-10-05 before source changes or competitive evaluation.
Parent is frozen R12 `tyhh_qos_robust_pearl_reserve_v127`, main SHA-256
`2a49f95382df699038ecb267c8694b7beff69febb5d0fae4570766e0beedeb82` and
reservation header SHA-256
`3fa9d132e302afe0b89096c2effe46a00b86b0628c4a6b318dce64094bb84f01`.
Frozen H14 main remains
`ce879d9510e5a4844f24af61b1d2db4b67e22061b27e46316cbd892fe4f670dc`.

Hypothesis: R12's score penalty may redirect a proved living pearl pickup
toward an unresolved portal or another unproved ordinary-scoring option,
although a separate known nonpearl escape exists. Reservations should apply
only when their score changes cannot select such an option.

Single additional factor: reservation is disabled unless every first step
admitted by the unchanged ordinary scorer has a complete own-body proof,
known visible surviving projection, proved visible continuation, permitted
portal landing and no direct, hard or soft friendly risk. The gate uses the
same kelp, portal-claim, longer-friend and current-occupancy exclusions as
the scorer. An unresolved or unknown admitted edge disables reservation.
R12's recipient conditions, score penalty and scored free-harvest exclusion
remain unchanged. While reservations are active, scored free-harvest
replacements must satisfy the same action proof, including soft friendly
risk; disabled reservation leaves original H14 proposals and free harvest
available. No new move, sprint, split or donation is emitted.

Focused tests must reproduce crowded known pearl pickup versus an admitted
unresolved portal: H14 and R14 take the known pickup, while frozen R12 may
select the unproved portal. Controls also cover incomplete body, known
dead-end, hard/soft friendly risk, claimed portal exclusions, R12's positive
reservation cases and exact unchanged behavior with no qualifying receiver.
Production build and inherited source-bound fixtures are required. No games
are launched by this task; root owns frozen paired evaluation and retention
of all results, including failures.

Risk: checking every admitted option is conservative and may suppress many
otherwise productive reservations. It proves the local first-step choice;
receiver future action remains uncertain. Competitive strength and judge
CPU-points compliance require separate evaluation.
