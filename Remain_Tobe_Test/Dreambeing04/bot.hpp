#pragma once
#include "decision.hpp"
#include "sensing.hpp"
namespace mybot {
// A: Integration and the single engine-action boundary.
class Bot {
public:
    Bot(unswbc::Controller& controller, unswbc::Game& game);
    void execute_turn();
    void emergency();
    bool has_acted() const { return acted_; }
private:
    TurnContext SenseNeaby();
    Decision Decide(const TurnContext& context);
    void Move(const Decision& decision);
    unswbc::Controller& ct_;
    Memory mem_;
    Navigation navigation_;
    Safety safety_;
    Scoring scoring_;
    Strategy strategy_;
    Sensing sensing_;
    Combat combat_;
    DecisionMachine decision_;
    bool acted_ = false;
};
}
