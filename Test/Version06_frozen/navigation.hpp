#pragma once

#include "memory.hpp"
#include "sense.hpp"

#include <deque>
#include <optional>

namespace xxqz {

enum class RouteKind { VisiblePearl, RememberedPearl, SharedPearl, Soon, Explore };

struct Route {
    Position goal;
    Direction first = Direction::NORTH;
    int distance = 0;
    RouteKind kind = RouteKind::Explore;
};

inline bool route_edge_open(Position pos, int index, View const& view, Memory const& map) {
    auto live = view.snap.find(pos);
    if (live != view.snap.end())
        return live->second.edges[index] == EdgeType::EMPTY;
    auto edge = map.edges.find(EdgeKey{pos.x, pos.y, index});
    // Unknown terrain is a planning hypothesis. The executed first step must be in view.safe.
    return edge == map.edges.end() || edge->second == EdgeType::EMPTY;
}

inline std::optional<Route> find_forage_route(View const& view, Memory const& map,
                                             std::vector<StepInfo> const& allowed_first,
                                             StrategyConfig const& config = strategy) {
    record(&DebugStats::route_calls);
    if (allowed_first.empty() || config.route_nodes < 2 || config.route_depth < 1)
        return std::nullopt;
    std::unordered_map<Position, RouteKind, PositionHash> goals;
    auto add = [&](Position pos, RouteKind kind) {
        if (pos == view.head)
            return;
        auto [it, inserted] = goals.emplace(pos, kind);
        if (!inserted && static_cast<int>(kind) < static_cast<int>(it->second))
            it->second = kind;
    };
    for (auto const& [pos, cell] : view.snap) {
        record(&DebugStats::goal_tiles_scanned);
        if (cell.pearl && cell.owner == Owner::Empty)
            add(pos, RouteKind::VisiblePearl);
    }
    for (Position pos : map.pearls)
        add(pos, RouteKind::RememberedPearl);
    if (config.sonar)
        for (auto const& [pos, timestamp] : map.shared_pearls)
            if (map.last_round >= timestamp && map.last_round - timestamp <= config.sonar_ttl)
                add(pos, RouteKind::SharedPearl);
    for (Position pos : map.soon())
        add(pos, RouteKind::Soon);
    for (Position pos : map.frontier())
        add(pos, RouteKind::Explore);
    if (goals.empty())
        return std::nullopt;

    std::array<bool, 4> first_allowed{};
    for (auto const& step : allowed_first)
        first_allowed[dir_index(step.direction)] = true;
    struct Node { Position pos; Direction first; int depth; };
    std::deque<Node> queue{{view.head, Direction::NORTH, 0}};
    std::unordered_set<Position, PositionHash> seen{view.head};
    record(&DebugStats::route_nodes);
    std::optional<Route> best;
    while (!queue.empty()) {
        Node node = queue.front();
        queue.pop_front();
        record(&DebugStats::route_expanded);
        if (node.depth >= config.route_depth)
            continue;
        for (int index = 0; index < 4; ++index) {
            if ((node.depth == 0 && !first_allowed[index]) || !route_edge_open(node.pos, index, view, map))
                continue;
            Position dest = node.pos.add_dir(directions()[index]);
            auto live = view.snap.find(dest);
            if ((live != view.snap.end() && live->second.owner != Owner::Empty) || seen.count(dest))
                continue;
            if (static_cast<int>(seen.size()) >= config.route_nodes)
                continue;
            seen.insert(dest);
            record(&DebugStats::route_nodes);
            Direction first = node.depth == 0 ? directions()[index] : node.first;
            int distance = node.depth + 1;
            if (auto goal = goals.find(dest); goal != goals.end()) {
                if (!best || static_cast<int>(goal->second) < static_cast<int>(best->kind))
                    best = Route{dest, first, distance, goal->second};
                // FIFO traversal makes the first goal within each tier the shortest path.
                if (goal->second == RouteKind::VisiblePearl)
                    return best;
            }
            queue.push_back(Node{dest, first, distance});
        }
    }
    return best;
}

}  // namespace xxqz
