# H15: Multi-Step Short Scout Portal Route

Parent: H5 queen_consolidate_v127. The only policy addition is a short-scout
portal-route priority before routine splitting. Nonqueens length<8 before220
without visible enemy heads use known ordinary-open edges to find a portal
within6moves, at most128popped nodes. Targets have no learned crossing exit,
no current room claim and no resolved destination already seen by this unit.
Its scout proposal contains just the first step, requiring exact known
surviving current action, no direct friend collision and no strong friend-exit
blocking. Existing freeHarvest postprocessing remains and may replace that
proposal with a proven safe multi-step pearl action for length5-7 scouts.

Adjacent early crossing policy, longdragon/queen portal safeguards, scoring,
feeding, consolidation and split thresholds are unchanged. This policy never
proposes a blind protected-dragon crossing. Unknown hidden enemies/future body
motion are not guaranteed safe; current occupancy is blocked conservatively.
Once it has crossed, learnedExit makes that directed portal cease to be a target.
After80 this addition routes near a portal but does not override the existing
actual portal crossing decision. It is not guaranteed room occupation.

Evidence: default rich corners have25cells while current provenResourceRoom
cap is24; queenparent A did not enter either, B entered northwest at497 only.
The local adjacent<80portal priority cannot establish a remote scout task.
Prediction: earlier resource-room entry and more resources/wins. Risk: scouts
defer splits/pearls and overcommit to currently known portals.

Development base78001, ranked17, opponents unchanged, both sides68games.
H14 confirmation source remains frozen; do not use confirmation to tune H15.
