#pragma once

#include "model.hpp"

namespace xxqz {

using unswbc::EdgeType;

enum class Owner { Empty, Me, Ally, Foe };

struct Cell {
    bool pearl = false;
    int eta = -1;
    Owner owner = Owner::Empty;
    int dragon_id = -1;
    bool is_head = false;
    Direction facing = Direction::NORTH;
    std::array<EdgeType, 4> edges{EdgeType::EMPTY, EdgeType::EMPTY, EdgeType::EMPTY, EdgeType::EMPTY};
};

// Decode each raw tile once before distributing the current-turn facts.
inline Cell read_cell(unswbc::Tile const& tile, int my_id, unswbc::Team my_team) {
    Cell cell;
    cell.pearl = tile.has_pearl();
    cell.eta = tile.get_pearl_time();
    for (int index = 0; index < 4; ++index)
        cell.edges[static_cast<std::size_t>(index)] =
            tile.get_edge(directions()[static_cast<std::size_t>(index)]).get_edge_type();
    if (auto const* part = tile.get_dragon()) {
        cell.dragon_id = part->get_id();
        cell.is_head = part->is_head();
        cell.facing = part->get_dir();
        cell.owner = cell.dragon_id == my_id ? Owner::Me
            : part->get_team() == my_team ? Owner::Ally : Owner::Foe;
    }
    return cell;
}

}  // namespace xxqz
