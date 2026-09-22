// v31 "warden": a TERRITORY-CONTROL breaking change (Tron/lightcycle style). Every prior
// bot's objective is pearls; warden's is SPACE. Each move maximises the free area it would
// CONTROL — a Voronoi flood over the remembered map counting tiles it reaches before the
// nearest enemy head — so the swarm expands its own region and walls the enemy into a
// shrinking pocket until they run out of room and self-crash (win by spatial domination ->
// elimination). Cornering trapped heads is the finisher; pearls are eaten only
// opportunistically (length = bigger walls + tiebreak fallback). Keeps swarm split +
// survival floor + terrain-memory BFS.
#include "helper.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <deque>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace unswbc;

// ------------------------------------------------------------------ globals ---
static Controller* CT = nullptr;
static int W = 0, H = 0;
static bool SMALL = false;   // <=220: tiny, elimination decides
static bool HUNT = false;    // <=300: hunt-primary (tight+medium); else harvest-primary
static int TARGET = 8;

static std::unordered_set<long> g_seen;
static std::unordered_set<long> g_blocked;
static std::unordered_map<long, std::pair<int,int>> g_eta;

static std::vector<Position> t_friends, t_enemyHeads, t_visPearls;
static std::unordered_map<int,int> t_enemySize;
static std::unordered_set<long> t_occupied;

// ------------------------------------------------------------------ geometry -
static int dir_index(Direction d) {
    switch (d.value) { case Direction::NORTH: return 0; case Direction::EAST: return 1;
                       case Direction::SOUTH: return 2; default: return 3; }
}
static long enc(Position p) { return (long)p.x * H + p.y; }
static long ekey(Position p, Direction d) { return enc(p) * 4 + dir_index(d); }
static int wrap1(int a, int m) { a = std::abs(a); return std::min(a, m - a); }
static int manh(Position a, Position b) { return wrap1(a.x - b.x, W) + wrap1(a.y - b.y, H); }
static int cheb(Position a, Position b) { return std::max(wrap1(a.x - b.x, W), wrap1(a.y - b.y, H)); }

// ------------------------------------------------------------------ safety ----
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
static int flood_space(Position start, int cap = 90) {
    std::unordered_set<long> vis{enc(start)}; std::queue<Position> q; q.push(start); int n = 0;
    while (!q.empty() && n < cap) { Position p = q.front(); q.pop(); ++n;
        for (auto d : Direction::get_direction_list())
            if (can_traverse(p, d)) { Position np = p.add_dir(d); if (vis.insert(enc(np)).second) q.push(np); } }
    return n;
}
// How many escape squares an enemy head currently has. Unknown (out-of-vision)
// neighbours are counted as open, so a head only reads as "cornered" when we can
// actually confirm its exits are blocked (no false traps).
static int enemy_escapes(Position eh) {
    auto const* h = CT->get_tile(eh);
    if (!h) return 4;
    int n = 0;
    for (auto d : Direction::get_direction_list()) {
        if (h->get_edge(d).get_edge_type() != EdgeType::EMPTY) continue;
        auto const* nb = CT->get_tile(eh.add_dir(d));
        if (!nb || !nb->get_dragon()) ++n;   // open, or unknown (assume open)
    }
    return n;
}

// ------------------------------------------------------------------ pathing ---
static bool path_edge_open(Position p, Direction d) {
    auto const* live = CT->get_tile(p);
    if (live) return live->get_edge(d).get_edge_type() == EdgeType::EMPTY;
    return g_blocked.count(ekey(p, d)) == 0;
}
template <class GoalFn>
static std::optional<Direction> bfs_first(Position start, GoalFn is_goal, int maxDepth, int maxNodes) {
    if (is_goal(start)) return std::nullopt;
    auto DIRS = Direction::get_direction_list();
    std::unordered_map<long,int> firstDir;
    std::deque<std::pair<Position,int>> q; q.push_back({start, 0});
    std::unordered_set<long> vis{enc(start)};
    while (!q.empty() && (int)vis.size() < maxNodes) {
        auto [p, depth] = q.front(); q.pop_front();
        if (depth >= maxDepth) continue;
        for (auto d : DIRS) {
            if (!path_edge_open(p, d)) continue;
            Position np = p.add_dir(d); long ne = enc(np);
            if (vis.count(ne) || t_occupied.count(ne)) continue;
            vis.insert(ne);
            int fd = (enc(p) == enc(start)) ? dir_index(d) : firstDir[enc(p)];
            firstDir[ne] = fd;
            if (is_goal(np)) return DIRS[fd];
            q.push_back({np, depth + 1});
        }
    }
    return std::nullopt;
}

// Territory estimate (Tron-style Voronoi): flood out from `dest` over the remembered
// map; a tile is counted only if we can reach it (BFS distance) no later than the
// nearest enemy head could (Manhattan lower bound). This is the free area we would
// CONTROL after moving to dest — maximise it to expand our region and wall the enemy
// into a shrinking pocket. With no enemy in view it degrades to plain free space.
static int controlled_space(Position dest, int cap = 80) {
    std::queue<std::pair<Position,int>> q; q.push({dest, 0});
    std::unordered_set<long> vis{enc(dest)};
    int count = 0;
    while (!q.empty() && count < cap) {
        auto [p, dd] = q.front(); q.pop();
        int em = INT_MAX;
        for (auto const& eh : t_enemyHeads) em = std::min(em, manh(p, eh));
        if (dd > em) continue;                    // enemy reaches p first -> not ours
        ++count;
        for (auto d : Direction::get_direction_list()) {
            if (!path_edge_open(p, d)) continue;
            Position np = p.add_dir(d); long ne = enc(np);
            if (t_occupied.count(ne)) continue;
            if (vis.insert(ne).second) q.push({np, dd + 1});
        }
    }
    return count;
}

// ------------------------------------------------------------------ main ------
int main() {
    auto [ct, game] = init();
    CT = &ct;
    auto [w, h] = game.get_map_size(); W = w; H = h;
    SMALL = (W * H) <= 220;
    HUNT = (W * H) <= 300;
    TARGET = std::clamp((W * H) / 10, 8, 62);

    while (update(ct, game)) {
        int round = game.get_round_num();
        Position here = ct.get_position();
        int len = ct.get_length();
        int pop = ct.get_unit_count();
        Team me = ct.get_team();

        // -------- observe --------
        t_friends.clear(); t_enemyHeads.clear(); t_visPearls.clear();
        t_enemySize.clear(); t_occupied.clear();
        for (auto const& t : ct.get_tiles()) {
            Position p = t.get_position(); g_seen.insert(enc(p));
            int pt = t.get_pearl_time();
            if (t.has_pearl()) { g_eta[enc(p)] = {0, round}; t_visPearls.push_back(p); }
            else if (pt >= 0) g_eta[enc(p)] = {pt, round};
            else g_eta.erase(enc(p));
            for (auto d : Direction::get_direction_list())
                if (t.get_edge(d).get_edge_type() != EdgeType::EMPTY) {
                    g_blocked.insert(ekey(p, d));
                    g_blocked.insert(ekey(p.add_dir(d), d.get_opposite()));
                }
            auto const* dr = t.get_dragon();
            if (dr) {
                t_occupied.insert(enc(p));
                if (dr->get_team() == me) { if (p.x != here.x || p.y != here.y) t_friends.push_back(p); }
                else { t_enemySize[dr->get_id()]++; if (dr->is_head()) t_enemyHeads.push_back(p); }
            }
        }
        t_occupied.erase(enc(here));

        // -------- 1) expand the swarm (population = un-eliminable + more hunters) --------
        if (pop < TARGET && len >= 4 && ct.can_split(2)) { ct.do_split(2); end_turn(); continue; }

        // -------- 2) ram an adjacent enemy head (trade-up only, like v22) --------
        // Killing an enemy HEAD removes that whole enemy dragon. Ram only when their
        // head is at least as big as us and worth it (no reckless kamikaze — that lost
        // to v13); the pounce below handles finishing already-trapped heads.
        {
            auto const* h = ct.get_tile(here);
            Direction ramDir = Direction::NORTH; int bestSz = -1;
            for (auto d : Direction::get_direction_list()) {
                if (!h || h->get_edge(d).get_edge_type() != EdgeType::EMPTY) continue;
                auto const* t = ct.get_tile(here.add_dir(d));
                if (!t) continue;
                auto const* dr = t->get_dragon();
                if (dr && dr->is_head() && !(dr->get_team() == me)) {
                    int sz = t_enemySize[dr->get_id()];
                    if (sz > bestSz) { bestSz = sz; ramDir = d; }
                }
            }
            if (bestSz >= (int)len && bestSz >= 4) { ct.make_move(ramDir); end_turn(); continue; }
        }

        // -------- 3) plan directions: route to pearls / explore (no blind chase) --------
        std::unordered_set<long> pearlGoals;
        for (auto const& p : t_visPearls) pearlGoals.insert(enc(p));
        for (auto const& [e, cr] : g_eta) pearlGoals.insert(e);
        std::optional<Direction> planDir;
        if (!pearlGoals.empty())
            planDir = bfs_first(here, [&](Position q){ return pearlGoals.count(enc(q)) > 0; }, 24, 320);
        if (!planDir)
            planDir = bfs_first(here, [&](Position q){ return g_seen.count(enc(q)) == 0; }, 24, 320);

        auto vis_pearl_dist = [&](Position s) -> int {
            std::unordered_set<long> seen{enc(s)}; std::queue<std::pair<Position,int>> q; q.push({s,0});
            while (!q.empty()) { auto [p,dist]=q.front(); q.pop();
                auto const* t = CT->get_tile(p); if (t && t->has_pearl()) return dist;
                for (auto d : Direction::get_direction_list())
                    if (can_traverse(p,d)) { Position np=p.add_dir(d); if (seen.insert(enc(np)).second) q.push({np,dist+1}); } }
            return 999;
        };
        bool pearlInView = !t_visPearls.empty();

        // -------- 4) score safe moves --------
        int const flr = std::min(len + 1, 8);
        Direction best = Direction::NORTH; long bestScore = LONG_MIN; bool found = false;
        for (auto d : Direction::get_direction_list()) {
            if (!safe_step(here, d)) continue;
            Position dest = here.add_dir(d);
            auto const* dt = ct.get_tile(dest);
            int space = flood_space(dest);
            int opens = onward_safe(dest);
            int crowd = 0; for (auto const& f : t_friends) if (cheb(dest, f) <= 2) ++crowd;
            bool eats = dt && dt->has_pearl();

            long score = 0;
            // PRIMARY objective: dominate territory. Maximise the free area we control
            // after moving (Voronoi over the remembered map) -> expand our region and
            // squeeze the enemy's, walling them into a shrinking pocket.
            int territory = controlled_space(dest, 160);
            score += (long)territory * 12;
            // cornering: pushing an enemy head toward 0 escapes IS the kill condition here
            for (auto const& eh : t_enemyHeads) {
                if (cheb(dest, eh) > 1) continue;
                int esc = enemy_escapes(eh);                   // 0..4
                if (esc <= 2) score += (long)(3 - esc) * (len <= 6 ? 90 : 40);  // finish trapped heads
                else          score -= 45;                     // mobile head: head-to-head risk (mild)
            }
            // secondary: grow (length = bigger walls + tiebreak). Eat when convenient.
            if (pearlInView) { score -= (long)vis_pearl_dist(dest) * 8; if (eats) score += 250; }
            if (planDir && d == *planDir) score += 30;

            // survival floor (always)
            score += -(long)crowd * 10;
            if (opens <= 1 && !eats) score -= 80;
            if (space < flr) score -= (long)(flr - space) * 300;

            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found) { int bs = -1; for (auto d : Direction::get_direction_list())
            if (safe_step(here, d)) { int s = flood_space(here.add_dir(d)); if (s > bs) { bs = s; best = d; found = true; } } }

        ct.make_move(best);
        end_turn();
    }
}
