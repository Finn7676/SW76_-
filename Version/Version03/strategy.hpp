#pragma once
#include "navigation.hpp"
namespace mybot {
// A: Target priorities and population policy.
class Strategy {
public:
    Strategy(const Memory& memory, const unswbc::Controller& controller, const Navigation& navigation)
        : mem_(memory), ct_(controller), nav_(navigation) {}
    Target choose_target() const;
    int unit_cap() const;
    bool want_split(const Cells& occupied, const Path& pearl_path) const;
    std::optional<int> normal_split_size() const;
    std::optional<int> emergency_split_size() const;
private:
    const Memory& mem_;
    const unswbc::Controller& ct_;
    const Navigation& nav_;
};
}
