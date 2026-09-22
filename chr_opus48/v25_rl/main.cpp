// v25 "rl": a SMALL reinforcement-learning bot. The move policy is a linear
// evaluator: for each safe move we build a 10-feature vector phi and score it as
// THETA . phi, then take the argmax. THETA (10 floats) is the entire "model" -- so
// inference is a 10-element dot product per move (no timeout risk whatsoever).
//
// THETA is trained OFFLINE by evolutionary policy search (cross-entropy method,
// see train_rl.py): sample weight vectors, play deterministic gauntlet games, keep
// the ones with the best win-rate (reward), repeat. THETA is initialised to the
// hand-tuned v22 "apex2" coefficients, so an untrained v25 == v22 exactly and
// training can only improve it.
//
// During training the weights are read from /tmp/bc_theta.txt (rewritten per
// candidate, so we never recompile to change the policy). In the arena that file
// is absent, so the BAKED-IN trained THETA below is used -- fully self-contained.
#include "helper.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <deque>
#include <fstream>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace unswbc;

// ------------------------------------------------------------- the "model" ---
// feature order:
//  0 vis_pearl_dist (when a pearl is in view)     1 eats-a-pearl (0/1)
//  2 follows wall-aware BFS plan (0/1)             3 spawn-camp ETA term (40-4*eff)
//  4 steps onto an unseen tile (0/1)               5 reachable space (flood)
//  6 friendly crowding count                       7 near-dead-end (opens<=1 & !eat)
//  8 adjacent enemy heads (danger, big maps)       9 trap deficit (flr-space)+
static double THETA[10] = { -25, 400, 140, 1, 18, 4, -12, -60, -70, -300 };
static void load_theta() {
    std::ifstream f("/tmp/bc_theta.txt");
    if (!f) return;
    for (int i = 0; i < 10; ++i) { double v; if (f >> v) THETA[i] = v; else break; }
}

// ------------------------------------------------------------------ globals ---
static Controller* CT = nullptr;
static int W = 0, H = 0;
static bool SMALL = false;
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

// ------------------------------------------------------------------ main ------
int main() {
    load_theta();
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

        // structural decisions (not learned): expand swarm, then ram up.
        if (pop < TARGET && len >= 4 && ct.can_split(2)) { ct.do_split(2); end_turn(); continue; }
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

        // ---- linear policy: score each safe move as THETA . phi, pick argmax ----
        int const flr = std::min(len + 1, 8);
        Direction best = Direction::NORTH; double bestScore = -1e18; bool found = false;
        for (auto d : Direction::get_direction_list()) {
            if (!safe_step(here, d)) continue;
            Position dest = here.add_dir(d);
            auto const* dt = ct.get_tile(dest);
            int space = flood_space(dest);
            int opens = onward_safe(dest);
            int crowd = 0; for (auto const& f : t_friends) if (cheb(dest, f) <= 2) ++crowd;
            bool eats = dt && dt->has_pearl();

            double phi[10] = {0,0,0,0,0,0,0,0,0,0};
            if (pearlInView) { phi[0] = vis_pearl_dist(dest); phi[1] = eats ? 1 : 0; }
            phi[2] = (planDir && d == *planDir) ? 1 : 0;
            auto it = g_eta.find(enc(dest));
            if (it != g_eta.end()) { int eff = it->second.first - (round - it->second.second); if (eff >= 0 && eff <= 6) phi[3] = 40 - eff * 4; }
            phi[4] = (g_seen.count(enc(dest)) == 0) ? 1 : 0;
            phi[5] = space;
            phi[6] = crowd;
            phi[7] = (opens <= 1 && !eats) ? 1 : 0;
            phi[8] = SMALL ? 0 : [&]{ int dg = 0; for (auto const& eh : t_enemyHeads) if (cheb(dest, eh) <= 1) ++dg; return dg; }();
            phi[9] = (space < flr) ? (flr - space) : 0;

            double score = 0; for (int i = 0; i < 10; ++i) score += THETA[i] * phi[i];
            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found) { int bs = -1; for (auto d : Direction::get_direction_list())
            if (safe_step(here, d)) { int s = flood_space(here.add_dir(d)); if (s > bs) { bs = s; best = d; found = true; } } }

        ct.make_move(best);
        end_turn();
    }
}
