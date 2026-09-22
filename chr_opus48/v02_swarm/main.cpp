// v2 "swarm": expand into a population by splitting (un-eliminable + out-harvests
// a lone dragon), then each dragon safely seeks pearls maximising open space.
// Adds enemy-head avoidance. This tests the core meta: splitting.
#include "helper.hpp"

#include <algorithm>
#include <array>
#include <queue>
#include <unordered_set>
#include <vector>

using namespace unswbc;

static Controller* CT = nullptr;
static int W = 0, H = 0;

static int cheb(Position a, Position b) {
    int dx = std::abs(a.x - b.x); dx = std::min(dx, W - dx);
    int dy = std::abs(a.y - b.y); dy = std::min(dy, H - dy);
    return std::max(dx, dy);
}

static bool safe_step(Position here, Direction dir) {
    auto const* h = CT->get_tile(here);
    if (!h || h->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* d = CT->get_tile(here.add_dir(dir));
    return d && !d->get_dragon();
}

static bool can_traverse(Position p, Direction dir) {
    auto const* t = CT->get_tile(p);
    if (!t || t->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* nb = CT->get_tile(p.add_dir(dir));
    return nb && !nb->get_dragon();
}

static int flood_space(Position start, int cap = 60) {
    std::unordered_set<Position, PositionHash> seen{start};
    std::queue<Position> q; q.push(start);
    int n = 0;
    while (!q.empty() && n < cap) {
        Position p = q.front(); q.pop(); ++n;
        for (auto d : Direction::get_direction_list())
            if (can_traverse(p, d)) { Position np = p.add_dir(d); if (seen.insert(np).second) q.push(np); }
    }
    return n;
}

static int pearl_distance(Position start) {
    std::unordered_set<Position, PositionHash> seen{start};
    std::queue<std::pair<Position, int>> q; q.push({start, 0});
    while (!q.empty()) {
        auto [p, dist] = q.front(); q.pop();
        auto const* t = CT->get_tile(p);
        if (t && t->has_pearl()) return dist;
        for (auto d : Direction::get_direction_list())
            if (can_traverse(p, d)) { Position np = p.add_dir(d); if (seen.insert(np).second) q.push({np, dist + 1}); }
    }
    return 999;
}

int main() {
    auto [ct, game] = init();
    CT = &ct;
    auto [w, h] = game.get_map_size(); W = w; H = h;
    int const target = std::clamp((W * H) / 12, 6, 28);  // desired swarm size

    while (update(ct, game)) {
        Position here = ct.get_position();
        int len = ct.get_length();
        int pop = ct.get_unit_count();

        // Expansion: grow a swarm. A team is only eliminated when ALL its dragons
        // die, and more mouths eat more pearls. Split small dragons until we have
        // ~target of them; then everyone grows into long champions.
        if (pop < target && len >= 4 && ct.can_split(2)) {
            ct.do_split(2);
            end_turn();
            continue;
        }

        // Enemy heads currently in view (stepping next to one risks a mutual kill).
        std::vector<Position> enemyHeads;
        Team me = ct.get_team();
        for (auto const& t : ct.get_tiles()) {
            auto const* dr = t.get_dragon();
            if (dr && dr->is_head() && !(dr->get_team() == me)) enemyHeads.push_back(t.get_position());
        }

        Direction best = Direction::NORTH;
        long bestScore = -1; bool found = false;
        for (auto d : Direction::get_direction_list()) {
            if (!safe_step(here, d)) continue;
            Position dest = here.add_dir(d);
            auto const* dt = ct.get_tile(dest);
            int space = flood_space(dest);
            if (space <= 1) continue;                 // dead end — skip unless nothing else
            int pdist = pearl_distance(dest);
            bool eats = dt && dt->has_pearl();
            int danger = 0;
            for (auto const& eh : enemyHeads) if (cheb(dest, eh) <= 1) ++danger;
            long score = (long)space * 100 - (long)pdist * 4 + (eats ? 80 : 0) - (long)danger * 130;
            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        // Fallback: if every safe move looked like a dead end, take any safe move.
        if (!found) {
            for (auto d : Direction::get_direction_list())
                if (safe_step(here, d)) { best = d; found = true; break; }
        }
        ct.make_move(best);
        end_turn();
    }
}
