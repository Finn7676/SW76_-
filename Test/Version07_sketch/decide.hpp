#pragma once

#include "forage.hpp"
#include "kamikaze.hpp"
#include "opening.hpp"
#include "perception.hpp"

#include <string>

namespace xxqz {

inline void act(Controller& ct, Game& game, StrategyConfig const& config = strategy) {
    if (memory.w == 0) {
        auto [width, height] = game.get_map_size();
        memory.configure(width, height);
    }
    Perception perception;
    if (config.unified_perception) {
        perception = sense_nearby(ct, game, memory, config);
    } else {
        memory.observe(ct, game, config);
        receive_sonar(ct, game, memory, config);
        perception.view = look(ct, game, config);
    }
    View const& view = perception.view;
    auto broadcast = [&](std::optional<Position> consumed = std::nullopt) {
        if (config.unified_perception)
            broadcast_pearl(ct, game, memory, perception.visible_pearls, consumed, config);
        else
            broadcast_pearl(ct, game, memory, consumed, config);
    };

    if (auto direction = short_trade(ct, view, config)) {
        ct.set_indicator_string("short-trade");
        ct.make_move(*direction);
        return;
    }

    if (auto path = find_dive(ct, view)) {
        std::string label = "kill ";
        for (Direction const& direction : *path)
            label.push_back(static_cast<char>(direction.value));
        ct.set_indicator_string(label);
        if (path->size() == 1)
            ct.make_move(path->front());
        else
            ct.make_moves(*path);
        return;
    }

    if (want_voluntary(ct, game, view) && split_two(ct)) {
        broadcast();
        return;
    }
    if (want_emergency(ct, view) && split_down(ct)) {
        broadcast();
        return;
    }

    if (auto picked = choose(ct, view, memory, config)) {
        paint(ct, *picked);
        broadcast(picked->step.dest);
        memory.pearls.erase(picked->step.dest);
        memory.shared_pearls.erase(picked->step.dest);
        return;
    }

    ct.set_indicator_string("fallback");
    ct.make_move(last_resort(ct));
}

}  // namespace xxqz
