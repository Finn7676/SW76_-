#pragma once
#include "memory.hpp"
namespace mybot {
// B: Immediate legality and the original visible-pocket heuristic.
class Safety {
public:
    Safety(const Memory& memory, const unswbc::Controller& controller) : mem_(memory), ct_(controller) {}
    int neighbor_open_count(Cell p, const Cells& occupied) const;
    std::pair<Cells, bool> visible_pocket(Cell start, const Cells& blocked) const;
    bool is_visible_dead_end(Cell destination, const Cells& occupied) const;
    std::vector<Step> legal_steps() const;
    Options classify_steps(const Cells& occupied) const;
    Moves fallback_move() const;
private:
    const Memory& mem_;
    const unswbc::Controller& ct_;
};
}
