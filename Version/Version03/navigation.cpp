#include "navigation.hpp"
#include "config.hpp"
#include <deque>
namespace mybot {
Simulation Navigation::simulate(const Moves& moves, const Cells& occupied_others) const {
    Simulation result;
    result.body = mem_.body;
    auto pearls = mem_.pearls;
    Cells own(result.body.begin(), result.body.end());
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (result.body.empty()) return result;
        const auto t = mem_.transit(result.body.front(), moves[i]);
        if (t.kind == TransitKind::Kelp || !t.destination) return result;
        if (t.kind == TransitKind::UnknownPortal || t.kind == TransitKind::Unknown) {
            result.unknown = true;
            if (i > 0 || t.kind == TransitKind::UnknownPortal) return result;
        }
        if (i > 0 && result.body.size() <= 2) return result;
        const Cell dest = *t.destination;
        if (occupied_others.contains(dest) || own.contains(dest)) return result;
        const bool ate = pearls.contains(dest);
        own.insert(dest);
        result.body.insert(result.body.begin(), dest);
        if (!ate) { own.erase(result.body.back()); result.body.pop_back(); }
        if (i > 0) {
            if (result.body.size() <= 2) return result;
            own.erase(result.body.back()); result.body.pop_back();
        }
        if (ate) { pearls.erase(dest); ++result.eats; }
    }
    result.ok = true;
    return result;
}
Path Navigation::local_bfs(const Cells& occupied_others, const Cells& goals, bool allow_unseen) const {
    if (mem_.body.empty()) return std::nullopt;
    const Cell start = mem_.body.front();
    if (goals.contains(start)) return Moves{};
    std::deque<Cell> queue{start};
    std::map<Cell, std::pair<Cell, Direction>> previous;
    std::map<Cell, int> dist{{start, 0}};
    std::optional<Cell> found;
    while (!queue.empty() && static_cast<int>(dist.size()) < config::local_bfs_nodes) {
        const Cell p = queue.front(); queue.pop_front();
        const int depth = dist.at(p);
        if (depth >= config::local_bfs_depth) continue;
        for (auto d : directions()) {
            const auto t = mem_.transit(p, d);
            if (!t.destination || t.kind == TransitKind::Kelp || t.kind == TransitKind::UnknownPortal) continue;
            const Cell dest = *t.destination;
            if (dist.contains(dest) || occupied_others.contains(dest) || contains(mem_.body, dest)) continue;
            if (!allow_unseen && !mem_.seen.contains(dest) && t.kind != TransitKind::Portal) continue;
            dist.emplace(dest, depth + 1);
            previous.emplace(dest, std::make_pair(p, d));
            if (goals.contains(dest)) { found = dest; break; }
            if (!mem_.seen.contains(dest)) continue;
            queue.push_back(dest);
        }
        if (found) break;
    }
    if (!found) return std::nullopt;
    Moves path;
    for (Cell p = *found; p != start;) {
        const auto& [parent, d] = previous.at(p);
        path.push_back(d); p = parent;
    }
    std::reverse(path.begin(), path.end());
    return path;
}
std::optional<Cell> Navigation::nearest_goal(const Cells& goals) const {
    if (goals.empty() || mem_.body.empty()) return std::nullopt;
    const Cell head = mem_.body.front();
    return *std::min_element(goals.begin(), goals.end(), [&](Cell a, Cell b) {
        return std::make_pair(distance(head, a), a) < std::make_pair(distance(head, b), b);
    });
}
Moves Navigation::heading_towards(Cell target) const {
    return mybot::heading_towards(mem_.body.at(0), target, mem_.width, mem_.height);
}
int Navigation::distance(Cell a, Cell b) const {
    return torus_distance(a, b, mem_.width, mem_.height);
}
}
