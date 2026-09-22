// v6 "adaptive": scale strategy to map size (the failure was one-size-fits-all).
//  - small maps: leaner swarm, stay nimble (shed length when long), survival-first,
//    do NOT avoid enemy heads (timidity gets you boxed in and killed).
//  - big maps: aggressive swarm + harvest + disperse for pearl share.
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
static int flood_space(Position start, int cap = 90) {
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
    bool const small = (W * H) <= 220;
    int const target = small ? std::clamp((W * H) / 14, 5, 10)
                             : std::clamp((W * H) / 10, 10, 44);

    while (update(ct, game)) {
        int round = game.get_round_num();
        Position here = ct.get_position();
        int len = ct.get_length();
        int pop = ct.get_unit_count();

        std::vector<Position> enemyHeads, friends;
        Team me = ct.get_team();
        for (auto const& t : ct.get_tiles()) {
            int pt = t.get_pearl_time(); Position p = t.get_position();
            if (t.has_pearl()) mem[p] = {0, round};
            else if (pt >= 0) mem[p] = {pt, round};
            else mem.erase(p);
            auto const* dr = t.get_dragon();
            if (dr) {
                if (dr->get_team() == me) { if (p.x != here.x || p.y != here.y) friends.push_back(p); }
                else if (dr->is_head()) enemyHeads.push_back(p);
            }
        }
        if (mem.size() > 500) {
            std::vector<std::pair<int, Position>> v; v.reserve(mem.size());
            for (auto const& [p, _] : mem) v.push_back({manh(here, p), p});
            std::nth_element(v.begin(), v.begin() + 300, v.end(),
                             [](auto const& a, auto const& b) { return a.first < b.first; });
            for (size_t i = 300; i < v.size(); ++i) mem.erase(v[i].second);
        }

        // Split policy. Small maps: keep a lean population AND stay nimble by
        // shedding length once long (long bodies self-trap in tight space).
        bool wantSplit = (pop < target) || (small && len >= 7 && pop < 24);
        if (wantSplit && len >= 4 && ct.can_split(2)) { ct.do_split(2); end_turn(); continue; }

        bool pearlInView = false;
        for (auto const& t : ct.get_tiles()) if (t.has_pearl()) { pearlInView = true; break; }
        bool haveTarget = false; Position tgt;
        if (!pearlInView) {
            long bestKey = LONG_MAX;
            for (auto const& [p, cr] : mem) {
                int eff = cr.first - (round - cr.second);
                long key = (long)manh(here, p) + (long)std::max(eff, 0) * 2;
                if (key < bestKey) { bestKey = key; tgt = p; haveTarget = true; }
            }
        }

        Direction best = Direction::NORTH; long bestScore = LONG_MIN; bool found = false;
        for (auto d : Direction::get_direction_list()) {
            if (!safe_step(here, d)) continue;
            Position dest = here.add_dir(d);
            auto const* dt = ct.get_tile(dest);
            int space = flood_space(dest);
            int crowd = 0;
            for (auto const& f : friends) if (cheb(dest, f) <= 2) ++crowd;

            long score = (long)space * 100 - (long)crowd * 15;  // survival-first + spread
            if (!small) {
                int danger = 0;
                for (auto const& eh : enemyHeads) if (cheb(dest, eh) <= 1) ++danger;
                score -= (long)danger * 110;
            }
            if (pearlInView) {
                score -= (long)pearl_distance(dest) * 6;
                if (dt && dt->has_pearl()) score += 120;
            } else if (haveTarget) {
                score -= (long)manh(dest, tgt) * 5;
            }
            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found) {  // boxed in: take the safe move with the most space, else NORTH
            int bestSpace = -1;
            for (auto d : Direction::get_direction_list())
                if (safe_step(here, d)) { int s = flood_space(here.add_dir(d)); if (s > bestSpace) { bestSpace = s; best = d; found = true; } }
        }

        ct.make_move(best);
        end_turn();
    }
}
