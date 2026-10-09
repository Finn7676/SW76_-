# protect_v3: guard the forced early portal branch

Base: frozen `tyhh_qos_protect_v2`, SHA256
`8615fdeac7708f9b0169db6d5ba0cc3d8402f4dcb4467697721d226835bbdebe`.

The sole behavioral change is an explored-portal filter in the round <80
forced adjacent-portal branch. Once a local crossing or friendly body evidence
marks a portal explored, this branch no longer preempts normal splitting or
move selection to return through that portal. A new unexplored portal can
still receive its original priority.

The ordinary early portal score bonuses, all Scout thresholds, relay behavior,
splitting, protection, and cross-dragon explored-state limitations are unchanged.
Normal scoring can still select a previously explored portal when useful.

Replay reconstruction found rapid repeat crossings in all three parent
variants, not solely in v2. Across the 20 development games, v2 had 61 successful
same-dragon/same-portal repeats within eight rounds with no positive body growth
between the two crossings, compared with 50 for Scout and 37 for relay.
These counts motivate the interface fix but do not establish it as the sole
cause of losses. The v2 Schooltime A loss had no repeated portal crossing in
its first 80 rounds. Competitive improvement remains to be measured.

`verification/early_guard_tests.cpp.txt` invokes the real takeTurn function and
checks that an already explored portal without any room claim does not preempt
an available split, an unexplored portal still does, and a different unexplored
portal remains selectable when one explored portal is filtered.
