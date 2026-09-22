"""Shared geometry and the numbers the tactics agree on."""

from helper import Direction

DIRS = Direction.get_direction_list()
DIR_INDEX = {direction: index for index, direction in enumerate(DIRS)}

# Split aggressively until this round, then keep at least a few heads.
EARLY_ROUND = 220
# After this, stop voluntary splits and grow length.
LATE_ROUND = 350
# Only a short dragon may spend itself on an enemy head.
MISSILE_MAX_LENGTH = 6
# A dragon this long is our length piece: never split, never dive.
QUEEN_LENGTH = 12
# Minimum enemy segments we must actually see before a dive.
LONG_ENEMY = 7


def dir_index(direction: Direction) -> int:
    return DIR_INDEX[direction]


def step_xy(x: int, y: int, direction: Direction, width: int, height: int) -> tuple[int, int]:
    dx, dy = direction.get_offset()
    return (x + dx) % width, (y + dy) % height


def torus_delta(a: int, b: int, size: int) -> int:
    delta = (b - a) % size
    if delta > size - delta:
        return delta - size
    return delta


def torus_dist(a: tuple[int, int], b: tuple[int, int], width: int, height: int) -> int:
    return abs(torus_delta(a[0], b[0], width)) + abs(torus_delta(a[1], b[1], height))


def population_target(area: int) -> int:
    """How many dragons to build before everyone switches to growing."""
    if area <= 160:
        return 8
    if area <= 500:
        return 12
    if area <= 1400:
        return 16
    return 20


def worth_killing(visible_segments: int, my_length: int) -> bool:
    """True when the part of the enemy we can see is clearly longer than us."""
    return visible_segments >= max(LONG_ENEMY, my_length + 4)
