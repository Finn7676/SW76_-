#pragma once

#include "sense.hpp"

#include <string>

namespace xxqz {

inline bool want_voluntary(Controller const& ct, Game const& game, View const& view) {
    int length = ct.get_length();
    if (length < 4 || length > 8 || length >= QUEEN_LENGTH)
        return false;
    int round_num = game.get_round_num();
    if (round_num > LATE_ROUND)
        return false;
    int units = ct.get_unit_count();
    if (round_num > EARLY_ROUND && units >= 4)
        return false;
    if (units >= population_target(view.w * view.h))
        return false;
    if (!ct.can_split(2))
        return false;
    bool has_room = false;
    for (StepInfo const& step : view.safe) {
        if (!step.hard_trap)
            has_room = true;
    }
    if (!has_room)
        return false;
    if (length <= MISSILE_MAX_LENGTH && units >= 2) {
        for (Enemy const& enemy : view.enemies) {
            if (enemy.has_head && worth_killing(enemy.visible, length))
                return false;
        }
    }
    if (length < 6) {
        for (StepInfo const& step : view.safe) {
            if (step.pearl && !step.hard_trap)
                return false;
        }
    }
    return true;
}

inline bool want_emergency(Controller const& ct, View const& view) {
    if (ct.get_length() < 4 || !ct.can_split(2))
        return false;
    for (StepInfo const& step : view.safe) {
        if (!step.hard_trap)
            return false;
    }
    return true;
}

inline bool split_two(Controller& ct) {
    if (!ct.can_split(2))
        return false;
    ct.do_split(2);
    ct.set_indicator_string("split 2");
    return true;
}

inline bool split_down(Controller& ct) {
    int child = ct.get_length() - 2;
    if (child < 2 || !ct.can_split(child))
        child = 2;
    if (!ct.can_split(child))
        return false;
    ct.do_split(child);
    ct.set_indicator_string("split trap " + std::to_string(child));
    return true;
}

}  // namespace xxqz
