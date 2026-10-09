# R25 Verification Pending

Candidate and actual H14 control compiled cleanly at g++-10 C++20 O2.
Candidate main differs only by short_terminal.hpp include and one final
reviewShortTerminal(ct) call after queen survival, before projection/emission.

38 focused source-bound controls passed on candidate and actual H14 control.
Positive quiet terminal, unscored proposal and source occupancy fallback;
queen/LEN3/LEN4/split/donation/multistep/empty/nonpearl/safe pearl exclusions;
unknown/portal action and postexit, hidden/occupied pearl, incomplete,
duplicate, hidden, bridged, unknown, disconnected and mismatched ownbody;
enemy head/body; missing/occupied/hidden/unknown/portal alternate; hard/soft
protection, alternate terminal/terminal continuation, other-body-not-proof.
Test include compiles emit the inherited renamed-main no-return warning.
Production builds do not. Initial test compile failed on an unqualified
fixture controller member, corrected to f.ct.length without policy edits.

All three unchanged inherited H14 tests passed with candidate source:
projection/payment/identity/portal, seven queen-guard assertions and eight
consolidation assertions. These suites are byte-identical to H14.

Official input preparation passed: exact27385 canonical physical/SONAR/
action events before trauma84010B id75r188, full true target body and visible
occupancy/pearls, official engine hash and replay hash. Raw target SHA256
ef5eff379593964e41aed03e42432a9f5f2e660f08d58e2c84c5705346170163.
Saved actual-state JSON/replay in experiment_results/xxqz_robust_20261004.

Fresh native full pipelines on that byte-identical actual input passed:
H14 MOVE S, R25 MOVE E. Both stderr empty; source-created current-body
observation is sufficient even with no prior-turn memory. See
r25_terminal_native.py/json. This establishes current input selection, not
historical persistent-state regeneration or competitive benefit.

Official 1.2.7 continuation arms passed. The original scripted arm held all
pre-target physical/SONAR/action events and other actors fixed, used id75
`MOVE S` then recorded `MOVE N`, and reproduced pearl pickup to LEN3 followed
by id75 self death at r189. The alternate changed only id75 to `MOVE E` then
`MOVE S`; it produced two successful updates, bodies `(35,21),(34,21)` then
`(35,22),(35,21)`, and no id75 death. Target r188 raw input was byte-identical
and the 27385-event canonical prefix matched in both arms. Replays and JSON
are `r25_terminal_rules_{original,alternate}.replay` and
`r25_terminal_rules.json`. This is a scripted local continuation, not live
strategy or competitive evidence.

Fresh independent source review reported no blocker, reproduced the focused
tests, and checked source/artifact hashes. Production is frozen at READY.md
values before the registered82001development screen. No competitive score
is recorded in this verification document.
