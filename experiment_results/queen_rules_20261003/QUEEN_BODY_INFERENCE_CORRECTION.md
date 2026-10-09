# Visible Body Inference Correction

Recorded 2026-10-06 during R20 implementation, before competitive games.
Original raw replays, engine fixtures and evidence reports are retained.

Nonhead part facing points to its preceding segment toward the head. It
does not identify that segment's next rear neighbor when the body turns.
For example, the actual dilemma84006A round199 input has queen part(26,3)
facingW toward(25,3), but its rear part is(26,4), whose facingN points back
to(26,3). Repeatedly applying current-part facing^2 would incorrectly visit
(27,3). General reconstruction must search predecessor links from all
visible same-ID/same-team parts and reject ambiguity/cycles/disconnected
parts. Own LENGTH supplies an independent completeness check.

An enemy tail's single reverse-facing cell being visible is also insufficient
to exclude an unseen rear part joining from another ordinary side. R20
requires every possible non-kelp tail neighbor to be known ordinary and
visible, rejecting portal/unknown geometry. This is a conservative admission
test, not a prediction of enemy movement or future occupancy.

The specific official1.2.7 action-tape comparison remains valid: queen N
and immediate enemy14 W kill both heads; queen NN retains length8 and turns
enemy14 W into a body collision while queen lives. Enemy14's tail(25,1)
has all ordinary neighbors in the actual view, so it still passes the
stricter test. Enemy16's tail(27,4) borders hidden cell(28,4), so its length
is not input-proven by this stricter rule and it must remain unknown.

Accordingly, the old report's statement that both enemy14 and enemy16 are
input-proven complete bodies must not be reused as general-policy evidence.
The reverse-facing helper in queen_immediate_engine.py is insufficient for
general curved-body inference. This correction changes neither recorded
actions nor the official engine's N/NN counterfactual; it narrows what can
be claimed from the current input. Candidate source-bound fixtures will
cover curved bodies and hidden side predecessors before R20 evaluation.
