#pragma once

#include "view.hpp"
#include "debug.hpp"

#include <deque>

namespace xxqz {

inline void build_topology(View& view) {
    record(&DebugStats::component_builds);
    view.topology = TopologyCache{};
    // Undirected components only equal the old reachability flood fill when
    // adjacent visible free cells agree on whether their shared edge is EMPTY.
    // Preserve directed test/custom snapshots with the original algorithm.
    for (auto const& [pos, cell] : view.snap) {
        record(&DebugStats::component_validation_tiles);
        if (cell.owner != Owner::Empty)
            continue;
        for (int i = 0; i < 4; ++i) {
            auto next = view.snap.find(pos.add_dir(directions()[i]));
            if (next == view.snap.end() || next->second.owner != Owner::Empty)
                continue;
            if ((cell.edges[i] == EdgeType::EMPTY)
                != (next->second.edges[(i + 2) % 4] == EdgeType::EMPTY)) {
                record(&DebugStats::component_fallbacks);
                return;
            }
        }
    }
    auto& cache = view.topology;
    // Seed only regions reachable by an executable first step. Disconnected
    // regions cannot affect today's candidates; shared regions still fill once.
    auto origin = view.snap.find(view.head);
    if (origin == view.snap.end()) {
        cache.ready = true;
        return;
    }
    for (int first_direction = 0; first_direction < 4; ++first_direction) {
        if (origin->second.edges[first_direction] != EdgeType::EMPTY)
            continue;
        Position start = view.head.add_dir(directions()[first_direction]);
        auto first = view.snap.find(start);
        if (first == view.snap.end() || first->second.owner != Owner::Empty || cache.component_id.count(start))
            continue;
        int id = static_cast<int>(cache.regions.size());
        RegionInfo region;
        std::deque<Position> queue{start};
        cache.component_id.emplace(start, id);
        record(&DebugStats::component_regions);
        while (!queue.empty()) {
            Position pos = queue.front();
            queue.pop_front();
            record(&DebugStats::component_nodes);
            ++region.size;
            auto const& cell = view.snap.at(pos);
            int degree = 0;
            for (int i = 0; i < 4; ++i) {
                if (cell.edges[i] != EdgeType::EMPTY)
                    continue;
                Position dest = pos.add_dir(directions()[i]);
                auto next = view.snap.find(dest);
                if (next == view.snap.end()) {
                    region.touches_unknown = true;
                    continue;
                }
                if (next->second.owner != Owner::Empty)
                    continue;
                ++degree;
                if (cache.component_id.emplace(dest, id).second)
                    queue.push_back(dest);
            }
            region.min_degree = std::min(region.min_degree, degree);
            region.max_degree = std::max(region.max_degree, degree);
        }
        cache.regions.push_back(region);
    }
    cache.ready = true;
}

}  // namespace xxqz
