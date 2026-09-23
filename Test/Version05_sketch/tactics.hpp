#pragma once

#include "sense.hpp"
#include "strategy.hpp"

#include <optional>

namespace xxqz {

inline std::optional<Direction> short_trade(Controller const& ct, View const& view,
                                            StrategyConfig const& config = strategy) {
    if (!config.short_trade || ct.get_length() != 2 || ct.get_unit_count() <= 1)
        return std::nullopt;
    auto origin = view.snap.find(view.head);
    if (origin == view.snap.end())
        return std::nullopt;
    std::optional<Direction> best;
    int best_length = ct.get_length();
    for (int index = 0; index < 4; ++index) {
        if (origin->second.edges[index] != EdgeType::EMPTY)
            continue;
        Direction direction = directions()[index];
        Position dest = view.head.add_dir(direction);
        for (auto const& enemy : view.enemies) {
            record(&DebugStats::short_trade_enemy_checks);
            if (enemy.has_head && enemy.head == dest && enemy.visible > best_length) {
                best = direction;
                best_length = enemy.visible;
            }
        }
    }
    return best;
}

inline int enemy_escapes(Enemy const& enemy, View const& view) {
    record(&DebugStats::escape_calls);
    auto head = view.snap.find(enemy.head);
    if (head == view.snap.end())
        return 4;
    int count = 0;
    for (int index = 0; index < 4; ++index) {
        record(&DebugStats::escape_edges);
        EdgeType edge = head->second.edges[index];
        if (edge == EdgeType::KELP)
            continue;
        // An unknown portal exit is a possible escape, never evidence of a trap.
        if (edge == EdgeType::PORTAL) {
            ++count;
            continue;
        }
        auto dest = view.snap.find(enemy.head.add_dir(directions()[index]));
        if (dest == view.snap.end() || dest->second.owner == Owner::Empty)
            ++count;
    }
    return count;
}

inline int tactical_score(Controller const& ct, View const& view, StepInfo const& step,
                          StrategyConfig const& config = strategy) {
    int score = 0;
    for (Enemy const& enemy : view.enemies) {
        record(&DebugStats::tactical_enemy_checks);
        if (!enemy.has_head)
            continue;
        auto head = view.snap.find(enemy.head);
        if (head == view.snap.end())
            continue;
        bool reaches = false;
        bool forward = false;
        for (int index = 0; index < 4; ++index) {
            if (head->second.edges[index] != EdgeType::EMPTY)
                continue;
            Direction direction = directions()[index];
            if (enemy.head.add_dir(direction) == step.dest) {
                reaches = true;
                forward = direction == head->second.facing;
            }
        }
        if (!reaches)
            continue;
        bool hunting = config.corner_pressure && view.w * view.h <= config.hunt_area
            && ct.get_length() <= MISSILE_MAX_LENGTH && ct.get_unit_count() > 1;
        int escapes = enemy_escapes(enemy, view);
        if (hunting && escapes <= 2) {
            score += (3 - escapes) * config.corner_bonus;
        } else if (config.head_risk && (view.w * view.h > config.head_risk_area
                                      || ct.get_length() > MISSILE_MAX_LENGTH)) {
            score -= config.adjacent_risk;
            if (forward)
                score -= config.forward_risk;
        }
    }
    return score;
}

}  // namespace xxqz
