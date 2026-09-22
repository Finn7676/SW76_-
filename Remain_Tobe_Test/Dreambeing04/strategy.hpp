#pragma once
#include "navigation.hpp"
#include "population.hpp"
namespace mybot {
// A: Target priorities and population policy.
class Strategy {
public:
    Strategy(const Memory& memory, const unswbc::Controller& controller, const Navigation& navigation, const unswbc::Game& game)
        : mem_(memory), ct_(controller), nav_(navigation), game_(game) {}
    Target choose_target() const;
    SplitEvaluation split_score(const Cells& occupied) const;
    std::optional<int> expansion_split_size() const;
    std::optional<int> emergency_split_size() const;
private:
    bool has_adjacent_pearl() const;
    const Memory& mem_;
    const unswbc::Controller& ct_;
    const Navigation& nav_;
    const unswbc::Game& game_;
};
}
