#include "combat.hpp"
#include "config.hpp"
namespace mybot {
std::optional<Decision> Combat::intercept(const TurnContext& context) const {
    if (!config::combat::enable_head_intercept ||
        ct_.get_length() != config::combat::attacker_length || mem_.body.empty()) return std::nullopt;
    if (config::combat::require_other_friendly_dragon && ct_.get_unit_count() <= 1)
        return std::nullopt;
    auto enemies = context.enemy_heads;
    std::sort(enemies.begin(), enemies.end(), [](const EnemyHead& a, const EnemyHead& b) {
        if (a.visible_segments != b.visible_segments) return a.visible_segments > b.visible_segments;
        return a.id < b.id;
    });
    for (const auto& enemy : enemies) {
        for (const auto direction : directions()) {
            const auto transition = mem_.transit(mem_.body.front(), direction);
            if (transition.kind != TransitKind::Empty || !transition.destination ||
                *transition.destination != enemy.position) continue;
            // Intentionally exempt only this visible enemy head from collision checks.
            // All remaining friendly/enemy body segments and our own body stay blocked.
            Cells obstacles = context.occupied_others;
            obstacles.erase(enemy.position);
            const auto simulated = nav_.simulate({direction}, obstacles);
            if (!simulated.ok || simulated.unknown) continue;
            return Decision{DecisionState::Intercept, MoveAction{direction}, enemy.position,
                "enemy=" + std::to_string(enemy.id) + " visible-length=" + std::to_string(enemy.visible_segments)};
        }
    }
    return std::nullopt;
}
}
