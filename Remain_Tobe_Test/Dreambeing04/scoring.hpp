#pragma once
#include "navigation.hpp"
#include "safety.hpp"
namespace mybot {
// B: Move ranking. Keep these weights together when tuning the bot.
class Scoring {
public:
    Scoring(const Memory& memory, const unswbc::Controller& controller,
            const Navigation& navigation, const Safety& safety)
        : mem_(memory), ct_(controller), nav_(navigation), safety_(safety) {}
    int score_step(const Step& step, const Cells& occupied, std::optional<Cell> target) const;
    Moves pick_from_options(const std::vector<Step>& options, const Cells& occupied,
                            const Moves& preferred, std::optional<Cell> target, const Path& path) const;
private:
    const Memory& mem_;
    const unswbc::Controller& ct_;
    const Navigation& nav_;
    const Safety& safety_;
};
}
