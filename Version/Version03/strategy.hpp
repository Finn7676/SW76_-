#pragma once
#include "navigation.hpp"
namespace mybot {
// A: Target priorities and population policy.
class Strategy {
public:
    Strategy(Memory& memory, unswbc::Controller& controller, const Navigation& navigation)
        : mem_(memory), ct_(controller), nav_(navigation) {}
    Target choose_target() const;
    int unit_cap() const;
    bool want_split(const Cells& occupied, const Path& pearl_path) const;
    bool normal_split();
    bool split_to_small();
private:
    Memory& mem_;
    unswbc::Controller& ct_;
    const Navigation& nav_;
};
}
