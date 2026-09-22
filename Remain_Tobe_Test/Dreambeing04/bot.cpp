#include "bot.hpp"
#include "config.hpp"
namespace mybot {
Bot::Bot(unswbc::Controller& controller, unswbc::Game& game)
    : ct_(controller), mem_(controller, game), navigation_(mem_),
      safety_(mem_, controller), scoring_(mem_, controller, navigation_, safety_),
      strategy_(mem_, controller, navigation_, game), sensing_(mem_, controller, game, safety_),
      combat_(mem_, controller, navigation_),
      decision_(strategy_, navigation_, scoring_, safety_, combat_) {}

void Bot::execute_turn() {
    acted_ = false;
    const auto context = SenseNeaby();
    const auto decision = Decide(context);
    Move(decision);
}
TurnContext Bot::SenseNeaby() { return sensing_.SenseNeaby(); }
Decision Bot::Decide(const TurnContext& context) { return decision_.Decide(context); }

void Bot::Move(const Decision& decision) {
    if (acted_) return;
    // Move is the action phase: it can execute movement OR splitting.
    if (const auto* split = std::get_if<SplitAction>(&decision.action)) {
        ct_.do_split(split->child_size);
        acted_ = true;
        mem_.remember_split(split->child_size);
    } else {
        const auto direction = std::get<MoveAction>(decision.action).direction;
        ct_.make_move(direction);
        acted_ = true;
        mem_.apply_moves({direction});
    }
    if constexpr (config::replay_indicators) {
        if (decision.target) ct_.draw_indicator_dot(position(*decision.target), 255, 210, 40);
        ct_.set_indicator_string(std::string(state_name(decision_.previous_state())) + " -> "
            + state_name(decision.state) + ": " + decision.reason);
    }
}
void Bot::emergency() {
    // Do not overwrite an emitted action if memory/diagnostics threw afterwards.
    if (acted_) return;
    const auto moves = safety_.fallback_move();
    const auto direction = moves.empty() ? Direction{Direction::NORTH} : moves.front();
    Move({DecisionState::Fallback, MoveAction{direction}, std::nullopt, "exception"});
}
}
