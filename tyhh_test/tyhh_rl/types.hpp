#pragma once
#include "helper.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace champion {
using Position = unswbc::Position;
using Direction = unswbc::Direction;
inline constexpr int DX[4] = {0, 1, 0, -1};
inline constexpr int DY[4] = {-1, 0, 1, 0};
inline constexpr char LETTER[4] = {'N','E','S','W'};
inline int dirIndex(Direction d) {
    for (int i = 0; i < 4; ++i) if (LETTER[i] == d.value) return i;
    return 0;
}
enum class TargetKind { Pearl, Spawn, Explore, Escape, Enemy };
struct Target {
    Position position{-1,-1};
    TargetKind kind = TargetKind::Explore;
    double value = 0;
    bool valid() const { return position.x >= 0 && position.y >= 0; }
};
struct Enemy { int id = -1; Position position; int visibleLength = 0; };
enum class ActionKind { Move, Split };
struct Action {
    ActionKind kind = ActionKind::Move;
    std::vector<int> directions;
    int childSize = 0;
    double score = -1e20;
    bool safe = false;
    static Action move(int d) { Action a; a.directions = {d}; return a; }
    static Action split(int n) { Action a; a.kind = ActionKind::Split; a.childSize = n; a.safe = true; return a; }
    void execute(unswbc::Controller& ct) const {
        if (kind == ActionKind::Split) { ct.do_split(childSize); return; }
        std::vector<Direction> ds;
        for (int d : directions) ds.emplace_back(LETTER[d]);
        if (ds.empty()) ds.emplace_back(ct.get_dir());
        ct.make_moves(ds);
    }
};
enum class Role { Flagship, Forager };
struct Config {
    static constexpr int beamWidth = 36;
    static constexpr int horizon = 10;
    static constexpr int maxSprint = 3;
    static constexpr int floodLimit = 512;
    // Match the bounded population strategy used by the stronger rule-based
    // variants. The policy still decides targets; splitting remains guarded
    // by visible food, space, and escape checks in Strategy::shouldSplit.
    static constexpr int maxPopulation = 8;
    static constexpr int reserveSplitLength = 10;
    static constexpr int splitStopRound = 320;
    static constexpr bool sonarEnabled = false;
};
}
