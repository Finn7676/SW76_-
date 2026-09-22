// v4 "controlled": memory swarm tuned to DIE LESS and accumulate length.
// Two swarms usually survive to round 500 -> winner decided on length, so the
// game is really about out-harvesting pearls while avoiding deaths (a death
// drops half your length as pearls the enemy can eat).
//  - right-sized swarm, never split when cramped
//  - avoid stepping into a region smaller than our body (self-trap)
//  - stop splitting late; grow champions for the length tiebreak
#include "helper.hpp"

#include <algorithm>
#include <climits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace unswbc;

static Controller* CT = nullptr;
static int W = 0, H = 0;
static std::unordered_map<Position, std::pair<int, int>, PositionHash> mem;

static int wrap1(int a, int m) { a = std::abs(a); return std::min(a, m - a); }
static int manh(Position a, Position b) { return wrap1(a.x - b.x, W) + wrap1(a.y - b.y, H); }
static int cheb(Position a, Position b) { return std::max(wrap1(a.x - b.x, W), wrap1(a.y - b.y, H)); }

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
static int flood_space(Position start, int cap = 80) {
    std::unordered_set<Position, PositionHash> seen{start};
    std::queue<Position> q; q.push(start); int n = 0;
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
    int const target = std::clamp((W * H) / 16, 5, 22);

    while (update(ct, game)) {
        int round = game.get_round_num();
        Position here = ct.get_position();
        int len = ct.get_length();
        int pop = ct.get_unit_count();

        for (auto const& t : ct.get_tiles()) {
            int pt = t.get_pearl_time(); Position p = t.get_position();
            if (t.has_pearl()) mem[p] = {0, round};
            else if (pt >= 0) mem[p] = {pt, round};
            else mem.erase(p);
        }
        if (mem.size() > 500) {
            std::vector<std::pair<int, Position>> v; v.reserve(mem.size());
            for (auto const& [p, _] : mem) v.push_back({manh(here, p), p});
            std::nth_element(v.begin(), v.begin() + 300, v.end(),
                             [](auto const& a, auto const& b) { return a.first < b.first; });
            for (size_t i = 300; i < v.size(); ++i) mem.erase(v[i].second);
        }

        // Expansion phase: build a swarm, but never split when cramped (a child
        // born in a tight spot just dies and feeds the enemy). Stop late.
        bool expanding = round < 330 && pop < target;
        if (expanding && len >= 4 && ct.can_split(2) && flood_space(here) >= 8) {
            ct.do_split(2); end_turn(); continue;
        }

        std::vector<Position> enemyHeads;
        Team me = ct.get_team();
        for (auto const& t : ct.get_tiles()) {
            auto const* dr = t.get_dragon();
            if (dr && dr->is_head() && !(dr->get_team() == me)) enemyHeads.push_back(t.get_position());
        }

        bool pearlInView = false;
        for (auto const& t : ct.get_tiles()) if (t.has_pearl()) { pearlInView = true; break; }
        bool haveTarget = false; Position target_pos;
        if (!pearlInView) {
            long bestKey = LONG_MAX;
            for (auto const& [p, cr] : mem) {
                int eff = cr.first - (round - cr.second);
                long key = (long)manh(here, p) + (long)std::max(eff, 0) * 2;
                if (key < bestKey) { bestKey = key; target_pos = p; haveTarget = true; }
            }
        }

        Direction best = Direction::NORTH; long bestScore = LONG_MIN; bool found = false;
        for (auto d : Direction::get_direction_list()) {
            if (!safe_step(here, d)) continue;
            Position dest = here.add_dir(d);
            auto const* dt = ct.get_tile(dest);
            int space = flood_space(dest);
            int danger = 0;
            for (auto const& eh : enemyHeads) if (cheb(dest, eh) <= 1) ++danger;

            long score = (long)space * 60 - (long)danger * 100;
            if (space < len) score -= (long)(len - space) * 220;  // would trap our own body
            if (pearlInView) {
                score -= (long)pearl_distance(dest) * 5;
                if (dt && dt->has_pearl()) score += 100;
            } else if (haveTarget) {
                score -= (long)manh(dest, target_pos) * 5;
            }
            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found)
            for (auto d : Direction::get_direction_list())
                if (safe_step(here, d)) { best = d; found = true; break; }

        ct.make_move(best);
        end_turn();
    }
}
