#pragma once
#include "navigation.hpp"
#include "turn.hpp"
namespace mybot {
// A/B: Plan intentional single-step enemy-head collisions. Never emit commands.
class Combat {
public:
    Combat(const Memory& memory, const unswbc::Controller& controller, const Navigation& navigation)
        : mem_(memory), ct_(controller), nav_(navigation) {}
    std::optional<Decision> intercept(const TurnContext& context) const;
private:
    const Memory& mem_;
    const unswbc::Controller& ct_;
    const Navigation& nav_;
};
}
