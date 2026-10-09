# Updated full-map battle: tyhh_portal_direct vs V3_portal

Protocol: current `benchmark.py --maps all --games 2`; 14 maps, both sides, 28 games.
The current map set includes `autarky.map` and `dilemma.map`; `help.map` is resolved from `maps_BeforeSprinter`.

| Map | direct wins | V3 wins | direct result A/B |
|---|---:|---:|---|
| Colosseum.map | 1 | 1 | win / loss |
| arena.map | 0 | 2 | loss / loss |
| autarky.map | 0 | 2 | loss / loss |
| big_empty.map | 0 | 2 | loss / loss |
| default.map | 1 | 1 | win / loss |
| default_small.map | 0 | 2 | loss / loss |
| devil.map | 1 | 1 | win / loss |
| dilemma.map | 2 | 0 | win / win |
| help.map | 2 | 0 | win / win |
| queen_of_spades.map | 1 | 1 | win / loss |
| schooltime.map | 0 | 2 | loss / loss |
| stronghold.map | 2 | 0 | win / win |
| trauma.map | 0 | 2 | loss / loss |
| trophy.map | 1 | 1 | win / loss |

## Result

The direct version scored 11/28 (39.3%); V3_portal scored 17/28 (60.7%). V3 leads by 6 wins and 21.4 percentage points.

The direct version wins both games on `dilemma`, `help`, and `stronghold`; it wins one game on `Colosseum`, `default`, `devil`, `queen_of_spades`, and `trophy`. V3 wins both games on `arena`, `autarky`, `big_empty`, `default_small`, `schooltime`, and `trauma`.

## Map update

The two newly included maps are `autarky` and `dilemma`. Direct wins 0/2 on autarky and 2/2 on dilemma. The new maps therefore contribute a net 2-2 split and do not explain the six-win overall gap.

## Interpretation

On the refreshed full map set, V3_portal is clearly stronger than the direct-root experimental version. The direct transplant showed a small +1 result against the root baseline in its earlier 24-condition check, but that gain does not transfer to the stronger V3 opponent. The direct implementation should remain experimental and should not replace V3_portal.