// v21 "apex": a full, integrated bot. Built on the proven v13 core (aggressive
// swarm + harvest-objective scoring) and layered with every lever that helped
// or is worth exploiting, engineered as subsystems:
//   * Memory:   persistent terrain (kelp/portal edges), pearl ETAs (aged), seen set
//   * Navigate: wall-aware BFS over the remembered map to the nearest pearl; else
//               explore toward the nearest unseen frontier
//   * Harvest:  spawn-camp ETA bonus; SPRINT through a straight run of pearls
//   * Swarm:    map-adaptive population target, split to expand, maintain
//   * Aggress:  expendable small dragons ram the biggest adjacent enemy dragon
//   * Survive:  flood-fill space, onward-safe (anti-dead-end), trap guard,
//               map-adaptive enemy-head avoidance
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
static bool SMALL = false;
static int TARGET = 8;

static std::unordered_set<long> g_seen;          // encoded positions ever observed
static std::unordered_set<long> g_blocked;       // encoded directed edges known non-empty
static std::unordered_map<long, std::pair<int,int>> g_eta;  // enc(pos) -> (countdown, round seen)

// per-turn scratch (refreshed each turn)
static std::vector<Position> t_friends, t_enemyHeads, t_visPearls;
static std::unordered_map<int,int> t_enemySize;
static std::unordered_set<long> t_occupied;      // dragon tiles currently in view

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
// Immediate legality this turn: empty edge and no dragon on the destination.
static bool safe_step(Position here, Direction dir) {
    auto const* h = CT->get_tile(here);
    if (!h || h->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* d = CT->get_tile(here.add_dir(dir));
    return d && !d->get_dragon();
}
static int onward_safe(Position p) {
    int n = 0; for (auto d : Direction::get_direction_list()) if (safe_step(p, d)) ++n; return n;
}
static bool can_traverse(Position p, Direction dir) {   // for in-vision flood fill
    auto const* t = CT->get_tile(p);
    if (!t || t->get_edge(dir).get_edge_type() != EdgeType::EMPTY) return false;
    auto const* nb = CT->get_tile(p.add_dir(dir));
    return nb && !nb->get_dragon();
}
// Reachable open tiles within the vision window (proxy for not-trapped).
static int flood_space(Position start, int cap = 90) {
    std::unordered_set<long> vis{enc(start)}; std::queue<Position> q; q.push(start); int n = 0;
    while (!q.empty() && n < cap) { Position p = q.front(); q.pop(); ++n;
        for (auto d : Direction::get_direction_list())
            if (can_traverse(p, d)) { Position np = p.add_dir(d); if (vis.insert(enc(np)).second) q.push(np); } }
    return n;
}

// ------------------------------------------------------------------ pathing ---
// Traverse the REMEMBERED map: known non-empty edges block; unknown edges are
// assumed open (optimistic exploration). Dragons currently in view block.
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

// ------------------------------------------------------------------ sprint ----
// Look for a straight run in one direction that eats >=2 pearls in a single turn
// and is affordable (n steps cost n-1 segments) and safe at every step.
static std::optional<std::vector<Direction>> sprint_harvest(Position here, int len) {
    std::optional<std::vector<Direction>> best; int bestNet = 0;
    for (auto d : Direction::get_direction_list()) {
        Position p = here; int pearls = 0; std::vector<Direction> run;
        for (int step = 1; step <= 4; ++step) {
            auto const* t = CT->get_tile(p);
            if (!t || t->get_edge(d).get_edge_type() != EdgeType::EMPTY) break;
            Position np = p.add_dir(d);
            auto const* nt = CT->get_tile(np);
            if (!nt || nt->get_dragon()) break;               // must see it and be clear
            run.push_back(d);
            if (nt->has_pearl()) ++pearls;
            p = np;
            int cost = step - 1;                               // segments spent
            int net = pearls - cost;                           // net length change
            if (step >= 2 && pearls >= 2 && len - cost >= 3 && net > bestNet) {
                bestNet = net; best = run;
            }
        }
    }
    return best;
}

// ------------------------------------------------------------------ main ------
int main() {
    auto [ct, game] = init();
    CT = &ct;
    auto [w, h] = game.get_map_size(); W = w; H = h;
    SMALL = (W * H) <= 220;
    TARGET = std::clamp((W * H) / 10, 8, 62);

    while (update(ct, game)) {
        int round = game.get_round_num();
        Position here = ct.get_position();
        int len = ct.get_length();
        int pop = ct.get_unit_count();
        Team me = ct.get_team();

        // -------- observe: refresh memory + per-turn scratch --------
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

        // -------- 1) expand the swarm --------
        if (pop < TARGET && len >= 4 && ct.can_split(2)) { ct.do_split(2); end_turn(); continue; }

        // -------- 2) aggression: expendable dragon rams the biggest adjacent enemy --------
        if (len <= 6 && pop > 1) {
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

        // -------- 3) sprint through a run of pearls --------
        if (auto run = sprint_harvest(here, len)) { ct.make_moves(*run); end_turn(); continue; }

        // -------- 4) plan a direction: BFS to nearest known pearl, else explore --------
        std::unordered_set<long> pearlGoals;
        for (auto const& p : t_visPearls) pearlGoals.insert(enc(p));
        for (auto const& [e, cr] : g_eta) pearlGoals.insert(e);
        std::optional<Direction> planDir;
        if (!pearlGoals.empty())
            planDir = bfs_first(here, [&](Position q){ return pearlGoals.count(enc(q)) > 0; }, 24, 320);
        if (!planDir)
            planDir = bfs_first(here, [&](Position q){ return g_seen.count(enc(q)) == 0; }, 24, 320);

        // nearest visible pearl distance (in-vision BFS) for fine harvest scoring
        auto vis_pearl_dist = [&](Position s) -> int {
            std::unordered_set<long> seen{enc(s)}; std::queue<std::pair<Position,int>> q; q.push({s,0});
            while (!q.empty()) { auto [p,dist]=q.front(); q.pop();
                auto const* t = CT->get_tile(p); if (t && t->has_pearl()) return dist;
                for (auto d : Direction::get_direction_list())
                    if (can_traverse(p,d)) { Position np=p.add_dir(d); if (seen.insert(enc(np)).second) q.push({np,dist+1}); } }
            return 999;
        };
        bool pearlInView = !t_visPearls.empty();

        // -------- 5) score safe moves (v13 harvest core + subsystem bonuses) --------
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
            // harvest (v13 core)
            if (pearlInView) { score -= (long)vis_pearl_dist(dest) * 25; if (eats) score += 400; }
            if (planDir && d == *planDir) score += 140;          // follow the wall-aware route
            // spawn-camp: arrive as a tile ripens
            auto it = g_eta.find(enc(dest));
            if (it != g_eta.end()) { int eff = it->second.first - (round - it->second.second); if (eff >= 0 && eff <= 6) score += 40 - eff * 4; }
            // exploration nudge toward unseen
            if (g_seen.count(enc(dest)) == 0) score += 18;
            // survival + coverage
            score += (long)space * 4 - (long)crowd * 12;
            if (opens <= 1 && !eats) score -= 60;
            if (!SMALL) { int danger = 0; for (auto const& eh : t_enemyHeads) if (cheb(dest, eh) <= 1) ++danger; score -= (long)danger * 70; }
            if (space < flr) score -= (long)(flr - space) * 300;   // trap guard

            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found) { int bs = -1; for (auto d : Direction::get_direction_list())
            if (safe_step(here, d)) { int s = flood_space(here.add_dir(d)); if (s > bs) { bs = s; best = d; found = true; } } }

        ct.make_move(best);
        end_turn();
    }
}
