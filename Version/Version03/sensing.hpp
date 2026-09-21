#pragma once
#include "safety.hpp"
#include "turn.hpp"

namespace mybot {
// C: Refresh persistent knowledge and build facts for this turn.
class Sensing {
public:
    Sensing(Memory& memory, const unswbc::Controller& controller,
            const unswbc::Game& game, const Safety& safety)
        : mem_(memory), ct_(controller), game_(game), safety_(safety) {}
    TurnContext SenseNeaby();
private:
    Memory& mem_;
    const unswbc::Controller& ct_;
    const unswbc::Game& game_;
    const Safety& safety_;
};
}
