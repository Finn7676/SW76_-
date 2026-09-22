#pragma once
#include "types.hpp"
#include <variant>

namespace mybot {
// Per-turn facts; rebuilt by sensing, then passed read-only to the decision layer.
struct TurnContext {
    Cells occupied_others;
    Options options;
};
enum class DecisionState { EmergencySplit, Escape, Split, Navigate, Fallback };
inline const char* state_name(DecisionState state) {
    switch (state) {
    case DecisionState::EmergencySplit: return "emergency-split";
    case DecisionState::Escape: return "escape";
    case DecisionState::Split: return "split";
    case DecisionState::Navigate: return "navigate";
    case DecisionState::Fallback: return "fallback";
    }
    return "unknown";
}
struct MoveAction { Direction direction; };
struct SplitAction { int child_size; };
struct Decision {
    DecisionState state;
    std::variant<MoveAction, SplitAction> action;
    std::optional<Cell> target;
    std::string reason;
};
} // namespace mybot
