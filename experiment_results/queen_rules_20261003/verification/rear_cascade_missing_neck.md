# Inherited Missing-Neck Boundary

After candidate READY, independent reviewer suggested checking unavailable
own-neck geometry. No frozen candidate files were changed. The separate
source-bound `rear_cascade_missing_neck.cpp.txt` test compiles and passes:
with LEN7, empty own trail and no visible own parts, `ownSeg(LEN-2)==-1`,
and both `splitSafe(2)` and `splitSafe(LEN-2)` return true.

This is inherited H14 behavior: `bridged(a,b)` returns false for unavailable
coordinates. R21 preserves the existing finite gate rather than adding a
whole-body or continuation requirement. Neither successful gate nor native
ordinary fixtures proves the safety of an unknown parent neck or rear path.
The candidate is a legal-growth allocation experiment, not a universal
split-survival policy. This limitation is consistent with READY/EXPERIMENT's
explicit legal-split-is-not-escape risk, and should remain in screen interpretation.

Test SHA256: `b2db5ffe01b30200b485ee8a8ad57d11500b255f2383cd9524d1ce9c290c2e0c`.
