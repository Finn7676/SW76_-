#pragma once
#include "types.hpp"
namespace mybot {
int torus_delta(int a, int b, int size);
int torus_distance(Cell a, Cell b, int width, int height);
Moves heading_towards(Cell head, Cell target, int width, int height);
}
