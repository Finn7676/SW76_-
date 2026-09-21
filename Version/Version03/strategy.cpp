#include "strategy.hpp"
#include <limits>
namespace mybot {
Target Strategy::choose_target() const {
    Cells visible_pearls;
    for (const auto& tile : ct_.get_tiles())
        if (tile.has_pearl()) visible_pearls.insert(cell(tile.get_position()));
    if (!visible_pearls.empty()) return {visible_pearls, nav_.nearest_goal(visible_pearls), "pearl"};
    if (!mem_.pearls.empty()) return {mem_.pearls, nav_.nearest_goal(mem_.pearls), "memory-pearl"};
    Cells soon;
    for (const auto& [p, eta] : mem_.eta) if (eta >= 0 && eta <= 8) soon.insert(p);
    if (!soon.empty()) return {soon, nav_.nearest_goal(soon), "soon"};
    Cells frontier;
    for (Cell p : mem_.seen) for (auto d : directions()) {
        const auto t = mem_.transit(p, d);
        if (!t.destination || t.kind == TransitKind::Kelp || t.kind == TransitKind::UnknownPortal) continue;
        if (!mem_.seen.contains(*t.destination)) frontier.insert(*t.destination);
    }
    if (!frontier.empty()) return {frontier, nav_.nearest_goal(frontier), "explore"};
    std::optional<Cell> stale;
    int best_eta = 1000000000;
    for (Cell p : mem_.eta_order) {
        const auto it = mem_.eta.find(p);
        if (it == mem_.eta.end() || it->second < 0) continue;
        if (it->second < best_eta) { best_eta = it->second; stale = p; }
    }
    if (stale) return {Cells{*stale}, stale, "respawn"};
    return {};
}
int Strategy::unit_cap() const {
    const int area = mem_.width * mem_.height;
    return area <= 200 ? 4 : area <= 900 ? 5 : 8;
}
bool Strategy::want_split(const Cells& occupied, const Path& pearl_path) const {
    const int min_length = mem_.width * mem_.height <= 200 ? 6 : 8;
    if (!ct_.can_split(2) || ct_.get_length() < min_length || ct_.get_unit_count() >= unit_cap()) return false;
    if (pearl_path && pearl_path->size() == 1) return false;
    int safe = 0;
    for (auto d : directions()) if (nav_.simulate({d}, occupied).ok) ++safe;
    return safe >= 2;
}
std::optional<int> Strategy::normal_split_size() const {
    const int child = ct_.can_split(3) && ct_.get_length() >= 10 ? 3 : 2;
    if (!ct_.can_split(child)) return std::nullopt;
    return child;
}
std::optional<int> Strategy::emergency_split_size() const {
    // Emergency splitting intentionally bypasses the soft population target.
    if (!ct_.can_split(2)) return std::nullopt;
    int child = ct_.get_length() - 2;
    if (!ct_.can_split(child)) child = 2;
    if (!ct_.can_split(child)) return std::nullopt;
    return child;
}
}
