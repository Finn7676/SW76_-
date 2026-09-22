"""Sprint a short dragon onto an enemy head.

The dive is the only move that is allowed to kill us, and only when the
whole path is inside the current window and the enemy is visibly longer.
"""

from collections import deque

import helper as unswbc
from helper import Direction, EdgeType

from model import DIRS, MISSILE_MAX_LENGTH, step_xy, worth_killing
from sense import View


def find(ct: unswbc.Controller, view: View) -> list[Direction] | None:
    length = ct.get_length()
    if length > MISSILE_MAX_LENGTH or length < 2:
        return None
    if ct.get_unit_count() < 2:
        return None
    targets = {
        enemy.id
        for enemy in view.enemies
        if enemy.head is not None and worth_killing(enemy.visible, length)
    }
    if not targets:
        return None

    max_steps = length - 1
    start_body = tuple(view.body) if view.body_complete else None
    queue: deque[tuple[tuple[int, int], list[Direction], tuple[tuple[int, int], ...] | None]] = deque()
    queue.append((view.head, [], start_body))
    seen: set[tuple] = {(view.head, start_body)}
    expanded = 0

    while queue and expanded < 400:
        pos, path, body = queue.popleft()
        expanded += 1
        if len(path) >= max_steps:
            continue
        if path and not view.body_complete:
            continue
        if path and body is not None and len(body) <= 2:
            continue
        cell = view.snap.get(pos)
        if cell is None:
            continue
        for index, direction in enumerate(DIRS):
            if cell.edges[index] != EdgeType.EMPTY:
                continue
            nxt = step_xy(pos[0], pos[1], direction, view.w, view.h)
            info = view.snap.get(nxt)
            if info is None:
                continue
            if info.owner == "foe" and info.is_head and info.dragon_id in targets:
                return path + [direction]
            if info.owner is not None:
                continue
            if body is not None and nxt in body:
                continue
            if not view.body_complete:
                continue
            new_body = _advance(body, nxt, info.pearl, len(path))
            if new_body is None:
                continue
            state = (nxt, new_body)
            if state in seen:
                continue
            seen.add(state)
            queue.append((nxt, path + [direction], new_body))
    return None


def _advance(body: tuple[tuple[int, int], ...] | None, dest: tuple[int, int], ate: bool,
             step_index: int) -> tuple[tuple[int, int], ...] | None:
    """One sprint step. Extra steps cost one segment after the tail moves."""
    if body is None:
        return None
    if step_index > 0 and len(body) <= 2:
        return None
    updated = (dest,) + body
    if not ate:
        updated = updated[:-1]
    if step_index > 0:
        if len(updated) <= 1:
            return None
        updated = updated[:-1]
    return updated
