# tyhh portal-module staged experiment — 2026-09-25

Opponent: current root tyhh.
Protocol: 12 maps, both sides, one deterministic game per condition.

| Version | Modules | Wins | Losses | Draws | Win rate |
|---|---|---:|---:|---:|---:|
| tyhh_portal_bridge | none (bridge control) | 10 | 14 | 0 | 41.7% |
| tyhh_portal_relay | Relay | 12 | 12 | 0 | 50.0% |
| tyhh_portal_relay_scout | Relay + Scout | 12 | 12 | 0 | 50.0% |
| tyhh_portal_core | Relay + Scout + Claim | 12 | 12 | 0 | 50.0% |

## Adjacent-stage flips

### tyhh_portal_bridge -> tyhh_portal_relay (+2 wins)
- default.map side B: loss -> win
- help.map side B: win -> loss
- schooltime.map side A: loss -> win
- schooltime.map side B: loss -> win
- stronghold.map side A: loss -> win
- trophy.map side A: win -> loss

### tyhh_portal_relay -> tyhh_portal_relay_scout (+0 wins)
- default.map side A: loss -> win
- queen_of_spades.map side A: loss -> win
- queen_of_spades.map side B: win -> loss
- trauma.map side B: win -> loss

### tyhh_portal_relay_scout -> tyhh_portal_core (+0 wins)
- Colosseum.map side A: loss -> win
- default.map side B: win -> loss
- help.map side A: win -> loss
- help.map side B: loss -> win
- stronghold.map side B: win -> loss
- trauma.map side B: loss -> win
