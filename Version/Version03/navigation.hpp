#pragma once
#include "memory.hpp"
#include "geometry.hpp"
namespace mybot {
// B: Local routing and movement simulation, without issuing controller actions.
class Navigation {
public:
    explicit Navigation(const Memory& memory) : mem_(memory) {}
    Simulation simulate(const Moves& moves, const Cells& occupied_others) const;
    Path local_bfs(const Cells& occupied_others, const Cells& goals, bool allow_unseen) const;
    std::optional<Cell> nearest_goal(const Cells& goals) const;
    Moves heading_towards(Cell target) const;
    int distance(Cell a, Cell b) const;
private:
    const Memory& mem_;
};
}
