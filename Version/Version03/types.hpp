#pragma once
#include "helper.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace mybot {
using Cell = std::pair<int, int>;
using Cells = std::set<Cell>;
using Direction = unswbc::Direction;
using Moves = std::vector<Direction>;
using Path = std::optional<Moves>; // nullopt: no route; empty vector: already at goal.
inline Cell cell(unswbc::Position p) { return {p.x, p.y}; }
inline unswbc::Position position(Cell p) { return {p.first, p.second}; }
inline int direction_index(Direction d) {
    switch (d.value) {
    case Direction::NORTH: return 0;
    case Direction::EAST: return 1;
    case Direction::SOUTH: return 2;
    default: return 3;
    }
}
inline bool contains(const std::vector<Cell>& body, Cell p) {
    return std::find(body.begin(), body.end(), p) != body.end();
}
inline Moves directions() {
    const auto ds = Direction::get_direction_list();
    return {ds.begin(), ds.end()};
}
enum class TransitKind { Empty, Kelp, Portal, UnknownPortal, Unknown };
struct Transition {
    TransitKind kind = TransitKind::Unknown;
    std::optional<Cell> destination;
};
struct Step {
    Direction direction;
    Cell destination;
    TransitKind kind;
};
struct Simulation {
    bool ok = false;
    std::vector<Cell> body;
    int eats = 0;
    bool unknown = false;
};
struct Target {
    Cells goals;
    std::optional<Cell> position;
    std::string reason = "wander";
};
struct Options { std::vector<Step> open, dead; };
} // namespace mybot
