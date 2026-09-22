// v16 "navigator": rebuild around the fundamentals the strongest bots have and
// mine lacked -- (1) persistent TERRAIN memory (kelp/portal edges + pearl ETAs)
// so we can (2) BFS-pathfind to pearls BEYOND the 7x7 (crude manhattan stalled on
// walls), and (3) an EXPLORATION drive (map unseen tiles -> find more pearls ->
// more coverage -> more length). Keeps the proven swarm + aggression meta.
#include "helper.hpp"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <deque>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace unswbc;

static Controller* CT = nullptr;
static int W = 0, H = 0;

static std::unordered_set<long> seen;      // encoded positions ever seen
static std::unordered_set<long> blocked;   // encoded directed edges known kelp/portal
static std::unordered_map<long, std::pair<int,int>> eta;  // encoded pos -> (countdown, round observed)

static int dir_index(Direction d) {
    switch (d.value) { case Direction::NORTH: return 0; case Direction::EAST: return 1;
                       case Direction::SOUTH: return 2; default: return 3; }
}
static long enc(Position p) { return (long)p.x * H + p.y; }
static long ekey(Position p, Direction d) { return enc(p) * 4 + dir_index(d); }
static int wrap1(int a, int m) { a = std::abs(a); return std::min(a, m - a); }
static int manh(Position a, Position b) { return wrap1(a.x - b.x, W) + wrap1(a.y - b.y, H); }
static int cheb(Position a, Position b) { return std::max(wrap1(a.x - b.x, W), wrap1(a.y - b.y, H)); }

static bool safe_step(Position here, Direction dir) {
    auto const* h = CT->get_tile(here);
    if (!h || h->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* d = CT->get_tile(here.add_dir(dir));
    return d && !d->get_dragon();
}
static int onward_safe(Position p) {
    int n = 0; for (auto d : Direction::get_direction_list()) if (safe_step(p, d)) ++n; return n;
}
static bool can_traverse(Position p, Direction dir) {
    auto const* t = CT->get_tile(p);
    if (!t || t->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* nb = CT->get_tile(p.add_dir(dir));
    return nb && !nb->get_dragon();
}
static int flood_space(Position start, int cap = 80) {
    std::unordered_set<long> vis{enc(start)}; std::queue<Position> q; q.push(start); int n = 0;
    while (!q.empty() && n < cap) { Position p = q.front(); q.pop(); ++n;
        for (auto d : Direction::get_direction_list())
            if (can_traverse(p, d)) { Position np = p.add_dir(d); if (vis.insert(enc(np)).second) q.push(np); } }
    return n;
}

// Shortest path over the REMEMBERED map (unknown edges assumed open; known
// kelp/portal + in-view dragons are obstacles). Returns first step toward the
// nearest goal, or nullopt.
template <class GoalFn>
static std::optional<Direction> bfs_first(Position start, GoalFn is_goal,
                                          std::unordered_set<long> const& occ,
                                          int maxDepth, int maxNodes) {
    if (is_goal(start)) return std::nullopt;
    std::unordered_map<long, int> firstOf;  // node enc -> first-step dir index
    std::deque<std::pair<Position,int>> q; q.push_back({start, 0});
    std::unordered_set<long> vis{enc(start)};
    auto DIRS = Direction::get_direction_list();
    while (!q.empty() && (int)vis.size() < maxNodes) {
        auto [p, depth] = q.front(); q.pop_front();
        if (depth >= maxDepth) continue;
        for (auto d : DIRS) {
            if (blocked.count(ekey(p, d))) continue;
            auto const* live = CT->get_tile(p);
            if (live && live->get_edge(d).get_edge_type() != EdgeType::EMPTY) continue;
            Position np = p.add_dir(d); long ne = enc(np);
            if (vis.count(ne) || occ.count(ne)) continue;
            vis.insert(ne);
            int firstDir = (enc(p) == enc(start)) ? dir_index(d) : firstOf[enc(p)];
            firstOf[ne] = firstDir;
            if (is_goal(np)) return DIRS[firstDir];
            q.push_back({np, depth + 1});
        }
    }
    return std::nullopt;
}

int main() {
    auto [ct, game] = init();
    CT = &ct;
    auto [w, h] = game.get_map_size(); W = w; H = h;
    int const target = std::clamp((W * H) / 10, 8, 62);
    bool const small = (W * H) <= 220;

    while (update(ct, game)) {
        int round = game.get_round_num();
        Position here = ct.get_position();
        int len = ct.get_length();
        int pop = ct.get_unit_count();
        Team me = ct.get_team();

        std::unordered_set<long> occ;
        std::vector<Position> enemyHeads, friends, visiblePearls;
        for (auto const& t : ct.get_tiles()) {
            Position p = t.get_position();
            seen.insert(enc(p));
            int pt = t.get_pearl_time();
            if (t.has_pearl()) { eta[enc(p)] = {0, round}; visiblePearls.push_back(p); }
            else if (pt >= 0) eta[enc(p)] = {pt, round};
            else eta.erase(enc(p));
            for (auto d : Direction::get_direction_list())
                if (t.get_edge(d).get_edge_type() != EdgeType::EMPTY) {
                    blocked.insert(ekey(p, d)); blocked.insert(ekey(p.add_dir(d), d.get_opposite()));
                }
            auto const* dr = t.get_dragon();
            if (dr) { occ.insert(enc(p));
                if (dr->get_team() == me) { if (p.x != here.x || p.y != here.y) friends.push_back(p); }
                else if (dr->is_head()) enemyHeads.push_back(p); }
        }
        occ.erase(enc(here));

        if (pop < target && len >= 4 && ct.can_split(2)) { ct.do_split(2); end_turn(); continue; }

        std::unordered_set<long> pearlGoals;
        for (auto const& p : visiblePearls) pearlGoals.insert(enc(p));
        for (auto const& [e, cr] : eta) { int eff = cr.first - (round - cr.second); if (eff <= 2) pearlGoals.insert(e); }

        std::optional<Direction> planDir;
        if (!pearlGoals.empty())
            planDir = bfs_first(here, [&](Position q){ return pearlGoals.count(enc(q)) > 0; }, occ, 20, 300);
        if (!planDir)
            planDir = bfs_first(here, [&](Position q){ return seen.count(enc(q)) == 0; }, occ, 20, 300);

        Direction best = Direction::NORTH; long bestScore = LONG_MIN; bool found = false;
        for (auto d : Direction::get_direction_list()) {
            if (!safe_step(here, d)) continue;
            Position dest = here.add_dir(d);
            auto const* dt = ct.get_tile(dest);
            int opens = onward_safe(dest);
            int space = flood_space(dest);
            int crowd = 0; for (auto const& f : friends) if (cheb(dest, f) <= 2) ++crowd;
            bool eats = dt && dt->has_pearl();

            long score = (long)opens * 30 + (long)std::min(space, 40) * 3;
            if (planDir && d == *planDir) score += 500;
            if (eats) score += 800;
            if (opens <= 1 && !eats) score -= 80;
            auto it = eta.find(enc(dest));
            if (it != eta.end()) { int eff = it->second.first - (round - it->second.second); if (eff >= 0 && eff <= 6) score += 40 - eff * 4; }
            if (seen.count(enc(dest)) == 0) score += 18;
            score -= (long)crowd * 12;
            if (!small) { int danger = 0; for (auto const& eh : enemyHeads) if (cheb(dest, eh) <= 1) ++danger; score -= (long)danger * 70; }
            if (space < len) score -= (long)(len - space) * 40;

            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found) { int bs = -1; for (auto d : Direction::get_direction_list())
            if (safe_step(here, d)) { int s = flood_space(here.add_dir(d)); if (s > bs) { bs = s; best = d; found = true; } } }

        ct.make_move(best);
        end_turn();
    }
}
