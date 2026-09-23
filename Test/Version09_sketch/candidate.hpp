#pragma once

#include "threats.hpp"

#include <algorithm>
#include <optional>
#include <unordered_set>

namespace xxqz {

// Ordered by preference. These are conservative local classes, not death proofs.
enum class MoveClass { Clear, FavorableExposure, Exposure, Constrained, NoExit, Illegal };

struct MoveEval {
    StepInfo step;
    MoveClass category = MoveClass::Illegal;
    int space = 0;
    int exits = 0;
    int unknown_exits = 0;
    int enemy_reach = 0;
    int enemy_forward_reach = 0;
    int ally_pressure = 0;
    int pearl_value = 0;
    int route_progress = 0;
    int navigation_value = 0;
    int route_value = 0;
    int resource_value = 0;
    int mobility_value = 0;
    int pressure_value = 0;
    bool tail_released = false;
    bool open_boundary = false;
    bool enters_trap = false;
    // Mutual death would be favorable by visible enemy length, not a won collision.
    bool favorable_trade = false;
};

inline MoveEval evaluate_move(Controller const& ct, View const& view, StepInfo const& step,
                              StrategyConfig const& config = strategy) {
    record(&DebugStats::micro_candidates);
    MoveEval result;
    result.step = step;
    auto source = view.snap.find(view.head), destination = view.snap.find(step.dest);
    if (step.dest != view.head.add_dir(step.direction) || source == view.snap.end()
        || source->second.edges[dir_index(step.direction)] != EdgeType::EMPTY
        || destination == view.snap.end() || destination->second.owner != Owner::Empty)
        return result;

    // The first move never enters a body tile. Only after it succeeds can the
    // known tail vacate; a pearl retains it. Incomplete bodies remain obstacles.
    result.tail_released = view.body_complete && !step.pearl && !view.body.empty();
    auto free_after = [&](Position pos) {
        if (pos == step.dest) return true;
        auto cell = view.snap.find(pos);
        if (cell == view.snap.end()) return false;
        return cell->second.owner == Owner::Empty
            || (result.tail_released && pos == view.body.back());
    };
    for (int i = 0; i < 4; ++i) {
        auto edge = destination->second.edges[i];
        Position next = step.dest.add_dir(directions()[i]);
        // Unknown/portal exits prevent claiming a closed trap but are not
        // executable known exits. Never count looping into the new head.
        if (edge == EdgeType::PORTAL || (edge == EdgeType::EMPTY && !view.snap.count(next)))
            ++result.unknown_exits;
        else if (edge == EdgeType::EMPTY && next != step.dest && free_after(next))
            ++result.exits;
    }

    // V07's pocket is the current-state fact. Recompute its candidate-specific
    // delta with the predicted occupancy, bounded by visible snapshot size.
    // No second raw-vision scan, memory mutation or navigation search occurs.
    std::vector<Position> queue{step.dest};
    std::unordered_set<Position, PositionHash> visited{step.dest};
    int min_degree = 4, max_degree = 0;
    for (std::size_t at = 0; at < queue.size(); ++at) {
        record(&DebugStats::micro_nodes);
        Position here = queue[at];
        auto cell = view.snap.find(here);
        int degree = 0;
        for (int i = 0; i < 4; ++i) {
            auto edge = cell->second.edges[i];
            Position next = here.add_dir(directions()[i]);
            if (edge == EdgeType::PORTAL || (edge == EdgeType::EMPTY && !view.snap.count(next))) {
                result.open_boundary = true;
                continue;
            }
            if (edge != EdgeType::EMPTY || next == here || !free_after(next)) continue;
            ++degree;
            if (visited.insert(next).second) queue.push_back(next);
        }
        min_degree = std::min(min_degree, degree);
        max_degree = std::max(max_degree, degree);
    }
    result.space = static_cast<int>(queue.size());
    result.enters_trap = !result.open_boundary
        && (result.space < ct.get_length() + (step.pearl ? 1 : 0)
            || (max_degree <= 2 && min_degree <= 1));

    // Use V07's direction masks. All legal candidates must be in view.safe.
    if (!config.threat_cache || !view.threats.ready) build_threats(view);
    auto bit = static_cast<std::uint8_t>(1U << dir_index(step.direction));
    bool all_favorable = true;
    for (auto const& enemy : view.threats.enemies) {
        if (!(enemy.reachable_mask & bit)) continue;
        ++result.enemy_reach;
        if (enemy.forward_mask & bit) ++result.enemy_forward_reach;
        // The enemy length is a lower bound; a small visible count cannot prove
        // that it is shorter than us. This is a payoff policy, not such a claim.
        all_favorable &= ct.get_length() <= MISSILE_MAX_LENGTH && ct.get_unit_count() > 1
            && worth_killing(enemy.visible_length, ct.get_length() + (step.pearl ? 1 : 0));
    }
    result.favorable_trade = result.enemy_reach > 0 && all_favorable;
    for (auto const& ally : view.ally_heads)
        if (torus_dist(step.dest, ally, view.w, view.h) <= 2) ++result.ally_pressure;
    if (!result.exits && !result.unknown_exits) result.category = MoveClass::NoExit;
    else if (result.enters_trap) result.category = MoveClass::Constrained;
    else if (!result.enemy_reach) result.category = MoveClass::Clear;
    else result.category = result.favorable_trade ? MoveClass::FavorableExposure : MoveClass::Exposure;
    return result;
}

inline int rank_move(Controller const& ct, View const& view, MoveEval& eval,
                     std::optional<Position> goal, bool stalking, int navigation_value, int route_value) {
    auto const& step = eval.step;
    eval.pearl_value = step.pearl ? (stalking && ct.get_length() >= 4 ? 80 : ct.get_length() < 4 ? 900 : 600) : 0;
    eval.resource_value = eval.pearl_value + (step.eta >= 0 && step.eta <= 8 ? 30 - 2 * step.eta : 0);
    eval.route_progress = 0;
    if (goal) {
        int delta = torus_dist(view.head, *goal, view.w, view.h) - torus_dist(step.dest, *goal, view.w, view.h);
        eval.route_progress = delta * (stalking && ct.get_length() >= QUEEN_LENGTH ? -8 : 12);
    } else {
        Direction preferred = directions()[static_cast<std::size_t>(ct.get_id() % 4)];
        eval.route_progress = step.direction == preferred ? 30
            : step.direction == preferred.get_left() || step.direction == preferred.get_right() ? 10 : 0;
    }
    eval.mobility_value = std::min(eval.space, 12) * 3 + std::min(eval.exits, 3) * 18;
    eval.pressure_value = -35 * eval.ally_pressure -70 * eval.enemy_reach -180 * eval.enemy_forward_reach;
    eval.navigation_value = navigation_value;
    eval.route_value = route_value;
    return eval.resource_value + eval.route_progress + eval.mobility_value + eval.pressure_value
        + eval.navigation_value + eval.route_value
        - (step.direction == ct.get_dir().get_opposite() ? 20 : 0)
        + (ct.get_id() + dir_index(step.direction)) % 5;
}

inline void record_choice(MoveEval const& eval) {
    record(&DebugStats::micro_selected);
    record(&DebugStats::micro_trap, eval.enters_trap);
    record(&DebugStats::micro_exposed, eval.enemy_reach > 0);
    record(&DebugStats::micro_favorable, eval.favorable_trade);
    record(&DebugStats::micro_space, eval.space);
    record(&DebugStats::micro_exits, eval.exits);
    record(&DebugStats::micro_pearl, eval.step.pearl);
    record(&DebugStats::micro_tail_release, eval.tail_released);
}

} // namespace xxqz
