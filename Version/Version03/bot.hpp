#pragma once
#include "strategy.hpp"
#include "scoring.hpp"
namespace mybot {
// A / integration: one persistent set of modules per dragon process.
class Bot {
public:
    Bot(unswbc::Controller& controller, unswbc::Game& game);
    void execute_turn();
    void emergency();
private:
    unswbc::Controller& ct_;
    unswbc::Game& game_;
    Memory mem_;
    Navigation navigation_;
    Safety safety_;
    Scoring scoring_;
    Strategy strategy_;
};
}
