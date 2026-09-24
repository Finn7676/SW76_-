#pragma once

#include "memory.hpp"
#include "sense.hpp"
#include "navigation.hpp"
#include "tactics.hpp"

#include <optional>
#include <string>
#include <vector>

namespace xxqz {

enum class Reason { Stalk, Pearl, MemoryPearl, SharedPearl, Soon, Explore, Wander };

struct Pick {
    StepInfo step;
    Reason reason = Reason::Wander;
    std::optional<Position> goal;
    bool routed = false;
};

inline char const* reason_name(Reason reason) {
    switch (reason) {
    case Reason::Stalk: return "stalk";
    case Reason::Pearl: return "pearl";
    case Reason::MemoryPearl: return "memory-pearl";
    case Reason::SharedPearl: return "sonar-pearl";
    case Reason::Soon: return "soon";
    case Reason::Explore: return "explore";
    default: return "wander";
    }
}

inline std::pair<std::optional<Position>, Reason> choose_goal(Controller const& ct, View const& view, Memory const& map) {
    int length = ct.get_length();
    if (length <= MISSILE_MAX_LENGTH && ct.get_unit_count() >= 2) {
        Enemy const* target = nullptr;
        int best_visible = -1;
        int best_dist = 1000000000;
        for (Enemy const& enemy : view.enemies) {
            if (!enemy.has_head || !worth_killing(enemy.visible, length))
                continue;
            int dist = torus_dist(view.head, enemy.head, view.w, view.h);
            if (!target || enemy.visible > best_visible || (enemy.visible == best_visible && dist < best_dist)) {
                target = &enemy;
                best_visible = enemy.visible;
                best_dist = dist;
            }
        }
        if (target)
            return {target->head, Reason::Stalk};
    }

    auto pearls = map.pearls;
    pearls.erase(view.head);
    if (auto pearl = map.nearest(view.head, pearls))
        return {pearl, Reason::Pearl};
    if (auto soon = map.nearest(view.head, map.soon()))
        return {soon, Reason::Soon};
    if (auto frontier = map.nearest(view.head, map.frontier()))
        return {frontier, Reason::Explore};
    return {std::nullopt, Reason::Wander};
}

inline int score_step(Controller const& ct, View const& view, StepInfo const& step, std::optional<Position> goal, Reason reason) {
    int score = 0;
    int length = ct.get_length();
    bool stalking = reason == Reason::Stalk;
    bool large_map = view.w * view.h >= 900;
    bool crowded = ct.get_unit_count() >= (large_map ? 5 : 4);
    bool explorer = large_map && (ct.get_id() % 4 >= 2);
    if (step.pearl) {
        if (stalking && length >= 4)
            score += 80;
        else if (length < 4)
            score += 900;
        else if (crowded)
            score += 260;
        else
            score += 600;
    }
    if (step.eta >= 0 && step.eta <= 8)
        score += 30 - step.eta * 2;
    if (step.hard_trap)
        score -= 2000;
    else if (step.soft_trap)
        score -= 250;
    score += std::min(step.pocket, 12) * 3;
    if (crowded)
        score += std::min(step.pocket, 12) * 5;
    if (explorer && step.direction == directions()[static_cast<std::size_t>(ct.get_id() % 4)])
        score += 45;
    if (explorer && !step.pearl)
        score += 18;

    if (goal) {
        int here = torus_dist(view.head, *goal, view.w, view.h);
        int there = torus_dist(step.dest, *goal, view.w, view.h);
        if (length >= QUEEN_LENGTH && reason == Reason::Stalk)
            score += (there - here) * 8;
        else
            score += (here - there) * 12;
    } else {
        Direction prefer = directions()[static_cast<std::size_t>(ct.get_id() % 4)];
        if (step.direction == prefer)
            score += 30;
        else if (step.direction == prefer.get_left() || step.direction == prefer.get_right())
            score += 10;
    }

    for (Position const& ally : view.ally_heads) {
        if (torus_dist(step.dest, ally, view.w, view.h) <= 2)
            score -= 35;
    }
    if (step.direction == ct.get_dir().get_opposite())
        score -= 20;
    score += (ct.get_id() + dir_index(step.direction)) % 5;
    return score;
}

inline std::optional<Pick> choose(Controller const& ct, View const& view, Memory const& map,
                                  StrategyConfig const& config = strategy) {
    std::vector<StepInfo> roomy;
    std::vector<StepInfo> soft;
    std::vector<StepInfo> hard;
    for (StepInfo const& step : view.safe) {
        if (!step.hard_trap && !step.soft_trap)
            roomy.push_back(step);
        else if (!step.hard_trap && step.soft_trap)
            soft.push_back(step);
        else if (step.hard_trap)
            hard.push_back(step);
    }
    std::vector<StepInfo> const* candidates = !roomy.empty() ? &roomy : !soft.empty() ? &soft : !hard.empty() ? &hard : nullptr;
    if (!candidates)
        return std::nullopt;

    auto [goal, reason] = choose_goal(ct, view, map);
    std::optional<Route> route;
    if (config.routing && reason != Reason::Stalk) {
        route = find_forage_route(view, map, *candidates, config);
        if (route) {
            goal = route->goal;
            switch (route->kind) {
            case RouteKind::VisiblePearl: reason = Reason::Pearl; break;
            case RouteKind::RememberedPearl: reason = Reason::MemoryPearl; break;
            case RouteKind::SharedPearl: reason = Reason::SharedPearl; break;
            case RouteKind::Soon: reason = Reason::Soon; break;
            case RouteKind::Explore: reason = Reason::Explore; break;
            }
        } else {
            goal.reset();
            reason = Reason::Wander;
        }
    }
    StepInfo best = candidates->front();
    int best_score = -1000000000;
    for (StepInfo const& step : *candidates) {
        int score = score_step(ct, view, step, goal, reason);
        score += tactical_score(ct, view, step, config);
        if (route && step.direction == route->first)
            score += config.route_bonus;
        int index = dir_index(step.direction);
        if (score > best_score || (score == best_score && index < dir_index(best.direction))) {
            best = step;
            best_score = score;
        }
    }
    return Pick{best, reason, goal, route.has_value() && best.direction == route->first};
}

inline void paint(Controller& ct, Pick const& pick) {
    if (pick.goal && pick.reason != Reason::Wander) {
        if (pick.reason == Reason::Stalk)
            ct.draw_indicator_dot(*pick.goal, 255, 70, 70);
        else
            ct.draw_indicator_dot(*pick.goal, 255, 210, 40);
    }
    std::string label = std::string(reason_name(pick.reason)) + " " + static_cast<char>(pick.step.direction.value);
    if (pick.routed)
        label += " bfs";
    ct.set_indicator_string(label);
    ct.make_move(pick.step.direction);
}

}  // namespace xxqz
