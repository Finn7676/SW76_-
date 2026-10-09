# `tyhh_endgame_kamikaze`

This is an isolated experiment copied from the current `tyhh` baseline. The
baseline directory is unchanged.

## Added endgame decisions

The trial is enabled from round 400 for non-champion dragons of observed
length at most 8, and never when the team has only one surviving dragon.
Visible segment counts are conservative lower bounds, so a teammate or enemy
is considered longer only when the visible count is already strictly greater
than the donor's own length.

1. If a longer friendly dragon is visible and no enemy segment is visible,
   the short dragon deliberately emits no movement. The engine resolves this
   as an intentional death and drops its pearls for the teammate to collect.
2. If a longer friendly dragon is visible together with an enemy, the short
   dragon tries to collide with an enemy head. If no enemy head is adjacent,
   it tries a safe two-step sprint into one. The same direct/sprint trade is
   allowed when an enemy visibly longer than the short dragon is present, even
   without a friendly long dragon in view.

Direct collisions are attempted before sprints. Portal edges are excluded
from the two-step trial because an unresolved portal cannot certify the final
head position. The existing baseline decisions remain otherwise unchanged.

Compile-time controls are `FEAT_ENDGAME_KAMIKAZE`,
`KAMIKAZE_START_ROUND`, `KAMIKAZE_MAX_LEN`, and `KAMIKAZE_FRIEND_LEAD` in
`main.cpp`.

## Minimal verification

```sh
clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror \
  -Wno-unused-function -I. main.cpp -o /tmp/tyhh_endgame_kamikaze
```

The bot was also run through `unswbc run` against `xxqz_cpp` on `arena.map`
and `big_empty.map`; the latter reached round 400 and reported the expected
`no valid action` deaths from the no-action donation branch. These are smoke
checks, not a statistically powered result.
