#pragma once
#include "strategy.hpp"
#include "combat.hpp"
#include "scoring.hpp"
#include "turn.hpp"

namespace mybot {
// A: Reactive state machine. Priority rules select one behavior each turn.
class DecisionMachine {
public:
    DecisionMachine(const Strategy& strategy, const Navigation& navigation,
                    const Scoring& scoring, const Safety& safety, const Combat& combat)
        : strategy_(strategy), nav_(navigation), scoring_(scoring), safety_(safety), combat_(combat) {}
    Decision Decide(const TurnContext& context);
    DecisionState state() const { return state_; }
    DecisionState previous_state() const { return previous_; }
private:
    struct Plan { Target target; Path path; Moves preferred; SplitEvaluation split; std::optional<Decision> attack; };
    Plan planRoute(const TurnContext& context) const;
    bool canIntercept(const TurnContext&, const Plan& plan) const;
    bool isEmergency(const TurnContext& context, const Plan&) const;
    bool shouldSplit(const TurnContext& context, const Plan& plan) const;
    bool canNavigate(const TurnContext& context, const Plan&) const;
    DecisionState selectState(const TurnContext& context, const Plan& plan) const;
    Decision runState(DecisionState state, const TurnContext& context, const Plan& plan) const;
    Decision planMove(DecisionState state, const std::vector<Step>& options,
                      const TurnContext& context, const Plan& plan) const;
    const Strategy& strategy_;
    const Navigation& nav_;
    const Scoring& scoring_;
    const Safety& safety_;
    const Combat& combat_;
    DecisionState state_ = DecisionState::Fallback;
    DecisionState previous_ = DecisionState::Fallback;
};
}
