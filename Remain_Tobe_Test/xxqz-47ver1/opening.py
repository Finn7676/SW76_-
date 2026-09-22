"""Early population, and splitting out of a hole instead of walking into it."""

import helper as unswbc

from model import EARLY_ROUND, LATE_ROUND, MISSILE_MAX_LENGTH, QUEEN_LENGTH, population_target, worth_killing
from sense import View


def want_voluntary(ct: unswbc.Controller, game: unswbc.Game, view: View) -> bool:
    """Split off a length-2 child while the team is still small."""
    length = ct.get_length()
    if length < 4 or length > 8 or length >= QUEEN_LENGTH:
        return False
    round_num = game.get_round_num()
    if round_num > LATE_ROUND:
        return False
    units = ct.get_unit_count()
    area = view.w * view.h
    if round_num > EARLY_ROUND and units >= 4:
        return False
    if units >= population_target(area):
        return False
    if not ct.can_split(2):
        return False
    if not any(not step.hard_trap for step in view.safe):
        return False
    # A short dragon that can already see a long head keeps its length for the dive.
    if length <= MISSILE_MAX_LENGTH and units >= 2:
        if any(enemy.head is not None and worth_killing(enemy.visible, length) for enemy in view.enemies):
            return False
    # Length 4 or 5 eats an adjacent pearl first, so the next split leaves a longer parent.
    if length < 6 and any(step.pearl and not step.hard_trap for step in view.safe):
        return False
    return True


def want_emergency(ct: unswbc.Controller, view: View) -> bool:
    """No open tile. Become a length-2 head instead of entering the dead end."""
    if ct.get_length() < 4 or not ct.can_split(2):
        return False
    if any(not step.hard_trap for step in view.safe):
        return False
    return True


def split_two(ct: unswbc.Controller) -> bool:
    if not ct.can_split(2):
        return False
    ct.do_split(2)
    ct.set_indicator_string("split 2")
    return True


def split_down(ct: unswbc.Controller) -> bool:
    child = ct.get_length() - 2
    if child < 2 or not ct.can_split(child):
        child = 2
    if not ct.can_split(child):
        return False
    ct.do_split(child)
    ct.set_indicator_string(f"split trap {child}")
    return True
