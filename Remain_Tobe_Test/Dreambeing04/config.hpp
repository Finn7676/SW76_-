#pragma once
namespace mybot::config {
namespace combat {
inline constexpr bool enable_head_intercept = true;
inline constexpr int attacker_length = 2;
// Set true to keep the team's last dragon from attempting a sacrifice.
inline constexpr bool require_other_friendly_dragon = true;
static_assert(attacker_length == 2); // This strategy is for minimum-size dragons.
}
// Population/split experiment parameters. These defaults need match testing.
namespace population {
// Population baseline; only the round curve changes the desired population.
inline constexpr int early_population_target = 20;
inline constexpr int early_end_round = 240;
inline constexpr int middle_end_round = 320;
inline constexpr int late_ramp_rounds = 180;
inline constexpr double early_multiplier = 1.00;
inline constexpr double middle_multiplier = 1.00;
inline constexpr double late_multiplier = 0.15;
inline constexpr int middle_minimum = 20;
inline constexpr int late_minimum = 3;

inline constexpr int expansion_child_size = 2;
inline constexpr int expansion_min_length = 4;
// tyhh expands exactly at length 4 and preserves length >= 6 as score reserves.
inline constexpr int expansion_max_length = 4;
inline constexpr int core_dragon_length = 6;
inline constexpr bool preserve_core_in_emergencies = true;
inline constexpr int minimum_safe_directions = 0;
inline constexpr double deficit_weight = 100.0;
inline constexpr double early_score_bonus = 60.0;
inline constexpr double middle_score_bonus = 60.0;
inline constexpr double late_score_bonus = -40.0;
inline constexpr double extra_safe_direction_bonus = 0.0;
// Economic expansion only: defer when a pearl is next to the head.
inline constexpr bool defer_split_for_adjacent_pearl = true;
inline constexpr double split_score_threshold = 35.0;

static_assert(early_population_target > 0);
static_assert(early_end_round >= 0 && middle_end_round > early_end_round && late_ramp_rounds > 0);
static_assert(early_multiplier == 1.0 && middle_multiplier > 0 &&
              middle_multiplier <= early_multiplier && late_multiplier >= 0 &&
              late_multiplier <= middle_multiplier);
static_assert(middle_minimum >= late_minimum && late_minimum > 0);
// This version's expansion mode is explicitly x -> (x-2) + 2.
static_assert(expansion_child_size == 2 && expansion_min_length >= 4);
static_assert(expansion_max_length >= expansion_min_length);
static_assert(core_dragon_length > expansion_max_length);
static_assert(minimum_safe_directions >= 0 && minimum_safe_directions <= 4);
}
// Move-score values taken from tyhh/main.cpp. Other tactics remain Version03's.
namespace movement {
inline constexpr int pearl_bonus = 1400;
inline constexpr int imminent_spawn_bonus = 70;
inline constexpr int spawn_delay_penalty = 5;
inline constexpr int unseen_bonus = 30;
inline constexpr int target_distance_penalty = 8;
inline constexpr int route_bonus = 55;
inline constexpr int eating_bonus = 300;
}
inline constexpr int local_bfs_depth = 12;
inline constexpr int local_bfs_nodes = 220;
inline constexpr bool replay_indicators = true;
}
