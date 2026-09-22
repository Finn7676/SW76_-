"""What this dragon can prove from the current 7x7 window.

Moves are chosen from this snapshot. Memory may rank a direction, but it
cannot make a tile look empty when vision shows a body or kelp.
"""

from collections import deque

import helper as unswbc
from helper import Direction, EdgeType

from model import DIRS, step_xy


class Cell:
    __slots__ = ("pearl", "eta", "owner", "dragon_id", "is_head", "edges")

    def __init__(self) -> None:
        self.pearl = False
        self.eta = -1
        self.owner = None  # None, "me", "ally", "foe"
        self.dragon_id = None
        self.is_head = False
        self.edges = [EdgeType.EMPTY, EdgeType.EMPTY, EdgeType.EMPTY, EdgeType.EMPTY]


class StepInfo:
    __slots__ = ("direction", "dest", "pearl", "eta", "hard_trap", "soft_trap", "pocket")

    def __init__(self, direction: Direction, dest: tuple[int, int], pearl: bool, eta: int,
                 hard_trap: bool, soft_trap: bool, pocket: int) -> None:
        self.direction = direction
        self.dest = dest
        self.pearl = pearl
        self.eta = eta
        self.hard_trap = hard_trap
        self.soft_trap = soft_trap
        self.pocket = pocket


class Enemy:
    __slots__ = ("id", "head", "visible")

    def __init__(self, dragon_id: int, head: tuple[int, int] | None, visible: int) -> None:
        self.id = dragon_id
        self.head = head
        self.visible = visible


class View:
    __slots__ = ("head", "body", "body_complete", "safe", "enemies", "ally_heads", "snap", "w", "h")

    def __init__(self) -> None:
        self.head = (0, 0)
        self.body: list[tuple[int, int]] = []
        self.body_complete = False
        self.safe: list[StepInfo] = []
        self.enemies: list[Enemy] = []
        self.ally_heads: list[tuple[int, int]] = []
        self.snap: dict[tuple[int, int], Cell] = {}
        self.w = 0
        self.h = 0


def look(ct: unswbc.Controller, game: unswbc.Game) -> View:
    view = View()
    view.w, view.h = game.get_map_size()
    head = ct.get_position()
    view.head = (head.x, head.y)
    my_id = ct.get_id()
    my_team = ct.get_team()
    mine: dict[tuple[int, int], Direction] = {}

    for tile in ct.get_tiles():
        pos = tile.get_position()
        key = (pos.x, pos.y)
        cell = Cell()
        cell.pearl = tile.has_pearl()
        cell.eta = tile.get_pearl_time()
        for index, direction in enumerate(DIRS):
            cell.edges[index] = tile.get_edge(direction).get_edge_type()
        part = tile.get_dragon()
        if part is not None:
            cell.dragon_id = part.get_id()
            cell.is_head = part.is_head()
            if part.get_id() == my_id:
                cell.owner = "me"
                mine[key] = part.get_dir()
            elif part.get_team() == my_team:
                cell.owner = "ally"
                if part.is_head():
                    view.ally_heads.append(key)
            else:
                cell.owner = "foe"
        view.snap[key] = cell

    view.body = _body(ct, view, mine)
    view.body_complete = (
        len(view.body) == ct.get_length() and len(set(view.body)) == len(view.body)
    )
    view.enemies = _enemies(view.snap)
    view.safe = _safe_steps(ct, view)
    return view


def _body(ct: unswbc.Controller, view: View, mine: dict[tuple[int, int], Direction]) -> list[tuple[int, int]]:
    facing = ct.get_dir()
    pos = view.head
    chain = [pos]
    for _ in range(ct.get_length() - 1):
        nxt = step_xy(pos[0], pos[1], facing.get_opposite(), view.w, view.h)
        segment = mine.get(nxt)
        if segment is None or nxt in chain:
            break
        chain.append(nxt)
        facing = segment
        pos = nxt
    return chain


def _enemies(snap: dict[tuple[int, int], Cell]) -> list[Enemy]:
    counts: dict[int, int] = {}
    heads: dict[int, tuple[int, int]] = {}
    for pos, cell in snap.items():
        if cell.owner != "foe" or cell.dragon_id is None:
            continue
        counts[cell.dragon_id] = counts.get(cell.dragon_id, 0) + 1
        if cell.is_head:
            heads[cell.dragon_id] = pos
    return [Enemy(dragon_id, heads.get(dragon_id), count) for dragon_id, count in counts.items()]


def _safe_steps(ct: unswbc.Controller, view: View) -> list[StepInfo]:
    origin = view.snap.get(view.head)
    if origin is None:
        return []
    steps = []
    for index, direction in enumerate(DIRS):
        if origin.edges[index] != EdgeType.EMPTY:
            continue
        dest = step_xy(view.head[0], view.head[1], direction, view.w, view.h)
        cell = view.snap.get(dest)
        if cell is None or cell.owner is not None:
            continue
        pocket, hard, soft = _pocket(dest, view, ct.get_length())
        steps.append(StepInfo(direction, dest, cell.pearl, cell.eta, hard, soft, pocket))
    return steps


def _pocket(dest: tuple[int, int], view: View, length: int) -> tuple[int, bool, bool]:
    """Return pocket size, hard trap, soft trap.

    A hard trap is a dead end we can see the end of, or a hole smaller than
    our body. A soft trap is a one-tile-wide tunnel that leaves the window:
    entering it cannot be reversed, so it is only taken when nothing opens up.
    """
    tiles = {dest}
    queue: deque[tuple[int, int]] = deque([dest])
    escapes = False
    while queue:
        cur = queue.popleft()
        cell = view.snap.get(cur)
        if cell is None:
            escapes = True
            continue
        for index, direction in enumerate(DIRS):
            if cell.edges[index] != EdgeType.EMPTY:
                continue
            nxt = step_xy(cur[0], cur[1], direction, view.w, view.h)
            if nxt in tiles:
                continue
            nxt_cell = view.snap.get(nxt)
            if nxt_cell is None:
                escapes = True
                continue
            if nxt_cell.owner is not None:
                continue
            tiles.add(nxt)
            queue.append(nxt)

    min_deg = 99
    max_deg = 0
    for cur in tiles:
        cell = view.snap[cur]
        degree = 0
        for index, direction in enumerate(DIRS):
            if cell.edges[index] != EdgeType.EMPTY:
                continue
            nxt = step_xy(cur[0], cur[1], direction, view.w, view.h)
            if nxt in tiles:
                degree += 1
        min_deg = min(min_deg, degree)
        max_deg = max(max_deg, degree)
    if not tiles:
        min_deg = 0

    hard = False
    soft = False
    if not escapes:
        if max_deg <= 2 and min_deg <= 1:
            hard = True
        elif len(tiles) < length:
            hard = True
    elif max_deg <= 2:
        soft = True
    return len(tiles), hard, soft


def last_resort(ct: unswbc.Controller) -> Direction:
    """A legal-looking direction when every empty tile is gone. Kelp and our own body stay last."""
    here = ct.get_position()
    tile = ct.get_tile(here)
    my_id = ct.get_id()
    my_team = ct.get_team()
    ranked: list[tuple[int, int, Direction]] = []
    for index, direction in enumerate(DIRS):
        penalty = 0
        edge = tile.get_edge(direction).get_edge_type() if tile is not None else EdgeType.KELP
        if edge == EdgeType.KELP:
            penalty += 5000
        elif edge == EdgeType.PORTAL:
            penalty += 900
        ahead = ct.get_tile(here.add_dir(direction))
        part = ahead.get_dragon() if ahead is not None else None
        if part is not None:
            if part.get_team() == my_team and part.get_id() != my_id and part.is_head():
                penalty += 8000
            elif part.get_id() == my_id:
                penalty += 3200
            elif part.get_team() == my_team:
                penalty += 3000
            elif part.is_head():
                penalty += 100
            else:
                penalty += 1800
        if direction == ct.get_dir().get_opposite():
            penalty += 40
        ranked.append((penalty, index, direction))
    ranked.sort()
    return ranked[0][2]
