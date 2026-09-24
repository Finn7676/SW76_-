#include "helper.hpp"

#include <algorithm>
#include <deque>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace unswbc;

namespace {
struct PosHash { std::size_t operator()(Position const& p) const { return PositionHash{}(p); } };
struct EdgeKey { Position p; char d; bool operator==(EdgeKey const&) const = default; };
struct EdgeHash { std::size_t operator()(EdgeKey const& k) const { return PositionHash{}(k.p) ^ (static_cast<unsigned>(k.d) << 1); } };
struct EdgeInfo { EdgeType kind = EdgeType::EMPTY; int portal = -1; };
struct PortalEdge { Position p; Direction d; };

struct Memory {
    int w = 0, h = 0, last_round = 0, initial_length = 0;
    int big_split_count = 0;
    std::unordered_set<Position, PosHash> seen, pearls;
    std::unordered_map<Position, int, PosHash> eta;
    std::unordered_map<EdgeKey, EdgeInfo, EdgeHash> edges;
    std::unordered_map<int, std::vector<PortalEdge>> portals;
    std::vector<Position> body;
    Position step(Position p, Direction d) const { return p.add_dir(d); }
    int delta(int a, int b, int n) const { int d = (b - a) % n; if (d < 0) d += n; return d > n - d ? d - n : d; }
    int dist(Position a, Position b) const { return std::abs(delta(a.x, b.x, w)) + std::abs(delta(a.y, b.y, h)); }
    EdgeKey canonical(Position p, Direction d) const { return (d == Direction::EAST || d == Direction::SOUTH) ? EdgeKey{p, d.value} : EdgeKey{step(p, d), d.get_opposite().value}; }
    EdgeInfo edge_at(Controller& ct, Position p, Direction d) const {
        if (auto const* t = ct.get_tile(p)) { auto const& e = t->get_edge(d); return {e.get_edge_type(), e.get_portal_id()}; }
        auto it = edges.find({p, d.value}); return it == edges.end() ? EdgeInfo{EdgeType::PORTAL, -2} : it->second;
    }
    std::optional<Position> portal_dest(Position p, Direction d, int id) const {
        EdgeKey here = canonical(p, d); auto it = portals.find(id); if (it == portals.end()) return std::nullopt;
        for (auto const& other : it->second) { if (canonical(other.p, other.d) == here) continue; if (other.d == d) return step(other.p, other.d); if (other.d == d.get_opposite()) return other.p; }
        return std::nullopt;
    }
    std::pair<std::string, std::optional<Position>> transit(Controller& ct, Position p, Direction d) const {
        EdgeInfo e = edge_at(ct, p, d); if (e.kind == EdgeType::KELP) return {"kelp", std::nullopt};
        if (e.kind == EdgeType::PORTAL) { if (e.portal < 0) return {"unknown_portal", std::nullopt}; auto dest = portal_dest(p, d, e.portal); return dest ? std::pair{"portal", dest} : std::pair{"unknown_portal", std::optional<Position>{}}; }
        return {"empty", step(p, d)};
    }
};
Memory mem;

void observe(Controller& ct, Game& game, std::unordered_set<Position, PosHash>& occupied, std::vector<Position>& own_parts) {
    if (!mem.w) { auto [w, h] = game.get_map_size(); mem.w = w; mem.h = h; }
    if (!mem.initial_length) mem.initial_length = ct.get_length();
    int round = game.get_round_num(); int delta = mem.last_round ? round - mem.last_round : 0; mem.last_round = round;
    if (delta > 0) for (auto it = mem.eta.begin(); it != mem.eta.end();) { if (it->second >= 0) it->second -= delta; if (it->second < 0) it = mem.eta.erase(it); else ++it; }
    for (auto const& tile : ct.get_tiles()) {
        Position p = tile.get_position(); mem.seen.insert(p); if (tile.has_pearl()) mem.pearls.insert(p); else mem.pearls.erase(p); mem.eta[p] = tile.get_pearl_time();
        for (Direction d : Direction::get_direction_list()) { auto const& edge = tile.get_edge(d); EdgeInfo info{edge.get_edge_type(), edge.get_portal_id()}; mem.edges[{p, d.value}] = info; if (info.kind == EdgeType::PORTAL && info.portal >= 0) { auto& list = mem.portals[info.portal]; if (std::none_of(list.begin(), list.end(), [&](PortalEdge const& x) { return x.p == p && x.d == d; })) list.push_back({p, d}); } }
        auto const* part = tile.get_dragon(); if (!part) continue; occupied.insert(p); if (part->get_id() == ct.get_id()) own_parts.push_back(p);
    }
    Position head = ct.get_position(); std::vector<Position> chain{head}; Position cursor = head; Direction facing = ct.get_dir(); std::unordered_set<Position, PosHash> own(own_parts.begin(), own_parts.end());
    for (int i = 1; i < ct.get_length(); ++i) { Position next = mem.step(cursor, facing.get_opposite()); if (!own.count(next) && std::find(mem.body.begin(), mem.body.end(), next) == mem.body.end()) break; chain.push_back(next); cursor = next; }
    if (chain.size() < static_cast<std::size_t>(ct.get_length()) && !mem.body.empty() && mem.body.front() == head) for (std::size_t i = chain.size(); i < mem.body.size() && chain.size() < static_cast<std::size_t>(ct.get_length()); ++i) if (std::find(chain.begin(), chain.end(), mem.body[i]) == chain.end()) chain.push_back(mem.body[i]);
    mem.body = std::move(chain);
}

bool simulate(Controller& ct, std::vector<Direction> const& moves, std::unordered_set<Position, PosHash> const& others, std::vector<Position>* resulting = nullptr, int* eats_out = nullptr, bool* unknown_out = nullptr) {
    std::vector<Position> body = mem.body; auto pearls = mem.pearls; int eats = 0; bool unknown = false; std::unordered_set<Position, PosHash> own(body.begin(), body.end());
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (body.empty()) return false; auto [kind, dest_opt] = mem.transit(ct, body.front(), moves[i]); if (!dest_opt || kind == "kelp") return false; if (kind == "unknown_portal") { unknown = true; if (i > 0) return false; }
        Position dest = *dest_opt; if (others.count(dest) || own.count(dest)) return false; if (i > 0 && body.size() <= 2) return false; bool ate = pearls.count(dest); body.insert(body.begin(), dest); own.insert(dest); if (!ate) { own.erase(body.back()); body.pop_back(); } if (i > 0) { if (body.size() <= 2) return false; own.erase(body.back()); body.pop_back(); } if (ate) { pearls.erase(dest); ++eats; }
    }
    if (resulting) *resulting = body; if (eats_out) *eats_out = eats; if (unknown_out) *unknown_out = unknown; return true;
}

std::optional<std::vector<Direction>> bfs(Controller& ct, std::unordered_set<Position, PosHash> const& others, std::unordered_set<Position, PosHash> const& goals, bool allow_unseen) {
    if (mem.body.empty()) return std::nullopt; Position start = mem.body.front(); if (goals.count(start)) return std::vector<Direction>{}; std::deque<Position> q{{start}}; std::unordered_map<Position, std::pair<Position, Direction>, PosHash> prev; std::unordered_map<Position, int, PosHash> dist{{start, 0}}; std::optional<Position> found;
    while (!q.empty() && dist.size() < 220) { Position p = q.front(); q.pop_front(); if (dist[p] >= 12) continue; for (Direction d : Direction::get_direction_list()) { auto [kind, dest_opt] = mem.transit(ct, p, d); if (!dest_opt || kind == "unknown_portal" || kind == "kelp") continue; Position dest = *dest_opt; if (dist.count(dest) || others.count(dest) || std::find(mem.body.begin(), mem.body.end(), dest) != mem.body.end()) continue; if (!allow_unseen && !mem.seen.count(dest) && kind != "portal") continue; dist.emplace(dest, dist[p] + 1); prev.emplace(dest, std::make_pair(p, d)); if (goals.count(dest)) { found = dest; q.clear(); break; } if (mem.seen.count(dest) || kind == "portal") q.push_back(dest); } if (found) break; }
    if (!found) return std::nullopt; std::vector<Direction> path; Position cur = *found; while (!(cur == start)) { auto [parent, d] = prev.at(cur); path.push_back(d); cur = parent; } std::reverse(path.begin(), path.end()); return path;
}

// A minimum-size dragon may trade itself for a visibly longer enemy dragon.
// Only the target head is removed from the obstacle set for this intentional
// final step; all other enemy segments remain forbidden.
bool try_short_dragon_intercept(Controller& ct, std::unordered_set<Position, PosHash> const& occupied) {
    if (ct.get_length() > 2 || ct.get_unit_count() <= 1) return false;
    struct EnemyHead { Position position; int id; int visible_length; Direction direction; };
    std::unordered_map<int, int> visible_lengths;
    std::vector<EnemyHead> heads;
    for (auto const& tile : ct.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (!part || part->get_team() == ct.get_team()) continue;
        ++visible_lengths[part->get_id()];
        if (part->is_head()) heads.push_back({tile.get_position(), part->get_id(), 0, part->get_dir()});
    }
    std::sort(heads.begin(), heads.end(), [&](EnemyHead const& a, EnemyHead const& b) {
        return visible_lengths[a.id] > visible_lengths[b.id];
    });
    for (auto enemy : heads) {
        enemy.visible_length = visible_lengths[enemy.id];
        // Vision may show only part of a dragon, so this is a conservative
        // lower-bound test: the visible part must already be longer than us.
        if (enemy.visible_length <= ct.get_length()) continue;
        std::unordered_set<Position, PosHash> attack_obstacles = occupied;
        attack_obstacles.erase(enemy.position);
        auto path = bfs(ct, attack_obstacles, {enemy.position}, true);
        // A multi-step chase is too easy to invalidate because the enemy also
        // moves. Only a currently adjacent head is a deterministic exchange.
        if (!path || path->size() != 1) continue;
        Direction move = path->front();
        int eats = 0; bool unknown = false; std::vector<Position> next;
        if (!simulate(ct, {move}, attack_obstacles, &next, &eats, &unknown)) continue;
        ct.set_indicator_string("intercept " + std::to_string(enemy.visible_length));
        ct.make_move(move);
        mem.body = std::move(next);
        return true;
    }
    return false;
}

std::optional<Position> nearest(Position from, std::unordered_set<Position, PosHash> const& goals) {
    if (goals.empty()) return std::nullopt; return *std::min_element(goals.begin(), goals.end(), [&](Position a, Position b) { return std::pair{mem.dist(from, a), a.x * mem.h + a.y} < std::pair{mem.dist(from, b), b.x * mem.h + b.y}; });
}
std::pair<std::unordered_set<Position, PosHash>, std::optional<Position>> choose_target(Controller& ct, std::string& reason) {
    std::unordered_set<Position, PosHash> visible; for (auto const& t : ct.get_tiles()) if (t.has_pearl()) visible.insert(t.get_position()); Position head = ct.get_position();
    if (!visible.empty()) { reason = "pearl"; return {visible, nearest(head, visible)}; } if (!mem.pearls.empty()) { reason = "memory-pearl"; return {mem.pearls, nearest(head, mem.pearls)}; }
    std::unordered_set<Position, PosHash> soon; for (auto const& [p, eta] : mem.eta) if (eta >= 0 && eta <= 8) soon.insert(p); if (!soon.empty()) { reason = "soon"; return {soon, nearest(head, soon)}; }
    std::unordered_set<Position, PosHash> frontier; for (Position p : mem.seen) for (Direction d : Direction::get_direction_list()) { auto [kind, dest] = mem.transit(ct, p, d); if (dest && kind != "kelp" && kind != "unknown_portal" && !mem.seen.count(*dest)) frontier.insert(*dest); } if (!frontier.empty()) { reason = "explore"; return {frontier, nearest(head, frontier)}; }
    reason = "wander"; return {{}, std::nullopt};
}

bool dead_end(Controller& ct, Position start, std::unordered_set<Position, PosHash> const& blocked) {
    if (!ct.get_tile(start)) return false; std::unordered_set<Position, PosHash> tiles{start}; std::deque<Position> q{{start}}; bool escapes = false;
    while (!q.empty()) { Position p = q.front(); q.pop_front(); for (Direction d : Direction::get_direction_list()) { auto [kind, dest] = mem.transit(ct, p, d); if (!dest || kind == "kelp" || kind == "portal" || kind == "unknown_portal" || blocked.count(*dest)) continue; if (!ct.get_tile(*dest)) { escapes = true; continue; } if (tiles.insert(*dest).second) q.push_back(*dest); } }
    if (escapes) return false; int max_deg = 0; for (Position p : tiles) { int deg = 0; for (Direction d : Direction::get_direction_list()) { auto [_, dest] = mem.transit(ct, p, d); if (dest && tiles.count(*dest)) ++deg; } max_deg = std::max(max_deg, deg); } return max_deg <= 2 || static_cast<int>(tiles.size()) < ct.get_length();
}

struct Candidate { Direction d; Position dest; int score; bool dead; };
int enemy_head_risk(Controller& ct, Position dest) {
    int penalty = 0;
    for (auto const& tile : ct.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (!part || part->get_id() == ct.get_id() || !part->is_head()) continue;
        Position enemy = tile.get_position();
        int distance = mem.dist(dest, enemy);
        if (distance == 1) penalty += 70;
        auto [kind, next] = mem.transit(ct, enemy, part->get_dir());
        if (next && kind != "kelp" && *next == dest) penalty += 180;
        if (distance <= 2 && part->get_dir() == ct.get_dir().get_opposite()) penalty += 45;
    }
    return penalty;
}
std::vector<Candidate> candidates(Controller& ct, std::unordered_set<Position, PosHash> const& others, std::optional<Position> target, std::optional<std::vector<Direction>> const& path) {
    std::vector<Candidate> out; Position head = ct.get_position(); if (!ct.get_tile(head)) return out; std::unordered_set<Position, PosHash> blocked = others; for (Position p : mem.body) blocked.insert(p);
    for (Direction d : Direction::get_direction_list()) { auto [kind, dest_opt] = mem.transit(ct, head, d); if (!dest_opt || kind == "kelp" || kind == "portal" || kind == "unknown_portal") continue; Position dest = *dest_opt; Tile const* dt = ct.get_tile(dest); if (dt && dt->get_dragon()) continue; if (std::find(mem.body.begin(), mem.body.end(), dest) != mem.body.end()) continue;
        int score = 0; for (Direction around : Direction::get_direction_list()) { auto [k, q] = mem.transit(ct, dest, around); if (q && k != "kelp" && k != "unknown_portal" && !blocked.count(*q)) score += 4; } if (mem.pearls.count(dest)) score += 1400; auto it = mem.eta.find(dest); if (it != mem.eta.end() && it->second >= 0 && it->second <= 6) score += 70 - it->second * 5; if (!mem.seen.count(dest)) score += 30; if (d == ct.get_dir().get_opposite()) score -= 25; if (target) score -= mem.dist(dest, *target) * 8; if (path && !path->empty() && d == path->front()) score += 55; if (mem.w * mem.h > 200) score -= enemy_head_risk(ct, dest); int eats = 0; bool unknown = false; if (!simulate(ct, {d}, others, nullptr, &eats, &unknown)) score -= 80; if (eats) score += 300; if (unknown) score -= 80; out.push_back({d, dest, score, dead_end(ct, dest, blocked)});
    }
    std::sort(out.begin(), out.end(), [](Candidate const& a, Candidate const& b) { return a.score > b.score; }); return out;
}

bool want_split(Controller& ct, std::unordered_set<Position, PosHash> const& others, std::optional<std::vector<Direction>> const& path) {
    int area = mem.w * mem.h, cap = area <= 200 ? 4 : area <= 900 ? 5 : 8, minimum = area <= 200 ? 6 : 8; if (ct.get_length() < minimum || ct.get_unit_count() >= cap || !ct.can_split(2) || (path && path->size() == 1)) return false; int safe = 0; for (Direction d : Direction::get_direction_list()) if (simulate(ct, {d}, others)) ++safe; return safe >= 2;
}
void fallback(Controller& ct) { auto c = candidates(ct, {}, std::nullopt, std::nullopt); ct.make_move(c.empty() ? Direction::NORTH : c.front().d); }
void execute(Controller& ct, Game& game) {
    std::unordered_set<Position, PosHash> occupied; std::vector<Position> own; observe(ct, game, occupied, own);
    // Keep a scouting floor before switching to pure survival. On default.map,
    // stopping at round 180 left tyhh badly outnumbered by xxqz_cpp.
    bool expansion_phase = game.get_round_num() < 320 || ct.get_unit_count() < 20;
    bool late_game = !expansion_phase;
    std::string reason; auto [goals, target] = choose_target(ct, reason); auto path = goals.empty() ? std::optional<std::vector<Direction>>{} : bfs(ct, occupied, goals, true); auto all = candidates(ct, occupied, target, path); std::vector<Candidate> open, dead; for (auto const& c : all) (c.dead ? dead : open).push_back(c);
    if (try_short_dragon_intercept(ct, occupied)) return;
    bool big_empty_mode = mem.w == 64 && mem.h == 64 && mem.initial_length == 6;
    bool big_empty_child = mem.w == 64 && mem.h == 64 && mem.initial_length <= 2 && ct.get_id() >= 6 && ct.get_id() < 24;
    if (big_empty_mode || big_empty_child) {
        // Delay splitting until a starting length-6 dragon has eaten twice;
        // this preserves a useful core while still matching the large map's
        // need for additional heads.
        bool designated_scout = big_empty_mode;
        bool child_of_scout = big_empty_child;
        if (designated_scout && mem.big_split_count < 1 && ct.get_length() >= 8 && ct.can_split(2) && !open.empty()) {
            ct.do_split(2);
            ++mem.big_split_count;
            if (mem.body.size() > 2) mem.body.resize(mem.body.size() - 2);
            return;
        }
        if (!designated_scout || child_of_scout || mem.big_split_count >= 1) {
            // Skip the generic length-4 split rule in this map-specific role.
            auto& choices = open.empty() ? dead : open;
            if (choices.empty()) { fallback(ct); return; }
            Candidate c = choices.front(); ct.set_indicator_string("big-core " + std::string(1, c.d.value));
            ct.make_move(c.d); std::vector<Position> next; int eats = 0; bool unknown = false;
            if (simulate(ct, {c.d}, occupied, &next, &eats, &unknown)) mem.body = std::move(next);
            mem.pearls.erase(c.dest);
            return;
        }
    }
    // Tyhh deliberately turns every newly legal long dragon into two scouts first.
    // This is the experiment that differs from xxqz_cpp's conservative population cap.
    // Only short and medium dragons are scouts. Long dragons are the score reserve.
    bool core_dragon = ct.get_length() >= 6;
    if (!core_dragon && expansion_phase && ct.get_length() == 4 && ct.can_split(2)) { ct.do_split(2); if (mem.body.size() > 2) mem.body.resize(mem.body.size() - 2); return; }
    if (!core_dragon && expansion_phase && ct.get_length() <= 4 && want_split(ct, occupied, path) && !open.empty()) { int child = ct.can_split(2) ? 2 : 0; if (child) { ct.do_split(child); if (mem.body.size() > static_cast<std::size_t>(child)) mem.body.resize(mem.body.size() - child); return; } }
    if (!core_dragon && open.empty() && ct.can_split(2) && ct.get_length() >= 4) {
        // In the late phase, split only when every available move is classified as a dead end.
        int child = ct.get_length() - 2; if (child < 2 || !ct.can_split(child)) child = 2;
        if (ct.can_split(child)) { ct.do_split(child); if (mem.body.size() > static_cast<std::size_t>(child)) mem.body.resize(mem.body.size() - child); return; }
    }
    if (late_game) reason = "survive";
    auto& choices = open.empty() ? dead : open; if (choices.empty()) { fallback(ct); return; }
    Candidate c = choices.front(); ct.set_indicator_string(reason + " " + std::string(1, c.d.value)); if (mem.pearls.count(c.dest)) ct.draw_indicator_dot(c.dest, 255, 210, 40); ct.make_move(c.d); std::vector<Position> next; int eats = 0; bool unknown = false; if (simulate(ct, {c.d}, occupied, &next, &eats, &unknown)) mem.body = std::move(next); mem.pearls.erase(c.dest);
}
} // namespace

int main() { auto [ct, game] = unswbc::init(); while (unswbc::update(ct, game)) { try { execute(ct, game); } catch (...) { fallback(ct); } unswbc::end_turn(); } }
