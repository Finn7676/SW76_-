#pragma once

#include "forage.hpp"
#include "kamikaze.hpp"
#include "opening.hpp"

#include <string>

namespace xxqz {

inline void act(Controller& ct, Game& game) {
    if (memory.w == 0) {
        auto [width, height] = game.get_map_size();
        memory.configure(width, height);
    }
    memory.observe(ct, game);
    View view = look(ct, game);

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

    if (want_voluntary(ct, game, view) && split_two(ct))
        return;
    if (want_emergency(ct, view) && split_down(ct))
        return;

    if (auto picked = choose(ct, view, memory)) {
        paint(ct, *picked);
        return;
    }

    ct.set_indicator_string("fallback");
    ct.make_move(last_resort(ct));
}

}  // namespace xxqz
