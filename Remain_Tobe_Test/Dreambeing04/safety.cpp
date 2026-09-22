#include "safety.hpp"
#include <deque>
#include <limits>
namespace mybot {
int Safety::neighbor_open_count(Cell p, const Cells& occupied) const {
    int count = 0;
    for (auto d : directions()) {
        const auto t = mem_.transit(p, d);
        if (!t.destination || t.kind == TransitKind::Kelp || t.kind == TransitKind::UnknownPortal) continue;
        if (occupied.contains(*t.destination) || contains(mem_.body, *t.destination)) continue;
        ++count;
    }
    return count;
}
std::pair<Cells, bool> Safety::visible_pocket(Cell start, const Cells& blocked) const {
    if (blocked.contains(start)) return {Cells{}, false};
    Cells tiles{start};
    std::deque<Cell> queue{start};
    bool escapes = false;
    while (!queue.empty()) {
        const Cell p = queue.front(); queue.pop_front();
        for (auto d : directions()) {
            const auto t = mem_.transit(p, d);
            if (!t.destination || t.kind == TransitKind::Kelp || t.kind == TransitKind::Portal || t.kind == TransitKind::UnknownPortal) continue;
            const Cell dest = *t.destination;
            if (blocked.contains(dest) || tiles.contains(dest)) continue;
            if (!ct_.get_tile(position(dest))) { escapes = true; continue; }
            tiles.insert(dest); queue.push_back(dest);
        }
    }
    return {std::move(tiles), escapes};
}
bool Safety::is_visible_dead_end(Cell destination, const Cells& occupied) const {
    if (!ct_.get_tile(position(destination))) return false;
    Cells blocked = occupied;
    blocked.insert(mem_.body.begin(), mem_.body.end());
    blocked.insert(cell(ct_.get_position()));
    blocked.erase(destination);
    const auto [tiles, escapes] = visible_pocket(destination, blocked);
    if (escapes || tiles.empty()) return tiles.empty();
    int max_degree = 0;
    for (Cell p : tiles) {
        int degree = 0;
        for (auto d : directions()) {
            const auto t = mem_.transit(p, d);
            if (t.destination && tiles.contains(*t.destination)) ++degree;
        }
        max_degree = std::max(max_degree, degree);
    }
    // Preserved verbatim as a policy: enclosed cycles can be labelled dead ends.
    if (max_degree <= 2) return true;
    return static_cast<int>(tiles.size()) < ct_.get_length();
}
std::vector<Step> Safety::legal_steps() const {
    const Cell here = cell(ct_.get_position());
    const auto* tile = ct_.get_tile(position(here));
    std::vector<Step> options;
    if (!tile) return options;
    for (auto d : directions()) {
        const auto& edge = tile->get_edge(d);
        // The Python movement policy rejects even fully discovered portals.
        if (edge.get_edge_type() == unswbc::EdgeType::KELP || edge.is_portal()) continue;
        const auto t = mem_.transit(here, d);
        if (!t.destination || t.kind == TransitKind::Kelp || t.kind == TransitKind::Portal || t.kind == TransitKind::UnknownPortal) continue;
        const auto* ahead = ct_.get_tile(position(mem_.step(here, d)));
        if (ahead && ahead->get_dragon()) continue;
        if (contains(mem_.body, *t.destination)) continue;
        options.push_back({d, *t.destination, t.kind});
    }
    std::vector<Step> without_reverse;
    for (const auto& option : options)
        if (option.direction != ct_.get_dir().get_opposite()) without_reverse.push_back(option);
    return without_reverse.empty() ? options : without_reverse;
}
Options Safety::classify_steps(const Cells& occupied) const {
    Options result;
    for (const auto& step : legal_steps()) {
        auto& group = is_visible_dead_end(step.destination, occupied) ? result.dead : result.open;
        group.push_back(step);
    }
    return result;
}
Moves Safety::fallback_move() const {
    const Cell p = cell(ct_.get_position());
    const auto* here = ct_.get_tile(position(p));
    const auto reverse = ct_.get_dir().get_opposite();
    int best = std::numeric_limits<int>::max();
    Direction choice = Direction::NORTH;
    for (auto d : directions()) {
        int penalty = 0;
        if (here && here->get_edge(d).get_edge_type() == unswbc::EdgeType::KELP) penalty += 1000;
        const auto* ahead = ct_.get_tile(position(mem_.step(p, d)));
        if (ahead && ahead->get_dragon()) penalty += 400;
        if (d == reverse) penalty += 30;
        // Iteration order N/E/S/W matches Python's explicit final tie-break.
        if (penalty < best) { best = penalty; choice = d; }
    }
    return {choice};
}
}
