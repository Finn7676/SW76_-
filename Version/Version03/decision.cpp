#include "decision.hpp"
#include <stdexcept>

namespace mybot {
DecisionMachine::Plan DecisionMachine::planRoute(const TurnContext& context) const {
    Plan plan;
    plan.target = strategy_.choose_target();
    if (!plan.target.goals.empty())
        plan.path = nav_.local_bfs(context.occupied_others, plan.target.goals, true);
    plan.preferred = plan.target.position ? nav_.heading_towards(*plan.target.position) : directions();
    if (plan.path && !plan.path->empty()) {
        const bool open = std::any_of(context.options.open.begin(), context.options.open.end(),
            [&](const Step& step) { return step.direction == plan.path->front(); });
        if (!open) plan.path.reset();
    }
    return plan;
}
bool DecisionMachine::isEmergency(const TurnContext& context, const Plan&) const {
    return context.options.open.empty();
}
bool DecisionMachine::shouldSplit(const TurnContext& context, const Plan& plan) const {
    return strategy_.want_split(context.occupied_others, plan.path)
        && strategy_.normal_split_size().has_value();
}
bool DecisionMachine::canNavigate(const TurnContext& context, const Plan&) const {
    return !context.options.open.empty();
}
DecisionState DecisionMachine::selectState(const TurnContext& context, const Plan& plan) const {
    using Guard = bool (DecisionMachine::*)(const TurnContext&, const Plan&) const;
    struct Rule { Guard guard; DecisionState destination; };
    // First matching rule wins, from every previous state. Emergencies preempt growth.
    static constexpr Rule rules[] = {
        {&DecisionMachine::isEmergency, DecisionState::EmergencySplit},
        {&DecisionMachine::shouldSplit, DecisionState::Split},
        {&DecisionMachine::canNavigate, DecisionState::Navigate},
    };
    for (const auto& rule : rules)
        if ((this->*rule.guard)(context, plan)) return rule.destination;
    return DecisionState::Fallback;
}
Decision DecisionMachine::planMove(DecisionState state, const std::vector<Step>& options,
                                   const TurnContext& context, const Plan& plan) const {
    if (options.empty()) return runState(DecisionState::Fallback, context, plan);
    const auto moves = scoring_.pick_from_options(options, context.occupied_others,
        plan.preferred, plan.target.position, plan.path);
    if (moves.empty()) return runState(DecisionState::Fallback, context, plan);
    return {state, MoveAction{moves.front()}, plan.target.position,
            state == DecisionState::Escape ? "dead-end" : plan.target.reason};
}
Decision DecisionMachine::runState(DecisionState state, const TurnContext& context, const Plan& plan) const {
    // Explicit bounded transitions; no handler sends commands to the engine.
    switch (state) {
    case DecisionState::EmergencySplit:
        if (const auto child = strategy_.emergency_split_size())
            return {state, SplitAction{*child}, std::nullopt, "split small"};
        return runState(DecisionState::Escape, context, plan);
    case DecisionState::Escape:
        return planMove(state, context.options.dead, context, plan);
    case DecisionState::Split:
        if (const auto child = strategy_.normal_split_size())
            return {state, SplitAction{*child}, std::nullopt, "split"};
        return runState(DecisionState::Navigate, context, plan);
    case DecisionState::Navigate:
        return planMove(state, context.options.open, context, plan);
    case DecisionState::Fallback: {
        const auto moves = safety_.fallback_move();
        const auto direction = moves.empty() ? Direction{Direction::NORTH} : moves.front();
        return {state, MoveAction{direction}, plan.target.position, "fallback"};
    }
    }
    throw std::logic_error("Unknown decision state");
}
Decision DecisionMachine::Decide(const TurnContext& context) {
    const auto plan = planRoute(context);
    auto decision = runState(selectState(context, plan), context, plan);
    previous_ = state_;
    state_ = decision.state;
    return decision;
}
}
