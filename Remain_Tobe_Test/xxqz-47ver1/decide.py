"""Turn priority.

1. Short dragon, teammate still alive, enemy head visibly longer: sprint onto that head.
2. Early and still under the population cap: split a length-2 child.
3. Every open tile is a dead end: split down to length 2 instead of walking in.
4. Otherwise one vision-safe step toward a pearl, fog, or that long head.
5. Last resort still refuses kelp and our own body when any other edge exists.
"""

import helper as unswbc

import forage
import kamikaze
import opening
from memory import memory
import sense


def act(ct: unswbc.Controller, game: unswbc.Game) -> None:
    if memory.w == 0:
        memory.configure(*game.get_map_size())
    memory.observe(ct, game)
    view = sense.look(ct, game)

    path = kamikaze.find(ct, view)
    if path:
        ct.set_indicator_string("kill " + "".join(direction.value for direction in path))
        if len(path) == 1:
            ct.make_move(path[0])
        else:
            ct.make_moves(path)
        return

    if opening.want_voluntary(ct, game, view) and opening.split_two(ct):
        return
    if opening.want_emergency(ct, view) and opening.split_down(ct):
        return

    picked = forage.choose(ct, view, memory)
    if picked is not None:
        step, reason, goal = picked
        forage.paint(ct, step, reason, goal)
        return

    ct.set_indicator_string("fallback")
    ct.make_move(sense.last_resort(ct))
