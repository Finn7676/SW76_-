# R19 Queen Hunt x Local Late Maturation

Registered after the isolated R10 and R17 development screens. Parent is the
source-frozen R10 `tyhh_qos_robust_queen_hunt_v127`; H14 remains the primary
reference. This is an interaction test, not evidence that two component gains
are additive.

Single source factor relative to R10: after round350, when population is at
least8 and no current visible enemy part exists, suppress only the ordinary
expansion flag. Queen-hunt target value, sprint call order, portal rules,
donation acceptance, low-population refill, tactical actions, protection and
emergency splitting are unchanged. The same gate was independently tested as
R17 against H14, where it improved the first screen but failed the robust
threshold; R19 tests whether it preserves R10's stronger xxqztest side.

Expected observation: retain R10's visible-queen attack gains while reducing
late fragmentation. Risks are interaction loss, lower population/exploration,
long-body dead ends and hidden-enemy exposure. No claim of additivity or
robust superiority is made before the paired screen.

Verification uses the R17 source-bound fixture with R10 as candidate parent,
the exact frozen H14 control, production O2 compile, and one 68-game screen on
development base82001. Expand only if the interaction is useful; held-out
91001/92001/93001 remain untouched.
