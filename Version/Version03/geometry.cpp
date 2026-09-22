#include "geometry.hpp"
#include <cstdlib>
namespace mybot {
int torus_delta(int a, int b, int size) {
    int delta = ((b - a) % size + size) % size;
    if (delta > size - delta) delta -= size;
    return delta;
}
int torus_distance(Cell a, Cell b, int width, int height) {
    return std::abs(torus_delta(a.first, b.first, width))
         + std::abs(torus_delta(a.second, b.second, height));
}
Moves heading_towards(Cell head, Cell target, int width, int height) {
    const int dx = torus_delta(head.first, target.first, width);
    const int dy = torus_delta(head.second, target.second, height);
    Moves result;
    auto horizontal = [&] {
        if (dx > 0) result.emplace_back(Direction::EAST);
        else if (dx < 0) result.emplace_back(Direction::WEST);
    };
    auto vertical = [&] {
        if (dy > 0) result.emplace_back(Direction::SOUTH);
        else if (dy < 0) result.emplace_back(Direction::NORTH);
    };
    if (std::abs(dx) >= std::abs(dy)) { horizontal(); vertical(); }
    else { vertical(); horizontal(); }
    for (auto d : directions())
        if (std::find(result.begin(), result.end(), d) == result.end()) result.push_back(d);
    return result;
}
}
