#include "scoring.hpp"
#include <limits>
namespace mybot {
int Scoring::score_step(const Step& step, const Cells& occupied, std::optional<Cell> target) const {
    const int opens = safety_.neighbor_open_count(step.destination, occupied);
    int score = opens * 4;
    if (mem_.pearls.contains(step.destination)) score += 800;
    else if (opens <= 1) score -= 70;
    const auto found = mem_.eta.find(step.destination);
    const int eta = found == mem_.eta.end() ? -1 : found->second;
    if (0 <= eta && eta <= 6) score += 40 - eta * 4;
    if (!mem_.seen.contains(step.destination)) score += 18;
    if (step.kind == TransitKind::UnknownPortal) score -= 80;
    else if (step.kind == TransitKind::Portal) score -= 8;
    if (step.direction == ct_.get_dir().get_opposite()) score -= 25;
    if (target) {
        score -= nav_.distance(step.destination, *target) * 6;
        score += nav_.distance(mem_.body.at(0), *target) * 3;
    }
    for (Cell other : occupied) if (nav_.distance(step.destination, other) == 1) score -= 6;
    return score;
}
Moves Scoring::pick_from_options(const std::vector<Step>& options, const Cells& occupied,
                                 const Moves& preferred, std::optional<Cell> target, const Path& path) const {
    if (options.empty()) return safety_.fallback_move();
    int best = std::numeric_limits<int>::min();
    Direction choice = Direction::NORTH;
    for (const auto& step : options) {
        const auto simulated = nav_.simulate({step.direction}, occupied);
        int score = score_step(step, occupied, target);
        if (path && !path->empty() && path->front() == step.direction) score += 40;
        if (auto it = std::find(preferred.begin(), preferred.end(), step.direction); it != preferred.end())
            score += 12 - static_cast<int>(it - preferred.begin()) * 3;
        if (simulated.eats != 0) score += 200;
        if (simulated.unknown || !simulated.ok) score -= 80;
        if (step.direction == ct_.get_dir().get_opposite()) score -= 35;
        if (score > best || (score == best && direction_index(step.direction) < direction_index(choice))) {
            best = score; choice = step.direction;
        }
    }
    return {choice};
}
}
