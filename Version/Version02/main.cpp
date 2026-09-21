#include "helper.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using unswbc::Controller;
using unswbc::Direction;
using unswbc::EdgeType;
using unswbc::Game;
using unswbc::Position;
using unswbc::PositionHash;
using unswbc::Tile;

namespace {

constexpr int LOCAL_BFS_DEPTH = 12;
constexpr int LOCAL_BFS_NODES = 220;

using PosSet = std::unordered_set<Position, PositionHash>;
using PosMapInt = std::unordered_map<Position, int, PositionHash>;

int dir_index(Direction d) {
    switch (d.value) {
    case Direction::NORTH: return 0;
    case Direction::EAST: return 1;
    case Direction::SOUTH: return 2;
    default: return 3;
    }
}

std::array<Direction, 4> const DIRS = Direction::get_direction_list();

enum class Kind { Unknown, Kelp, Portal, UnknownPortal, Empty };

struct Transit {
    Kind kind = Kind::Unknown;
    std::optional<Position> dest;
};

struct StepOpt {
    Direction direction;
    Position dest;
    Kind kind;
};

struct Memory {
    int w = 0;
    int h = 0;
    PosSet seen;
    std::unordered_map<long long, std::pair<EdgeType, int>> edge;
    std::unordered_map<int, std::vector<std::tuple<int, int, int>>> portal_obs;
    PosSet pearls;
    PosMapInt eta;
    std::vector<Position> body;
    int last_round = 0;
    std::optional<Position> last_head;

    static long long edge_key(int x, int y, int d) {
        return (static_cast<long long>(x) << 24) ^ (static_cast<long long>(y) << 8) ^ d;
    }

    void configure(int width, int height) {
        w = width;
        h = height;
    }

    Position step(int x, int y, Direction direction) const {
        auto const [dx, dy] = direction.get_offset();
        int nx = (x + dx) % w;
        int ny = (y + dy) % h;
        if (nx < 0) nx += w;
        if (ny < 0) ny += h;
        return {nx, ny};
    }

    std::tuple<int, int, int> canon_edge(int x, int y, Direction direction) const {
        if (direction.value == Direction::EAST || direction.value == Direction::SOUTH)
            return {x, y, dir_index(direction)};
        Position nxt = step(x, y, direction);
        return {nxt.x, nxt.y, dir_index(direction.get_opposite())};
    }

    std::optional<Position> portal_dest(int x, int y, Direction direction, int pid) const {
        auto here = canon_edge(x, y, direction);
        auto it = portal_obs.find(pid);
        if (it == portal_obs.end())
            return std::nullopt;
        for (auto const& [ox, oy, od] : it->second) {
            Direction other_dir = DIRS[od];
            auto other = canon_edge(ox, oy, other_dir);
            if (other == here)
                continue;
            if (other_dir == direction)
                return step(ox, oy, other_dir);
            if (other_dir == direction.get_opposite())
                return Position{ox, oy};
        }
        return std::nullopt;
    }

    Transit transit(Controller& ct, int x, int y, Direction direction) const {
        Tile const* live = ct.get_tile(Position{x, y});
        EdgeType kind;
        int pid = -1;
        if (live) {
            auto const& edge = live->get_edge(direction);
            kind = edge.get_edge_type();
            pid = edge.get_portal_id();
        } else {
            auto it = edge.find(edge_key(x, y, dir_index(direction)));
            if (it == edge.end())
                return {Kind::Unknown, step(x, y, direction)};
            kind = it->second.first;
            pid = it->second.second;
        }
        if (kind == EdgeType::KELP)
            return {Kind::Kelp, std::nullopt};
        if (kind == EdgeType::PORTAL) {
            auto dest = portal_dest(x, y, direction, pid);
            if (!dest)
                return {Kind::UnknownPortal, std::nullopt};
            return {Kind::Portal, dest};
        }
        return {Kind::Empty, step(x, y, direction)};
    }

    void tick_unseen_eta(Game& game_state, PosSet const& visible) {
        int round_num = game_state.get_round_num();
        int delta = last_round ? round_num - last_round : 0;
        last_round = round_num;
        if (delta <= 0)
            return;
        std::vector<Position> stale;
        for (auto& [pos, value] : eta) {
            if (visible.count(pos) || value < 0)
                continue;
            int nxt = value - delta;
            if (nxt < 0)
                stale.push_back(pos);
            else
                value = nxt;
        }
        for (auto const& pos : stale)
            eta.erase(pos);
    }

    void sync_body(Controller& ct, std::vector<std::tuple<int, int, Direction, bool>> const& my_parts) {
        Position head = ct.get_position();
        int length = ct.get_length();
        if (!body.empty() && body[0] == head && static_cast<int>(body.size()) == length) {
            last_head = head;
            return;
        }

        std::unordered_map<Position, Direction, PositionHash> facing_at;
        facing_at.emplace(head, ct.get_dir());
        for (auto const& [x, y, direction, is_head] : my_parts) {
            facing_at[Position{x, y}] = direction;
            (void)is_head;
        }

        std::vector<Position> chain{head};
        Position pos = head;
        Direction facing = ct.get_dir();
        for (int i = 0; i < length - 1; ++i) {
            Position nxt = step(pos.x, pos.y, facing.get_opposite());
            chain.push_back(nxt);
            auto fit = facing_at.find(nxt);
            if (fit != facing_at.end()) {
                facing = fit->second;
                pos = nxt;
                continue;
            }
            auto bit = std::find(body.begin(), body.end(), pos);
            if (bit != body.end()) {
                auto nxt_it = std::next(bit);
                if (nxt_it != body.end() && *nxt_it == nxt) {
                    pos = nxt;
                    continue;
                }
            }
            break;
        }

        if (static_cast<int>(chain.size()) < length && !body.empty() && body[0] == head) {
            for (std::size_t i = chain.size(); i < body.size() && static_cast<int>(chain.size()) < length; ++i) {
                if (std::find(chain.begin(), chain.end(), body[i]) != chain.end())
                    continue;
                chain.push_back(body[i]);
            }
        }
        if (static_cast<int>(chain.size()) > length)
            chain.resize(static_cast<std::size_t>(length));
        body = std::move(chain);
        last_head = head;
    }

    void observe(Controller& ct, Game& game_state) {
        PosSet visible;
        int my_id = ct.get_id();
        std::vector<std::tuple<int, int, Direction, bool>> my_parts;

        for (auto const& tile : ct.get_tiles()) {
            Position pos = tile.get_position();
            visible.insert(pos);
            seen.insert(pos);
            if (tile.has_pearl())
                pearls.insert(pos);
            else
                pearls.erase(pos);
            eta[pos] = tile.get_pearl_time();
            auto const* part = tile.get_dragon();
            if (part) {
                if (part->get_id() == my_id)
                    my_parts.emplace_back(pos.x, pos.y, part->get_dir(), part->is_head());
            }
            for (auto const direction : DIRS) {
                auto const& e = tile.get_edge(direction);
                EdgeType kind = e.get_edge_type();
                int pid = e.get_portal_id();
                edge[edge_key(pos.x, pos.y, dir_index(direction))] = {kind, pid};
                if (kind == EdgeType::PORTAL)
                    portal_obs[pid].emplace_back(pos.x, pos.y, dir_index(direction));
            }
        }
        tick_unseen_eta(game_state, visible);
        sync_body(ct, my_parts);
    }

    void apply_moves(Controller& ct, std::vector<Direction> const& moves) {
        auto cur = body;
        auto pearls_now = pearls;
        for (std::size_t i = 0; i < moves.size(); ++i) {
            if (cur.empty())
                break;
            Transit t = transit(ct, cur[0].x, cur[0].y, moves[i]);
            if (!t.dest || t.kind == Kind::Kelp)
                break;
            if (i > 0 && cur.size() <= 2)
                break;
            bool ate = pearls_now.count(*t.dest) != 0;
            cur.insert(cur.begin(), *t.dest);
            if (!ate && cur.size() > 1)
                cur.pop_back();
            if (i > 0 && cur.size() > 2)
                cur.pop_back();
            pearls_now.erase(*t.dest);
        }
        body = std::move(cur);
        pearls = std::move(pearls_now);
        last_head = body.empty() ? std::optional<Position>{} : body[0];
    }
};

Memory mem;

int torus_delta(int a, int b, int n) {
    int d = (b - a) % n;
    if (d < 0)
        d += n;
    if (d > n - d)
        return d - n;
    return d;
}

int torus_dist(Position a, Position b) {
    return std::abs(torus_delta(a.x, b.x, mem.w)) + std::abs(torus_delta(a.y, b.y, mem.h));
}

struct Sim {
    bool ok = false;
    int eats = 0;
    bool unknown_portal = false;
};

Sim simulate(Controller& ct, std::vector<Direction> const& moves, PosSet const& occupied_others) {
    Sim out;
    out.ok = true;
    auto body = mem.body;
    auto pearls = mem.pearls;
    PosSet occupied_own(body.begin(), body.end());
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (body.empty()) {
            out.ok = false;
            return out;
        }
        Transit t = mem.transit(ct, body[0].x, body[0].y, moves[i]);
        if (t.kind == Kind::Kelp || !t.dest) {
            out.ok = false;
            return out;
        }
        if (t.kind == Kind::UnknownPortal || t.kind == Kind::Unknown) {
            out.unknown_portal = true;
            if (i > 0 || t.kind == Kind::UnknownPortal) {
                out.ok = false;
                return out;
            }
        }
        if (i > 0 && body.size() <= 2) {
            out.ok = false;
            return out;
        }
        if (occupied_others.count(*t.dest) || occupied_own.count(*t.dest)) {
            out.ok = false;
            return out;
        }
        bool ate = pearls.count(*t.dest) != 0;
        occupied_own.insert(*t.dest);
        body.insert(body.begin(), *t.dest);
        if (!ate) {
            Position tail = body.back();
            body.pop_back();
            occupied_own.erase(tail);
        }
        if (i > 0) {
            if (body.size() <= 2) {
                out.ok = false;
                return out;
            }
            Position extra = body.back();
            body.pop_back();
            occupied_own.erase(extra);
        }
        if (ate) {
            pearls.erase(*t.dest);
            ++out.eats;
        }
    }
    return out;
}

std::optional<std::vector<Direction>> local_bfs(Controller& ct, PosSet const& occupied_others, PosSet const& goals) {
    if (mem.body.empty())
        return std::nullopt;
    Position start = mem.body[0];
    if (goals.count(start))
        return std::vector<Direction>{};
    std::deque<Position> q{start};
    std::unordered_map<Position, std::pair<Position, Direction>, PositionHash> prev;
    std::unordered_map<Position, int, PositionHash> dist;
    dist[start] = 0;
    std::optional<Position> found;
    while (!q.empty() && static_cast<int>(dist.size()) < LOCAL_BFS_NODES) {
        Position cur = q.front();
        q.pop_front();
        int d0 = dist[cur];
        if (d0 >= LOCAL_BFS_DEPTH)
            continue;
        bool stop = false;
        for (auto const direction : DIRS) {
            Transit t = mem.transit(ct, cur.x, cur.y, direction);
            if (!t.dest || t.kind == Kind::Kelp || t.kind == Kind::UnknownPortal)
                continue;
            if (dist.count(*t.dest))
                continue;
            if (occupied_others.count(*t.dest) || std::find(mem.body.begin(), mem.body.end(), *t.dest) != mem.body.end())
                continue;
            dist[*t.dest] = d0 + 1;
            prev[*t.dest] = {cur, direction};
            if (goals.count(*t.dest)) {
                found = *t.dest;
                stop = true;
                break;
            }
            if (!mem.seen.count(*t.dest))
                continue;
            q.push_back(*t.dest);
        }
        if (stop)
            break;
    }
    if (!found)
        return std::nullopt;
    std::vector<Direction> path;
    Position cur = *found;
    while (!(cur == start)) {
        auto const& p = prev[cur];
        path.push_back(p.second);
        cur = p.first;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::optional<Position> nearest_goal(PosSet const& goals) {
    if (goals.empty() || mem.body.empty())
        return std::nullopt;
    Position head = mem.body[0];
    std::optional<Position> best;
    int best_d = 1e9;
    for (auto const& p : goals) {
        int d = torus_dist(head, p);
        if (!best || d < best_d || (d == best_d && (p.x < best->x || (p.x == best->x && p.y < best->y)))) {
            best = p;
            best_d = d;
        }
    }
    return best;
}

std::vector<Direction> heading_towards(Position target) {
    Position head = mem.body[0];
    int dx = torus_delta(head.x, target.x, mem.w);
    int dy = torus_delta(head.y, target.y, mem.h);
    std::vector<Direction> order;
    auto push = [&](Direction d) {
        if (std::find(order.begin(), order.end(), d) == order.end())
            order.push_back(d);
    };
    if (std::abs(dx) >= std::abs(dy)) {
        if (dx > 0) push(Direction::EAST);
        else if (dx < 0) push(Direction::WEST);
        if (dy > 0) push(Direction::SOUTH);
        else if (dy < 0) push(Direction::NORTH);
    } else {
        if (dy > 0) push(Direction::SOUTH);
        else if (dy < 0) push(Direction::NORTH);
        if (dx > 0) push(Direction::EAST);
        else if (dx < 0) push(Direction::WEST);
    }
    for (auto d : DIRS)
        push(d);
    return order;
}

int neighbor_open_count(Controller& ct, Position pos, PosSet const& occupied) {
    int n = 0;
    for (auto direction : DIRS) {
        Transit t = mem.transit(ct, pos.x, pos.y, direction);
        if (!t.dest || t.kind == Kind::Kelp || t.kind == Kind::UnknownPortal)
            continue;
        if (occupied.count(*t.dest) || std::find(mem.body.begin(), mem.body.end(), *t.dest) != mem.body.end())
            continue;
        ++n;
    }
    return n;
}

std::pair<PosSet, bool> visible_pocket(Controller& ct, Position start, PosSet const& blocked) {
    if (blocked.count(start))
        return {{}, false};
    PosSet tiles{start};
    std::deque<Position> q{start};
    bool escapes = false;
    while (!q.empty()) {
        Position cur = q.front();
        q.pop_front();
        for (auto direction : DIRS) {
            Transit t = mem.transit(ct, cur.x, cur.y, direction);
            if (!t.dest || t.kind == Kind::Kelp || t.kind == Kind::Portal || t.kind == Kind::UnknownPortal)
                continue;
            if (blocked.count(*t.dest) || tiles.count(*t.dest))
                continue;
            if (!ct.get_tile(*t.dest)) {
                escapes = true;
                continue;
            }
            tiles.insert(*t.dest);
            q.push_back(*t.dest);
        }
    }
    return {tiles, escapes};
}

bool is_visible_dead_end(Controller& ct, Position dest, PosSet const& occupied) {
    if (!ct.get_tile(dest))
        return false;
    Position head = ct.get_position();
    PosSet blocked = occupied;
    for (auto const& p : mem.body)
        blocked.insert(p);
    blocked.insert(head);
    blocked.erase(dest);
    auto [tiles, escapes] = visible_pocket(ct, dest, blocked);
    if (escapes || tiles.empty())
        return tiles.empty();
    int max_deg = 0;
    for (auto const& p : tiles) {
        int deg = 0;
        for (auto direction : DIRS) {
            Transit t = mem.transit(ct, p.x, p.y, direction);
            if (t.dest && tiles.count(*t.dest))
                ++deg;
        }
        max_deg = std::max(max_deg, deg);
    }
    if (max_deg <= 2)
        return true;
    return static_cast<int>(tiles.size()) < ct.get_length();
}

int score_step(Controller& ct, Direction direction, Position dest, Kind kind, PosSet const& occupied,
               std::optional<Position> target) {
    int opens = neighbor_open_count(ct, dest, occupied);
    int score = opens * 4;
    if (mem.pearls.count(dest))
        score += 800;
    if (!mem.pearls.count(dest) && opens <= 1)
        score -= 70;
    auto eit = mem.eta.find(dest);
    int eta = eit == mem.eta.end() ? -1 : eit->second;
    if (eta >= 0 && eta <= 6)
        score += 40 - eta * 4;
    if (!mem.seen.count(dest))
        score += 18;
    if (kind == Kind::UnknownPortal)
        score -= 80;
    else if (kind == Kind::Portal)
        score -= 8;
    if (direction == ct.get_dir().get_opposite())
        score -= 25;
    if (target) {
        score -= torus_dist(dest, *target) * 6;
        score += torus_dist(mem.body[0], *target) * 3;
    }
    for (auto const& other : occupied) {
        if (torus_dist(dest, other) == 1)
            score -= 6;
    }
    return score;
}

std::vector<StepOpt> legal_steps(Controller& ct) {
    std::vector<StepOpt> options;
    Position here = ct.get_position();
    Tile const* here_tile = ct.get_tile(here);
    if (!here_tile)
        return options;
    for (auto direction : DIRS) {
        EdgeType edge = here_tile->get_edge(direction).get_edge_type();
        if (edge == EdgeType::KELP || edge == EdgeType::PORTAL)
            continue;
        Transit t = mem.transit(ct, here.x, here.y, direction);
        if (!t.dest || t.kind == Kind::Kelp || t.kind == Kind::Portal || t.kind == Kind::UnknownPortal)
            continue;
        Tile const* dest_tile = ct.get_tile(here.add_dir(direction));
        if (dest_tile && dest_tile->get_dragon())
            continue;
        if (std::find(mem.body.begin(), mem.body.end(), *t.dest) != mem.body.end())
            continue;
        options.push_back({direction, *t.dest, t.kind});
    }
    Direction reverse = ct.get_dir().get_opposite();
    std::vector<StepOpt> without_reverse;
    for (auto const& o : options) {
        if (!(o.direction == reverse))
            without_reverse.push_back(o);
    }
    return without_reverse.empty() ? options : without_reverse;
}

void classify_steps(Controller& ct, PosSet const& occupied, std::vector<StepOpt>& open_opts, std::vector<StepOpt>& dead_opts) {
    open_opts.clear();
    dead_opts.clear();
    for (auto const& item : legal_steps(ct)) {
        if (is_visible_dead_end(ct, item.dest, occupied))
            dead_opts.push_back(item);
        else
            open_opts.push_back(item);
    }
}

Direction fallback_move(Controller& ct) {
    Tile const* here = ct.get_tile(ct.get_position());
    Direction reverse = ct.get_dir().get_opposite();
    Direction best = Direction::NORTH;
    int best_pen = 1e9;
    for (auto direction : DIRS) {
        int penalty = 0;
        if (here && here->get_edge(direction).get_edge_type() == EdgeType::KELP)
            penalty += 1000;
        Tile const* ahead = ct.get_tile(ct.get_position().add_dir(direction));
        if (ahead && ahead->get_dragon())
            penalty += 400;
        if (direction == reverse)
            penalty += 30;
        if (penalty < best_pen || (penalty == best_pen && dir_index(direction) < dir_index(best))) {
            best_pen = penalty;
            best = direction;
        }
    }
    return best;
}

Direction pick_from_options(Controller& ct, std::vector<StepOpt> const& options, PosSet const& occupied,
                            std::vector<Direction> const& preferred, std::optional<Position> target,
                            std::optional<std::vector<Direction>> const& path) {
    if (options.empty())
        return fallback_move(ct);
    Direction reverse = ct.get_dir().get_opposite();
    Direction best = options[0].direction;
    int best_score = -1e9;
    for (auto const& opt : options) {
        Sim sim = simulate(ct, {opt.direction}, occupied);
        int score = score_step(ct, opt.direction, opt.dest, opt.kind, occupied, target);
        if (path && !path->empty() && (*path)[0] == opt.direction)
            score += 40;
        auto pit = std::find(preferred.begin(), preferred.end(), opt.direction);
        if (pit != preferred.end())
            score += 12 - static_cast<int>(pit - preferred.begin()) * 3;
        if (sim.eats)
            score += 200;
        if (sim.unknown_portal || !sim.ok)
            score -= 80;
        if (opt.direction == reverse)
            score -= 35;
        if (score > best_score || (score == best_score && dir_index(opt.direction) < dir_index(best))) {
            best_score = score;
            best = opt.direction;
        }
    }
    return best;
}

bool split_to_small(Controller& ct) {
    if (!ct.can_split(2))
        return false;
    int child = ct.get_length() - 2;
    if (!ct.can_split(child))
        child = 2;
    if (!ct.can_split(child))
        return false;
    ct.do_split(child);
    if (static_cast<int>(mem.body.size()) > child) {
        mem.body.resize(mem.body.size() - static_cast<std::size_t>(child));
        if (!mem.body.empty())
            mem.last_head = mem.body[0];
    }
    std::ostringstream os;
    os << "split small " << child;
    ct.set_indicator_string(os.str());
    return true;
}

int unit_cap() {
    int area = mem.w * mem.h;
    if (area <= 200)
        return 4;
    if (area <= 900)
        return 5;
    return 8;
}

bool want_split(Controller& ct, PosSet const& occupied, std::optional<std::vector<Direction>> const& pearl_path) {
    int length = ct.get_length();
    int units = ct.get_unit_count();
    int cap = unit_cap();
    int min_len = mem.w * mem.h <= 200 ? 6 : 8;
    if (!ct.can_split(2) || length < min_len || units >= cap)
        return false;
    if (pearl_path && pearl_path->size() == 1)
        return false;
    int safe = 0;
    for (auto direction : DIRS) {
        if (simulate(ct, {direction}, occupied).ok)
            ++safe;
    }
    return safe >= 2;
}

struct TargetInfo {
    PosSet goals;
    std::optional<Position> target;
    std::string reason;
};

TargetInfo choose_target(Controller& ct) {
    Position head = mem.body[0];
    PosSet visible_pearls;
    for (auto const& tile : ct.get_tiles()) {
        if (tile.has_pearl())
            visible_pearls.insert(tile.get_position());
    }
    if (!visible_pearls.empty()) {
        auto goal = nearest_goal(visible_pearls);
        return {visible_pearls, goal, "pearl"};
    }
    if (!mem.pearls.empty()) {
        auto goal = nearest_goal(mem.pearls);
        return {mem.pearls, goal, "memory-pearl"};
    }
    PosSet soon;
    for (auto const& [pos, eta] : mem.eta) {
        if (eta >= 0 && eta <= 8)
            soon.insert(pos);
    }
    if (!soon.empty()) {
        auto goal = nearest_goal(soon);
        return {soon, goal, "soon"};
    }
    PosSet unseen_frontier;
    for (auto const& pos : mem.seen) {
        for (auto direction : DIRS) {
            Transit t = mem.transit(ct, pos.x, pos.y, direction);
            if (!t.dest || t.kind == Kind::Kelp || t.kind == Kind::UnknownPortal)
                continue;
            if (!mem.seen.count(*t.dest))
                unseen_frontier.insert(*t.dest);
        }
    }
    if (!unseen_frontier.empty()) {
        auto goal = nearest_goal(unseen_frontier);
        return {unseen_frontier, goal, "explore"};
    }
    std::optional<Position> stale;
    int best_eta = 1000000000;
    for (auto const& [pos, eta] : mem.eta) {
        if (eta < 0)
            continue;
        if (eta < best_eta) {
            best_eta = eta;
            stale = pos;
        }
    }
    if (stale)
        return {{*stale}, stale, "respawn"};
    return {{}, std::nullopt, "wander"};
}

void emergency(Controller& ct) {
    ct.make_move(fallback_move(ct));
}

void execute_turn(Controller& ct, Game& game_state) {
    if (mem.w == 0) {
        auto [w, h] = game_state.get_map_size();
        mem.configure(w, h);
    }
    mem.observe(ct, game_state);
    int my_id = ct.get_id();
    PosSet occupied_others;
    for (auto const& tile : ct.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (!part || part->get_id() == my_id)
            continue;
        occupied_others.insert(tile.get_position());
    }

    Position head = ct.get_position();
    if (mem.body.empty() || !(mem.body[0] == head)) {
        std::vector<Position> next{head};
        for (auto const& cell : mem.body) {
            if (!(cell == head))
                next.push_back(cell);
        }
        if (next.size() > static_cast<std::size_t>(ct.get_length()))
            next.resize(static_cast<std::size_t>(ct.get_length()));
        if (next.empty())
            next.push_back(head);
        mem.body = std::move(next);
        mem.last_head = mem.body[0];
    }

    TargetInfo info = choose_target(ct);
    std::optional<std::vector<Direction>> path;
    if (!info.goals.empty())
        path = local_bfs(ct, occupied_others, info.goals);
    std::vector<Direction> preferred = info.target ? heading_towards(*info.target) : std::vector<Direction>(DIRS.begin(), DIRS.end());
    PosSet occupied = occupied_others;
    for (auto const& p : mem.body)
        occupied.insert(p);
    std::vector<StepOpt> open_opts, dead_opts;
    classify_steps(ct, occupied, open_opts, dead_opts);
    if (path && !path->empty()) {
        bool first_open = false;
        for (auto const& o : open_opts) {
            if (o.direction == (*path)[0])
                first_open = true;
        }
        if (!first_open)
            path.reset();
    }

    Direction move = Direction::NORTH;
    std::string reason = info.reason;
    if (!open_opts.empty()) {
        if (want_split(ct, occupied_others, path)) {
            int child = (ct.can_split(3) && ct.get_length() >= 10) ? 3 : 2;
            if (ct.can_split(child)) {
                ct.do_split(child);
                if (static_cast<int>(mem.body.size()) > child) {
                    mem.body.resize(mem.body.size() - static_cast<std::size_t>(child));
                    if (!mem.body.empty())
                        mem.last_head = mem.body[0];
                }
                std::ostringstream os;
                os << "split " << child;
                ct.set_indicator_string(os.str());
                return;
            }
        }
        move = pick_from_options(ct, open_opts, occupied_others, preferred, info.target, path);
    } else if (split_to_small(ct)) {
        return;
    } else {
        reason = "dead-end";
        move = pick_from_options(ct, dead_opts, occupied_others, preferred, info.target, path);
    }

    if (info.target)
        ct.draw_indicator_dot(*info.target, 255, 210, 40);
    std::ostringstream os;
    os << reason << " " << static_cast<char>(move.value);
    ct.set_indicator_string(os.str());
    ct.make_move(move);
    mem.apply_moves(ct, {move});
}

} // namespace

int main() {
    auto [ct, game] = unswbc::init();
    while (unswbc::update(ct, game)) {
        try {
            execute_turn(ct, game);
        } catch (...) {
            try {
                emergency(ct);
            } catch (...) {
                ct.make_move(Direction::NORTH);
            }
        }
        unswbc::end_turn();
    }
}
