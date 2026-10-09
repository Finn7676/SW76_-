# tyhh_qos_protect

Baseline: `tyhh_qos_feed`.

The movement scorer now models visible friendly-head exits after a candidate move.
Longer dragons and late-game champions receive hard priority: occupying their only
currently safe exit is strongly rejected, while moves that reopen exits receive a
large bonus even when they leave this dragon with little personal space. Dragons
within two observed segments of our length receive a smaller route-preservation
bonus only when our own candidate retains at least two exits and sufficient reachable
space. Portal exits and time-aware tail release use the existing topology and
occupancy model. Direct portal, ram, sprint, and pearl-sprint paths use the same
long-friend blocker check.

Focused exit-space scenarios are in `verification/friend_protection.cpp.txt`.
