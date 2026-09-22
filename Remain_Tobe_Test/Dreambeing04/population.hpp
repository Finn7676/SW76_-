#pragma once
#include "types.hpp"

namespace mybot {
struct SplitEvaluation {
    double score = 0;
    int current_population = 0;
    int desired_population = 0;
    int safe_directions = 0;
    bool eligible = false;
    bool should_split = false;
    const char* reason = "not-evaluated";
};
// Round-dependent demand, clamped to the engine limit. No engine actions here.
int desired_population(const unswbc::Game& game);
SplitEvaluation evaluate_split_score(const unswbc::Game& game,
                                    const unswbc::Controller& controller,
                                    int safe_directions, bool adjacent_pearl);
}
