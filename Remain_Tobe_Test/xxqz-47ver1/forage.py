"""Pick one vision-safe step: pearls, unexplored tiles, or a long enemy's head."""

import helper as unswbc
from helper import Position

from memory import Memory
from model import DIRS, MISSILE_MAX_LENGTH, QUEEN_LENGTH, dir_index, torus_dist, worth_killing
from sense import StepInfo, View


def choose(ct: unswbc.Controller, view: View, memory: Memory) -> tuple[StepInfo, str, tuple[int, int] | None] | None:
    roomy = [step for step in view.safe if not step.hard_trap and not step.soft_trap]
    soft = [step for step in view.safe if not step.hard_trap and step.soft_trap]
    hard = [step for step in view.safe if step.hard_trap]
    candidates = roomy or soft or hard
    if not candidates:
        return None

    goal, reason = _goal(ct, view, memory)
    best = candidates[0]
    best_score = -10**9
    for step in candidates:
        score = _score(ct, view, step, goal, reason)
        index = dir_index(step.direction)
        if score > best_score or (score == best_score and index < dir_index(best.direction)):
            best = step
            best_score = score
    return best, reason, goal


def _goal(ct: unswbc.Controller, view: View, memory: Memory) -> tuple[tuple[int, int] | None, str]:
    length = ct.get_length()
    if length <= MISSILE_MAX_LENGTH and ct.get_unit_count() >= 2:
        longs = [enemy for enemy in view.enemies if enemy.head is not None and worth_killing(enemy.visible, length)]
        if longs:
            target = max(longs, key=lambda enemy: (enemy.visible, -torus_dist(view.head, enemy.head, view.w, view.h)))
            return target.head, "stalk"

    pearls = {pearl for pearl in memory.pearls if pearl != view.head}
    pearl = memory.nearest(view.head, pearls)
    if pearl is not None:
        return pearl, "pearl"
    soon = memory.nearest(view.head, memory.soon())
    if soon is not None:
        return soon, "soon"
    frontier = memory.nearest(view.head, memory.frontier())
    if frontier is not None:
        return frontier, "explore"
    return None, "wander"


def _score(ct: unswbc.Controller, view: View, step: StepInfo, goal: tuple[int, int] | None, reason: str) -> int:
    score = 0
    length = ct.get_length()
    stalking = reason == "stalk"
    if step.pearl:
        if stalking and length >= 4:
            score += 80
        elif length < 4:
            score += 900
        else:
            score += 600
    if 0 <= step.eta <= 8:
        score += 30 - step.eta * 2
    if step.hard_trap:
        score -= 2000
    elif step.soft_trap:
        score -= 250
    score += min(step.pocket, 12) * 3

    if goal is not None:
        here = torus_dist(view.head, goal, view.w, view.h)
        there = torus_dist(step.dest, goal, view.w, view.h)
        # Queen keeps distance from a head it is not allowed to hit.
        if length >= QUEEN_LENGTH and reason == "stalk":
            score += (there - here) * 8
        else:
            score += (here - there) * 12
    else:
        prefer = DIRS[ct.get_id() % 4]
        if step.direction == prefer:
            score += 30
        elif step.direction == prefer.get_left() or step.direction == prefer.get_right():
            score += 10

    for ally in view.ally_heads:
        if torus_dist(step.dest, ally, view.w, view.h) <= 2:
            score -= 35
    if step.direction == ct.get_dir().get_opposite():
        score -= 20
    # Stable spread so split copies do not walk the same line.
    score += (ct.get_id() + dir_index(step.direction)) % 5
    return score


def paint(ct: unswbc.Controller, step: StepInfo, reason: str, goal: tuple[int, int] | None) -> None:
    if goal is not None and reason != "wander":
        color = (255, 70, 70) if reason == "stalk" else (255, 210, 40)
        ct.draw_indicator_dot(Position(goal[0], goal[1]), *color)
    ct.set_indicator_string(f"{reason} {step.direction.value}")
    ct.make_move(step.direction)
