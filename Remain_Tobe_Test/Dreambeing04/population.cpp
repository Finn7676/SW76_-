#include "population.hpp"
#include "config.hpp"
#include <cmath>

namespace mybot {
namespace {
namespace params = config::population;
struct Phase { double multiplier; double score_bonus; int minimum; };
double interpolate(double from, double to, double t) {
    return from + t * (to - from);
}
Phase phase_at(int round) {
    if (round <= params::early_end_round)
        return {params::early_multiplier, params::early_score_bonus, params::early_population_target};
    if (round <= params::middle_end_round) {
        const double t = static_cast<double>(round - params::early_end_round)
                       / (params::middle_end_round - params::early_end_round);
        return {interpolate(params::early_multiplier, params::middle_multiplier, t),
                interpolate(params::early_score_bonus, params::middle_score_bonus, t),
                params::middle_minimum};
    }
    const double t = std::clamp(static_cast<double>(round - params::middle_end_round)
                               / params::late_ramp_rounds, 0.0, 1.0);
    return {interpolate(params::middle_multiplier, params::late_multiplier, t),
            interpolate(params::middle_score_bonus, params::late_score_bonus, t),
            params::late_minimum};
}
}
int desired_population(const unswbc::Game& game) {
    const int early_cap = std::clamp(params::early_population_target, 1, game.get_unit_limit());
    const auto phase = phase_at(game.get_round_num());
    const int target = std::max(phase.minimum,
        static_cast<int>(std::round(early_cap * phase.multiplier)));
    // Engine limits must not make phase floors exceed the permitted baseline.
    return std::clamp(target, 1, early_cap);
}
SplitEvaluation evaluate_split_score(const unswbc::Game& game,
                                    const unswbc::Controller& controller,
                                    int safe_directions, bool adjacent_pearl) {
    SplitEvaluation result;
    result.current_population = controller.get_unit_count();
    result.desired_population = desired_population(game);
    result.safe_directions = safe_directions;
    const double deficit = static_cast<double>(result.desired_population - result.current_population)
                         / result.desired_population;
    result.score = params::deficit_weight * deficit
                 + phase_at(game.get_round_num()).score_bonus
                 + params::extra_safe_direction_bonus
                   * std::max(0, safe_directions - params::minimum_safe_directions);

    if (params::defer_split_for_adjacent_pearl && adjacent_pearl) {
        result.reason = "adjacent-pearl";
        return result;
    }
    // Hard eligibility gates remain distinct from the tunable preference score.
    if (!controller.can_split(params::expansion_child_size)) {
        result.reason = "engine-limit-or-length";
        return result;
    }
    if (controller.get_length() >= params::core_dragon_length) {
        result.reason = "preserve-core";
        return result;
    }
    if (controller.get_length() > params::expansion_max_length) {
        result.reason = "outside-expansion-length";
        return result;
    }
    if (controller.get_length() < params::expansion_min_length) {
        result.reason = "minimum-length";
        return result;
    }
    if (result.current_population >= result.desired_population) {
        result.reason = "population-target-met";
        return result;
    }
    if (safe_directions < params::minimum_safe_directions) {
        result.reason = "insufficient-safe-directions";
        return result;
    }
    result.eligible = true;
    result.should_split = result.score >= params::split_score_threshold;
    result.reason = result.should_split ? "expand" : "score-below-threshold";
    return result;
}
}
