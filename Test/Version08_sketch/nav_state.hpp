#pragma once

#include "model.hpp"
#include <optional>

namespace xxqz {

enum class RouteKind { VisiblePearl, RememberedPearl, SharedPearl, Soon, Explore };
enum class NavPurpose { Pearl, Explore, Combat, Escape };
enum class NavMode { Expand, Route, Retarget, Escape };

struct NavigationState {
    int dragon_id = -1;
    int sector = -1;
    int last_round = -1;
    std::optional<Position> target;
    RouteKind target_kind = RouteKind::Explore;
    NavPurpose purpose = NavPurpose::Explore;
    NavMode mode = NavMode::Route;
    int best_distance = 1000000000;
    int stagnant_rounds = 0;
    int failed_routes = 0;
    std::array<Position, 6> recent_heads{};
    int recent_count = 0;
    bool oscillating = false;
    std::optional<Direction> temporarily_avoid;
    int avoid_until = -1;
    std::optional<Position> rejected_target;
    int reject_until = -1;
    int escape_until = -1;

    void clear_target() {
        target.reset();
        best_distance = 1000000000;
        stagnant_rounds = 0;
        failed_routes = 0;
    }
    void interrupt() {
        clear_target();
        recent_count = 0;
        oscillating = false;
        temporarily_avoid.reset();
        escape_until = -1;
        mode = NavMode::Route;
        purpose = NavPurpose::Explore;
    }
};

}  // namespace xxqz
