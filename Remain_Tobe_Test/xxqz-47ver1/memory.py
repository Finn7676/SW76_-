"""Tiles this dragon has already seen. Used only to rank safe steps."""

from collections import deque

import helper as unswbc
from helper import Direction, EdgeType

from model import DIRS, dir_index, step_xy, torus_dist


class Memory:
    def __init__(self) -> None:
        self.w = 0
        self.h = 0
        self.seen: set[tuple[int, int]] = set()
        self.edges: dict[tuple[int, int, int], EdgeType] = {}
        self.pearls: set[tuple[int, int]] = set()
        self.eta: dict[tuple[int, int], int] = {}
        self.last_round = 0

    def configure(self, width: int, height: int) -> None:
        self.w = width
        self.h = height

    def observe(self, ct: unswbc.Controller, game: unswbc.Game) -> None:
        if self.w == 0:
            self.w, self.h = game.get_map_size()
        round_num = game.get_round_num()
        visible: set[tuple[int, int]] = set()
        if self.last_round and round_num > self.last_round:
            delta = round_num - self.last_round
            for pos in list(self.eta):
                self.eta[pos] -= delta
                if self.eta[pos] < 0:
                    self.eta.pop(pos, None)
                    self.pearls.discard(pos)
        self.last_round = round_num

        for tile in ct.get_tiles():
            pos = tile.get_position()
            key = (pos.x, pos.y)
            visible.add(key)
            self.seen.add(key)
            if tile.has_pearl():
                self.pearls.add(key)
            else:
                self.pearls.discard(key)
            self.eta[key] = tile.get_pearl_time()
            for direction in DIRS:
                self.edges[(key[0], key[1], dir_index(direction))] = tile.get_edge(direction).get_edge_type()

    def nearest(self, origin: tuple[int, int], cells: set[tuple[int, int]]) -> tuple[int, int] | None:
        best = None
        best_dist = 10**9
        for cell in cells:
            dist = torus_dist(origin, cell, self.w, self.h)
            if dist < best_dist or (dist == best_dist and best is not None and cell < best):
                best = cell
                best_dist = dist
        return best

    def frontier(self) -> set[tuple[int, int]]:
        cells: set[tuple[int, int]] = set()
        for x, y in self.seen:
            for direction in DIRS:
                edge = self.edges.get((x, y, dir_index(direction)))
                if edge != EdgeType.EMPTY:
                    continue
                nxt = step_xy(x, y, direction, self.w, self.h)
                if nxt not in self.seen:
                    cells.add(nxt)
        return cells

    def soon(self) -> set[tuple[int, int]]:
        return {pos for pos, eta in self.eta.items() if 0 <= eta <= 10 and pos not in self.pearls}

    def first_step_toward(self, origin: tuple[int, int], goals: set[tuple[int, int]]) -> Direction | None:
        """First empty step along a known path, or None if kelp sits in the way."""
        if not goals or origin in goals:
            return None
        queue: deque[tuple[int, int]] = deque([origin])
        prev: dict[tuple[int, int], tuple[tuple[int, int], Direction] | None] = {origin: None}
        found = None
        expanded = 0
        while queue and expanded < 500:
            cur = queue.popleft()
            expanded += 1
            for direction in DIRS:
                edge = self.edges.get((cur[0], cur[1], dir_index(direction)))
                if edge != EdgeType.EMPTY:
                    continue
                nxt = step_xy(cur[0], cur[1], direction, self.w, self.h)
                if nxt in prev:
                    continue
                prev[nxt] = (cur, direction)
                if nxt in goals:
                    found = nxt
                    queue.clear()
                    break
                if nxt not in self.seen:
                    continue
                queue.append(nxt)
        if found is None:
            return None
        cursor = found
        while cursor != origin:
            parent = prev[cursor]
            if parent is None:
                return None
            prev_pos, direction = parent
            if prev_pos == origin:
                return direction
            cursor = prev_pos
        return None


memory = Memory()
