#pragma once

#include "sense.hpp"
#include "sonar.hpp"

namespace xxqz {

struct Perception {
    View view;
    // Directly visible, unoccupied pearls eligible for broadcasting. The selected
    // movement's consumed destination is excluded later, after policy runs.
    std::vector<Position> visible_pearls;
    std::vector<Position> visible_enemy_heads;
    int round = 0;
};

inline Perception sense_nearby(Controller const& ct, Game const& game, Memory& map,
                               StrategyConfig const& config = strategy) {
    Perception result;
    result.round = game.get_round_num();
    map.begin_turn(game, config);
    ViewBuilder builder(ct, game);
    int my_id = ct.get_id();
    auto my_team = ct.get_team();
    for (auto const& tile : ct.get_tiles()) {
        record_tile(&DebugStats::perception_tiles_scanned);
        Position key = tile.get_position();
        Cell cell = read_cell(tile, my_id, my_team);
        map.observe_cell(key, cell);
        builder.add(key, cell);
        if (cell.pearl && cell.owner == Owner::Empty)
            result.visible_pearls.push_back(key);
        if (cell.owner == Owner::Foe && cell.is_head)
            result.visible_enemy_heads.push_back(key);
    }
    // Reconcile every direct sighting before admitting timestamped team reports.
    receive_sonar(ct, game, map, config);
    result.view = builder.finish(ct);
    return result;
}

}  // namespace xxqz
