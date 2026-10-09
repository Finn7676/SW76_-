# R18 Source-Bound Validation

Production g++-10 C++20 O2 build clean. Both exact-current-view controls passed.

Candidate output:

```text
r55 candidate action=S split=0 gain=1 survives=1 scores=915,855,1410,-9223372036854775808,
r39 candidate action=N split=0 gain=0 survives=1 scores=915,780,-9223372036854775808,-9223372036854775808,
r55 self-observed history candidate action=S split=0 gain=1 survives=1 scores=915,855,1410,-9223372036854775808,
r39 self-observed history candidate action=N split=0 gain=0 survives=1 scores=915,780,-9223372036854775808,-9223372036854775808,
portal-once source-bound assertions passed
```

Frozen H14 output:

```text
r55 H14 control action=N split=0 gain=0 survives=1 scores=0,0,0,0,
r39 H14 control action=N split=0 gain=0 survives=1 scores=0,0,0,0,
r55 self-observed history H14 control action=N split=0 gain=0 survives=1 scores=0,0,0,0,
r39 self-observed history H14 control action=N split=0 gain=0 survives=1 scores=0,0,0,0,
portal-once source-bound assertions passed
```

Historical fixture uses mechanically extracted same-actor observed 7x7 cell
union from default_portal_probe_v4.json, not invisible live resources. Remote
sonar topology and remembered pearl timers are not fully reconstructed, so
this is a source-bound counterfactual, not an exact full production-state replay.
The r55 local action improves; the r39 local action explicitly remains a recross.
