// v23 "reaper": a DISTINCT archetype from the harvest-champion (v22) — a predator
// that wins by ELIMINATION. On tight/medium maps (where killing the whole enemy team
// ends the game) it hunts and CORNERS enemy heads instead of chasing pearls:
//   * chase  — BFS toward the nearest enemy head (beyond the 7x7, over remembered map)
//   * corner — reward moving adjacent to an enemy head that has few escape squares
//              (squeeze its options toward 0 -> forced death), pressed hardest by
//              expendable small dragons
//   * kamikaze ram — a small dragon (len<=6) with population to spare head-to-heads
//              ANY adjacent enemy head: head-to-head kills both, and killing a head
//              removes that entire enemy dragon (a huge elimination trade)
//   * no enemy-head-danger penalty in hunt mode (we WANT contact)
// On big maps (elimination is impractical) it falls back to the v22 harvest brain so it
// doesn't lose on the length tiebreak. Keeps swarm split + survival floor throughout.
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

        // -------- 2) ram an adjacent enemy head --------
        // Killing an enemy HEAD removes that whole enemy dragon. In hunt mode an
        // expendable small dragon with population to spare head-to-heads ANY adjacent
        // head (kamikaze). Otherwise only trade UP (their head bigger than us).
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
            bool kamikaze = HUNT && len <= 6 && pop >= 3 && bestSz >= 2;
            bool tradeUp  = bestSz >= (int)len && bestSz >= 4;
            if (bestSz >= 0 && (kamikaze || tradeUp)) { ct.make_move(ramDir); end_turn(); continue; }
        }

        // -------- 3) plan directions: chase nearest enemy head (hunt) / route to pearls --------
        std::optional<Direction> chaseDir;
        if (HUNT && !t_enemyHeads.empty()) {
            std::unordered_set<long> heads;
            for (auto const& eh : t_enemyHeads) heads.insert(enc(eh));
            chaseDir = bfs_first(here, [&](Position q){ return heads.count(enc(q)) > 0; }, 20, 300);
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
            // opportunistic harvest (kept so we don't lose on length)
            if (pearlInView) { score -= (long)vis_pearl_dist(dest) * 25; if (eats) score += 400; }
            if (planDir && d == *planDir) score += HUNT ? 60 : 140;
            auto it = g_eta.find(enc(dest));
            if (it != g_eta.end()) { int eff = it->second.first - (round - it->second.second); if (eff >= 0 && eff <= 6) score += 40 - eff * 4; }
            if (g_seen.count(enc(dest)) == 0) score += 18;

            // HUNT: chase + corner enemy heads
            if (HUNT && !t_enemyHeads.empty()) {
                if (chaseDir && d == *chaseDir) score += 130;
                for (auto const& eh : t_enemyHeads) {
                    if (cheb(dest, eh) <= 1) {
                        int esc = enemy_escapes(eh);           // 0..4; fewer = more trapped
                        int press = (4 - esc) * (len <= 6 ? 70 : 20);
                        score += std::max(press, 0);
                    }
                }
            } else if (!SMALL) {
                // harvest mode: avoid enemy-head-adjacent death traps
                int danger = 0; for (auto const& eh : t_enemyHeads) if (cheb(dest, eh) <= 1) ++danger;
                score -= (long)danger * 70;
            }

            // survival floor (always)
            score += (long)space * 4 - (long)crowd * 12;
            if (opens <= 1 && !eats) score -= 60;
            if (space < flr) score -= (long)(flr - space) * 300;

            if (!found || score > bestScore) { bestScore = score; best = d; found = true; }
        }
        if (!found) { int bs = -1; for (auto d : Direction::get_direction_list())
            if (safe_step(here, d)) { int s = flood_space(here.add_dir(d)); if (s > bs) { bs = s; best = d; found = true; } } }

        ct.make_move(best);
        end_turn();
    }
}
