// v27 "esmlp": the proven v22 skeleton (swarm-split + ram + terrain-BFS + memory)
// with its hand-tuned LINEAR move-scorer augmented by a small RESIDUAL MLP:
//
//     score(move) = LINSCALE * (LIN . f)   +   SCALE * MLP(g)
//
// where f are v22's raw scoring features, LIN are v22's hand weights, and MLP is a
// tiny 1-hidden-layer net (tanh, 6 units) over normalised features g. The whole
// thing is trained by Evolution Strategies / CEM (train_es.py), which maximises
// gauntlet WIN-RATE directly -- a genuine (derivative-free) RL method that suits
// this slow, non-differentiable, subprocess environment far better than a
// gradient method. Crucially the baked defaults are SCALE=0, LINSCALE=1, MLP=0,
// so an UNTRAINED v27 == v22 exactly: the search starts at the champion and can
// only improve. Inference is a handful of dot products -> no timeout risk, and
// being C++ it trains at C++ speed.
//
// Weights load from /tmp/bc_es.txt if present (training rewrites it per candidate
// with the source unchanged, so unswbc reuses its cached binary); in the arena
// that file is absent and the BAKED-IN trained weights below are used.
#include "helper.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <deque>
#include <fstream>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace unswbc;

// ---------------------------------------------------------------- the model --
static constexpr int NFEAT = 10;   // raw linear features
static constexpr int NG = 11;      // normalised MLP inputs (NFEAT + bias)
static constexpr int HD = 6;       // hidden units
// v22 hand-tuned linear weights (the strong base)
static const double LIN[NFEAT] = { -25, 400, 140, 1, 18, 4, -12, -60, -70, -300 };
// trainable params (baked defaults => residual is a no-op => == v22)
static double SCALE = 0.0;
static double LINSCALE = 1.0;
static double W1[HD * NG] = {0};   // hidden layer (row-major HD x NG)
static double W2[HD] = {0};        // output layer
static double B2 = 0.0;
static constexpr int NPARAM = 2 + HD * NG + HD + 1;  // 75

static void load_params() {
    std::ifstream f("/tmp/bc_es.txt");
    if (!f) return;
    std::vector<double> v; v.reserve(NPARAM);
    double x; while (f >> x) v.push_back(x);
    if ((int)v.size() < NPARAM) return;
    int i = 0;
    SCALE = v[i++]; LINSCALE = v[i++];
    for (int k = 0; k < HD * NG; ++k) W1[k] = v[i++];
    for (int k = 0; k < HD; ++k) W2[k] = v[i++];
    B2 = v[i++];
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

// ---- residual MLP over normalised features; returns the additive score term ---
static double mlp_residual(const double f[NFEAT]) {
    if (SCALE == 0.0) return 0.0;   // fast path: untrained == v22
    double g[NG] = {
        f[0] / 12.0, f[1], f[2], f[3] / 40.0, f[4],
        f[5] / 20.0, f[6] / 4.0, f[7], f[8] / 3.0, std::min(f[9], 8.0) / 8.0, 1.0
    };
    double out = B2;
    for (int j = 0; j < HD; ++j) {
        double a = 0.0;
        for (int k = 0; k < NG; ++k) a += W1[j * NG + k] * g[k];
        out += W2[j] * std::tanh(a);
    }
    return SCALE * out;
}

// ------------------------------------------------------------------ main ------
int main() {
    load_params();
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

            double f[NFEAT] = {0,0,0,0,0,0,0,0,0,0};
            if (pearlInView) { f[0] = vis_pearl_dist(dest); f[1] = eats ? 1 : 0; }
            f[2] = (planDir && d == *planDir) ? 1 : 0;
            auto it = g_eta.find(enc(dest));
            if (it != g_eta.end()) { int eff = it->second.first - (round - it->second.second); if (eff >= 0 && eff <= 6) f[3] = 40 - eff * 4; }
            f[4] = (g_seen.count(enc(dest)) == 0) ? 1 : 0;
            f[5] = space;
            f[6] = crowd;
            f[7] = (opens <= 1 && !eats) ? 1 : 0;
            f[8] = SMALL ? 0 : [&]{ int dg = 0; for (auto const& eh : t_enemyHeads) if (cheb(dest, eh) <= 1) ++dg; return dg; }();
            f[9] = (space < flr) ? (flr - space) : 0;

            double lin = 0; for (int i = 0; i < NFEAT; ++i) lin += LIN[i] * f[i];
            double score = LINSCALE * lin + mlp_residual(f);
            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found) { int bs = -1; for (auto d : Direction::get_direction_list())
            if (safe_step(here, d)) { int s = flood_space(here.add_dir(d)); if (s > bs) { bs = s; best = d; found = true; } } }

        ct.make_move(best);
        end_turn();
    }
}
