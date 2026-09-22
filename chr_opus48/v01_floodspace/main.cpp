// v1 "floodspace": survive by maximising reachable open space (flood fill),
// grow by heading toward the nearest visible pearl. No sprinting/splitting.
#include "helper.hpp"

#include <array>
#include <queue>
#include <unordered_set>
#include <vector>

using namespace unswbc;

static Game* G = nullptr;
static Controller* CT = nullptr;

// Is stepping `dir` from `here` immediately safe? (empty edge, no dragon on the
// destination tile — kelp/portal edges and any dragon segment are fatal.)
static bool safe_step(Position here, Direction dir) {
    auto const* hereTile = CT->get_tile(here);
    if (!hereTile) return false;
    if (hereTile->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* dest = CT->get_tile(here.add_dir(dir));
    if (!dest) return false;              // just outside vision — treat as blocked
    if (dest->get_dragon()) return false; // kelp handled by edge; dragons are fatal
    return true;
}

// Can we move from tile p across dir into a free (dragon-free, in-vision) tile?
static bool can_traverse(Position p, Direction dir) {
    auto const* t = CT->get_tile(p);
    if (!t) return false;
    if (t->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* nb = CT->get_tile(p.add_dir(dir));
    return nb && !nb->get_dragon();
}

// Count free tiles reachable from `start` within the vision window (open space).
static int flood_space(Position start) {
    std::unordered_set<Position, PositionHash> seen;
    std::queue<Position> q;
    seen.insert(start);
    q.push(start);
    int count = 0;
    while (!q.empty()) {
        Position p = q.front(); q.pop();
        ++count;
        for (auto d : Direction::get_direction_list()) {
            if (!can_traverse(p, d)) continue;
            Position np = p.add_dir(d);
            if (seen.insert(np).second) q.push(np);
        }
    }
    return count;
}

// BFS distance (in steps) from `start` to the nearest visible pearl; big if none.
static int pearl_distance(Position start) {
    std::unordered_set<Position, PositionHash> seen;
    std::queue<std::pair<Position, int>> q;
    seen.insert(start);
    q.push({start, 0});
    while (!q.empty()) {
        auto [p, dist] = q.front(); q.pop();
        auto const* t = CT->get_tile(p);
        if (t && t->has_pearl()) return dist;
        for (auto d : Direction::get_direction_list()) {
            if (!can_traverse(p, d)) continue;
            Position np = p.add_dir(d);
            if (seen.insert(np).second) q.push({np, dist + 1});
        }
    }
    return 999;
}

int main() {
    auto [ct, game] = init();
    CT = &ct; G = &game;

    while (update(ct, game)) {
        Position here = ct.get_position();

        Direction best = Direction::NORTH;
        long bestScore = -1;
        bool found = false;

        for (auto d : Direction::get_direction_list()) {
            if (!safe_step(here, d)) continue;
            Position dest = here.add_dir(d);
            auto const* destTile = ct.get_tile(dest);

            int space = flood_space(dest);
            int pdist = pearl_distance(dest);
            bool eats = destTile && destTile->has_pearl();

            // Survival dominates (open space); pearls pull us toward food and
            // eating one right now is a bonus (growth for the length tiebreak).
            long score = (long)space * 100 - (long)pdist * 4 + (eats ? 60 : 0);
            if (!found || score > bestScore) {
                bestScore = score; best = d; found = true;
            }
        }

        ct.make_move(best);   // if nothing was safe, NORTH is the honest last resort
        end_turn();
    }
}
