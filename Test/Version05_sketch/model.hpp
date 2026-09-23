#pragma once

#include "helper.hpp"

#include <algorithm>
#include <array>

namespace xxqz {

using unswbc::Direction;
using unswbc::Position;
using unswbc::PositionHash;

inline constexpr int EARLY_ROUND = 220;
inline constexpr int LATE_ROUND = 350;
inline constexpr int MISSILE_MAX_LENGTH = 6;
inline constexpr int QUEEN_LENGTH = 12;
inline constexpr int LONG_ENEMY = 7;

inline std::array<Direction, 4> const& directions() {
    static std::array<Direction, 4> dirs = Direction::get_direction_list();
    return dirs;
}

inline int dir_index(Direction direction) {
    switch (direction.value) {
    case Direction::NORTH: return 0;
    case Direction::EAST: return 1;
    case Direction::SOUTH: return 2;
    default: return 3;
    }
}

inline int wrap_delta(int from, int to, int size) {
    int delta = (to - from) % size;
    if (delta < 0)
        delta += size;
    if (delta > size - delta)
        return delta - size;
    return delta;
}

inline int torus_dist(Position a, Position b, int width, int height) {
    return std::abs(wrap_delta(a.x, b.x, width)) + std::abs(wrap_delta(a.y, b.y, height));
}

inline int population_target(int area) {
    if (area <= 160)
        return 8;
    if (area <= 500)
        return 12;
    if (area <= 1400)
        return 16;
    return 20;
}

inline bool worth_killing(int visible_segments, int my_length) {
    return visible_segments >= std::max(LONG_ENEMY, my_length + 4);
}

}  // namespace xxqz
