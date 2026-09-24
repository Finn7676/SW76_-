#pragma once

#include "model.hpp"
#include "strategy.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xxqz {

using unswbc::Controller;
using unswbc::EdgeType;
using unswbc::Game;

struct EdgeKey {
    int x = 0;
    int y = 0;
    int dir = 0;
    bool operator==(EdgeKey const& other) const {
        return x == other.x && y == other.y && dir == other.dir;
    }
};

struct EdgeKeyHash {
    std::size_t operator()(EdgeKey const& key) const {
        return (static_cast<std::size_t>(key.x) << 20) ^ (static_cast<std::size_t>(key.y) << 4) ^ static_cast<std::size_t>(key.dir);
    }
};

class Memory {
  public:
    int w = 0;
    int h = 0;
    std::unordered_set<Position, PositionHash> seen;
    std::unordered_map<EdgeKey, EdgeType, EdgeKeyHash> edges;
    std::unordered_set<Position, PositionHash> pearls;
    std::unordered_map<Position, int, PositionHash> eta;
    // Direct sightings and team reports have independent lifetimes.
    std::unordered_map<Position, int, PositionHash> observed_at;
    std::unordered_map<Position, int, PositionHash> shared_pearls;
    int last_round = 0;

    void configure(int width, int height) {
        w = width;
        h = height;
    }

    void observe(Controller const& ct, Game const& game, StrategyConfig const& config = strategy) {
        if (w == 0) {
            auto [width, height] = game.get_map_size();
            configure(width, height);
        }
        int round_num = game.get_round_num();
        int delta = last_round ? std::max(0, round_num - last_round) : 0;
        for (auto it = eta.begin(); it != eta.end();) {
            it->second -= delta;
            if (it->second < 0)
                it = eta.erase(it);
            else
                ++it;
        }
        // A spawn timer expiring does NOT mean an existing pearl disappeared.
        for (auto it = pearls.begin(); it != pearls.end();) {
            auto seen_time = observed_at.find(*it);
            if (seen_time == observed_at.end() || round_num - seen_time->second > config.own_pearl_ttl)
                it = pearls.erase(it);
            else
                ++it;
        }
        for (auto it = shared_pearls.begin(); it != shared_pearls.end();) {
            if (round_num - it->second > config.sonar_ttl)
                it = shared_pearls.erase(it);
            else
                ++it;
        }
        last_round = round_num;

        for (auto const& tile : ct.get_tiles()) {
            Position key = tile.get_position();
            seen.insert(key);
            observed_at[key] = round_num;
            shared_pearls.erase(key); // Local observation always wins, including empty tiles.
            if (tile.has_pearl())
                pearls.insert(key);
            else
                pearls.erase(key);
            if (tile.get_pearl_time() >= 0)
                eta[key] = tile.get_pearl_time();
            else
                eta.erase(key);
            for (int index = 0; index < 4; index++) {
                EdgeType kind = tile.get_edge(directions()[static_cast<std::size_t>(index)]).get_edge_type();
                edges[EdgeKey{key.x, key.y, index}] = kind;
                Position neighbor = key.add_dir(directions()[static_cast<std::size_t>(index)]);
                edges[EdgeKey{neighbor.x, neighbor.y, (index + 2) % 4}] = kind;
            }
        }
    }

    std::optional<Position> nearest(Position origin, std::unordered_set<Position, PositionHash> const& cells) const {
        std::optional<Position> best;
        int best_dist = 1000000000;
        for (Position const& cell : cells) {
            int dist = torus_dist(origin, cell, w, h);
            bool closer = dist < best_dist;
            bool tie = best && dist == best_dist && (cell.x < best->x || (cell.x == best->x && cell.y < best->y));
            if (!best || closer || tie) {
                best = cell;
                best_dist = dist;
            }
        }
        return best;
    }

    std::unordered_set<Position, PositionHash> frontier() const {
        std::unordered_set<Position, PositionHash> cells;
        for (Position const& pos : seen) {
            for (int index = 0; index < 4; index++) {
                auto edge = edges.find(EdgeKey{pos.x, pos.y, index});
                if (edge == edges.end() || edge->second != EdgeType::EMPTY)
                    continue;
                Position nxt = pos.add_dir(directions()[static_cast<std::size_t>(index)]);
                if (!seen.count(nxt))
                    cells.insert(nxt);
            }
        }
        return cells;
    }

    std::unordered_set<Position, PositionHash> soon() const {
        std::unordered_set<Position, PositionHash> cells;
        for (auto const& [pos, time] : eta) {
            if (time >= 0 && time <= 10 && !pearls.count(pos))
                cells.insert(pos);
        }
        return cells;
    }
};

inline Memory memory;

}  // namespace xxqz
