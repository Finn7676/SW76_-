#pragma once

#include "observation.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace xxqz {

using unswbc::Controller;
using unswbc::Game;

struct StepInfo {
    Direction direction = Direction::NORTH;
    Position dest;
    bool pearl = false;
    int eta = -1;
    bool hard_trap = false;
    bool soft_trap = false;
    int pocket = 0;
};

struct Enemy {
    int id = 0;
    bool has_head = false;
    Position head;
    int visible = 0;
};

struct RegionInfo {
    int size = 0;
    bool touches_unknown = false;
    int min_degree = 4;
    int max_degree = 0;
};

struct TopologyCache {
    bool ready = false;
    std::unordered_map<Position, int, PositionHash> component_id;
    std::vector<RegionInfo> regions;
};

struct EnemyThreat {
    int enemy_id = 0;
    Position head;
    int visible_length = 0;
    int escapes = 4;
    std::uint8_t reachable_mask = 0;
    std::uint8_t forward_mask = 0;
};

// Counts grouped by enemy escape count, so scoring keeps its original config
// and dragon-length semantics without iterating every enemy per candidate.
struct ThreatBucket {
    std::array<int, 5> reachable{};
    std::array<int, 5> forward{};
};

struct ThreatCache {
    bool ready = false;
    std::uint8_t candidate_mask = 0;
    std::vector<EnemyThreat> enemies;
    std::array<ThreatBucket, 4> candidates{};
};

struct View {
    Position head;
    std::vector<Position> body;
    bool body_complete = false;
    std::vector<StepInfo> safe;
    std::vector<Enemy> enemies;
    std::vector<Position> ally_heads;
    std::unordered_map<Position, Cell, PositionHash> snap;
    TopologyCache topology;
    // Memoized on first tactical use; facts are tied to this immutable snapshot.
    mutable ThreatCache threats;
    int w = 0;
    int h = 0;
};

}  // namespace xxqz
