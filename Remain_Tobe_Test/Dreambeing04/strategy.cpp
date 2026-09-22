#include "strategy.hpp"
#include "config.hpp"
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
bool Strategy::has_adjacent_pearl() const {
    const Cell head = cell(ct_.get_position());
    for (const auto direction : directions()) {
        const auto transition = mem_.transit(head, direction);
        if (transition.kind != TransitKind::Empty || !transition.destination) continue;
        const auto* tile = ct_.get_tile(position(*transition.destination));
        if (tile && tile->has_pearl()) return true;
    }
    return false;
}
SplitEvaluation Strategy::split_score(const Cells& occupied) const {
    int safe = 0;
    for (auto direction : directions())
        if (nav_.simulate({direction}, occupied).ok) ++safe;
    return evaluate_split_score(game_, ct_, safe, has_adjacent_pearl());
}
std::optional<int> Strategy::expansion_split_size() const {
    if (config::population::defer_split_for_adjacent_pearl && has_adjacent_pearl())
        return std::nullopt;
    const int child = config::population::expansion_child_size;
    if (!ct_.can_split(child)) return std::nullopt;
    return child;
}
std::optional<int> Strategy::emergency_split_size() const {
    // Survival splitting ignores adjacent pearls; the pearl veto is expansion-only.
    // tyhh keeps long dragons intact even when their movement options are poor.
    if (config::population::preserve_core_in_emergencies &&
        ct_.get_length() >= config::population::core_dragon_length) return std::nullopt;
    // Emergency splitting intentionally bypasses the soft population target.
    if (!ct_.can_split(2)) return std::nullopt;
    int child = ct_.get_length() - 2;
    if (!ct_.can_split(child)) child = 2;
    if (!ct_.can_split(child)) return std::nullopt;
    return child;
}
}
