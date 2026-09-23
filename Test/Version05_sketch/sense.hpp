#pragma once

#include "model.hpp"
#include "debug.hpp"

#include <array>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xxqz {

using unswbc::Controller;
using unswbc::EdgeType;
using unswbc::Game;

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

struct StepInfo {
    Direction direction = Direction::NORTH;
    Position dest;
    bool pearl = false;
    int eta = -1;
    bool hard_trap = false;
    bool soft_trap = false;
    int pocket = 0;
};

struct Enemy {
    int id = 0;
    bool has_head = false;
    Position head;
    int visible = 0;
};

struct View {
    Position head;
    std::vector<Position> body;
    bool body_complete = false;
    std::vector<StepInfo> safe;
    std::vector<Enemy> enemies;
    std::vector<Position> ally_heads;
    std::unordered_map<Position, Cell, PositionHash> snap;
    int w = 0;
    int h = 0;
};

inline bool contains_pos(std::vector<Position> const& cells, Position pos) {
    for (Position const& cell : cells)
        if (cell == pos)
            return true;
    return false;
}

inline std::tuple<int, bool, bool> pocket(Position dest, View const& view, int length) {
    record(&DebugStats::pocket_calls);
    std::unordered_set<Position, PositionHash> tiles;
    std::deque<Position> queue;
    tiles.insert(dest);
    queue.push_back(dest);
    bool escapes = false;
    while (!queue.empty()) {
        Position cur = queue.front();
        queue.pop_front();
        record(&DebugStats::pocket_nodes);
        auto found = view.snap.find(cur);
        if (found == view.snap.end()) {
            escapes = true;
            continue;
        }
        Cell const& cell = found->second;
        for (int index = 0; index < 4; index++) {
            if (cell.edges[static_cast<std::size_t>(index)] != EdgeType::EMPTY)
                continue;
            Position nxt = cur.add_dir(directions()[static_cast<std::size_t>(index)]);
            if (tiles.count(nxt))
                continue;
            auto nxt_cell = view.snap.find(nxt);
            if (nxt_cell == view.snap.end()) {
                escapes = true;
                continue;
            }
            if (nxt_cell->second.owner != Owner::Empty)
                continue;
            tiles.insert(nxt);
            queue.push_back(nxt);
        }
    }

    int min_deg = 99;
    int max_deg = 0;
    for (Position const& cur : tiles) {
        record(&DebugStats::pocket_degree_nodes);
        Cell const& cell = view.snap.at(cur);
        int degree = 0;
        for (int index = 0; index < 4; index++) {
            if (cell.edges[static_cast<std::size_t>(index)] != EdgeType::EMPTY)
                continue;
            Position nxt = cur.add_dir(directions()[static_cast<std::size_t>(index)]);
            if (tiles.count(nxt))
                degree++;
        }
        min_deg = std::min(min_deg, degree);
        max_deg = std::max(max_deg, degree);
    }
    if (tiles.empty())
        min_deg = 0;

    bool hard = false;
    bool soft = false;
    if (!escapes) {
        if (max_deg <= 2 && min_deg <= 1)
            hard = true;
        else if (static_cast<int>(tiles.size()) < length)
            hard = true;
    } else if (max_deg <= 2) {
        soft = true;
    }
    return {static_cast<int>(tiles.size()), hard, soft};
}

inline std::vector<Position> reconstruct_body(Controller const& ct, View const& view,
                                              std::unordered_map<Position, Direction, PositionHash> const& mine) {
    Direction facing = ct.get_dir();
    Position pos = view.head;
    std::vector<Position> chain{pos};
    for (int step = 0; step < ct.get_length() - 1; step++) {
        Position nxt = pos.add_dir(facing.get_opposite());
        auto segment = mine.find(nxt);
        if (segment == mine.end() || contains_pos(chain, nxt))
            break;
        chain.push_back(nxt);
        facing = segment->second;
        pos = nxt;
    }
    return chain;
}

inline std::vector<Enemy> collect_enemies(std::unordered_map<Position, Cell, PositionHash> const& snap) {
    std::unordered_map<int, int> counts;
    std::unordered_map<int, Position> heads;
    for (auto const& [pos, cell] : snap) {
        record(&DebugStats::enemy_group_tiles);
        if (cell.owner != Owner::Foe || cell.dragon_id < 0)
            continue;
        counts[cell.dragon_id] += 1;
        if (cell.is_head)
            heads[cell.dragon_id] = pos;
    }
    std::vector<Enemy> enemies;
    enemies.reserve(counts.size());
    for (auto const& [dragon_id, count] : counts) {
        Enemy enemy;
        enemy.id = dragon_id;
        enemy.visible = count;
        auto head = heads.find(dragon_id);
        if (head != heads.end()) {
            enemy.has_head = true;
            enemy.head = head->second;
        }
        enemies.push_back(enemy);
    }
    return enemies;
}

inline std::vector<StepInfo> safe_steps(Controller const& ct, View const& view) {
    auto origin = view.snap.find(view.head);
    if (origin == view.snap.end())
        return {};
    std::vector<StepInfo> steps;
    for (int index = 0; index < 4; index++) {
        if (origin->second.edges[static_cast<std::size_t>(index)] != EdgeType::EMPTY)
            continue;
        Direction direction = directions()[static_cast<std::size_t>(index)];
        Position dest = view.head.add_dir(direction);
        auto cell = view.snap.find(dest);
        if (cell == view.snap.end() || cell->second.owner != Owner::Empty)
            continue;
        auto [size, hard, soft] = pocket(dest, view, ct.get_length());
        StepInfo step;
        step.direction = direction;
        step.dest = dest;
        step.pearl = cell->second.pearl;
        step.eta = cell->second.eta;
        step.hard_trap = hard;
        step.soft_trap = soft;
        step.pocket = size;
        steps.push_back(step);
    }
    return steps;
}

inline View look(Controller const& ct, Game const& game) {
    View view;
    auto [width, height] = game.get_map_size();
    view.w = width;
    view.h = height;
    view.head = ct.get_position();
    int my_id = ct.get_id();
    auto my_team = ct.get_team();
    std::unordered_map<Position, Direction, PositionHash> mine;

    for (auto const& tile : ct.get_tiles()) {
        record_tile(&DebugStats::sense_tiles_scanned);
        Position key = tile.get_position();
        Cell cell;
        cell.pearl = tile.has_pearl();
        cell.eta = tile.get_pearl_time();
        for (int index = 0; index < 4; index++)
            cell.edges[static_cast<std::size_t>(index)] =
                tile.get_edge(directions()[static_cast<std::size_t>(index)]).get_edge_type();
        if (auto const* part = tile.get_dragon()) {
            cell.dragon_id = part->get_id();
            cell.is_head = part->is_head();
            cell.facing = part->get_dir();
            if (part->get_id() == my_id) {
                cell.owner = Owner::Me;
                mine.emplace(key, part->get_dir());
            } else if (part->get_team() == my_team) {
                cell.owner = Owner::Ally;
                if (part->is_head())
                    view.ally_heads.push_back(key);
            } else {
                cell.owner = Owner::Foe;
            }
        }
        view.snap.emplace(key, cell);
    }

    view.body = reconstruct_body(ct, view, mine);
    view.body_complete = static_cast<int>(view.body.size()) == ct.get_length()
        && static_cast<int>(std::unordered_set<Position, PositionHash>(view.body.begin(), view.body.end()).size())
            == static_cast<int>(view.body.size());
    view.enemies = collect_enemies(view.snap);
    view.safe = safe_steps(ct, view);
    return view;
}

inline Direction last_resort(Controller const& ct) {
    record(&DebugStats::fallback_calls);
    Position here = ct.get_position();
    auto const* tile = ct.get_tile(here);
    int my_id = ct.get_id();
    auto my_team = ct.get_team();
    Direction best = Direction::NORTH;
    int best_penalty = 1000000000;
    int best_index = 99;
    for (int index = 0; index < 4; index++) {
        Direction direction = directions()[static_cast<std::size_t>(index)];
        int penalty = 0;
        EdgeType edge = tile ? tile->get_edge(direction).get_edge_type() : EdgeType::KELP;
        if (edge == EdgeType::KELP)
            penalty += 5000;
        else if (edge == EdgeType::PORTAL)
            penalty += 900;
        auto const* ahead = ct.get_tile(here.add_dir(direction));
        auto const* part = ahead ? ahead->get_dragon() : nullptr;
        if (part) {
            if (part->get_team() == my_team && part->get_id() != my_id && part->is_head())
                penalty += 8000;
            else if (part->get_id() == my_id)
                penalty += 3200;
            else if (part->get_team() == my_team)
                penalty += 3000;
            else if (part->is_head())
                penalty += 100;
            else
                penalty += 1800;
        }
        if (direction == ct.get_dir().get_opposite())
            penalty += 40;
        if (penalty < best_penalty || (penalty == best_penalty && index < best_index)) {
            best = direction;
            best_penalty = penalty;
            best_index = index;
        }
    }
    return best;
}

}  // namespace xxqz
