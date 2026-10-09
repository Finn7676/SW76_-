# Maturation-only control

Parent: tyhh_qos_protect_relay, main.cpp SHA256 c65cfa425e09b38662610fd19fc309b419e5c82edc2c6e7cddadeab8337b4961.

Registered before candidate matches: change only the ordinary expansion cutoff
from round 470 to round 350. Population-below-eight recovery, emergency splits,
champion selection, portal exploration, protection, and existing donation are
unchanged. This tests whether continued ordinary splitting prevents harvested
length from persisting late in the game. It is a control for the more elaborate
recipient-aware crown module, not an assumption that sacrifice is absent.

Motivation: in baseline seed73001 Portals games against xxqztest, relay's mean
longest length changes from9.5 at350 to4.5 at450; final relay total77 exceeds
opponent48.5, yet longest11 trails15. Growth and deaths may also contribute;
this single change does not pre-attribute the loss to splitting.

Evaluate both V3_portal and xxqztest, official ten maps and both sides using
the root frozen development protocol. Expect improved longest length but
possible action-count and map-control losses; record both.
