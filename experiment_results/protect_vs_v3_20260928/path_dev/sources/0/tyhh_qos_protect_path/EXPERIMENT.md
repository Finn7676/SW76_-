# Equal-distance ordinary routing experiment

Base: `tyhh_test/tyhh_qos_protect_route` on 2026-09-28. The base is unchanged.

The ordinary goal BFS now compares all goals at the minimum reached depth.
Equal-distance alternatives prefer more visible pearls along the route, then
the sum of the existing V3 additive route terms: unseen cells +130, each nearby
friendly segment -35, and narrow exits without a pearl -80. Each cell retains
its best same-depth route. The same mechanism compares alternative routes to
one goal. The first step excludes known friendly heads and longer friendly
bodies, as required by ordinary move execution.

The search retains its 24-step and 320-node limits. Goal construction and the
ordinary +30 route direction bonus are unchanged. No Scout, relay, portal-claim,
protection, champion, split, or other move-scoring changes are included.

Hypothesis: direction-order tie breaking can waste otherwise equivalent
approaches by choosing less productive or more crowded paths. This intervention
may improve collection when the movement scores are close; its effect is
bounded by the unchanged +30 bonus and requires competitive measurement.

Focused ASan/UBSan scenarios check equal-distance distinct goals, equal-distance
paths to one goal, strict distance priority, crowd preference, friendly-head
first-step exclusion, unknown portal exits, and the depth bound.
