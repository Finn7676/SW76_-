#pragma once

#include "helper.hpp"
#include <algorithm>
#include <climits>

namespace bot {

#ifndef FEAT_V3_SCOUT
#define FEAT_V3_SCOUT 1
#endif
#ifndef FEAT_V3_PORTAL_CLAIM
#define FEAT_V3_PORTAL_CLAIM 1
#endif
#ifndef FEAT_V3_RELAY
#define FEAT_V3_RELAY 1
#endif
#ifndef FEAT_V3_EXPLORED_BONUS
#define FEAT_V3_EXPLORED_BONUS 1
#endif
#ifndef FEAT_V3_SHORT_AVOID
#define FEAT_V3_SHORT_AVOID 1
#endif
#ifndef FEAT_V3_PEARL_ECONOMY
#define FEAT_V3_PEARL_ECONOMY 1
#endif
#ifndef FEAT_V3_DEATH_PEARLS
#define FEAT_V3_DEATH_PEARLS 1
#endif
#ifndef FEAT_V3_TACTICAL_SPLIT
#define FEAT_V3_TACTICAL_SPLIT 1
#endif
// Explicit macro defaults for the V3 planner's state arbitration and mission handling.
#ifndef FEAT_V32_SCOUT_MISSION
#define FEAT_V32_SCOUT_MISSION 1
#endif
#ifndef FEAT_V32_MISSION_EXPANSION_ARB
#define FEAT_V32_MISSION_EXPANSION_ARB 1
#endif
#ifndef FEAT_V32_SEVERE_BLOCKAGE
#define FEAT_V32_SEVERE_BLOCKAGE 1
#endif
#ifndef FEAT_V32_ENEMY_RELEVANCE
#define FEAT_V32_ENEMY_RELEVANCE 1
#endif
#ifndef V3_SACRIFICE_QUIET_ROUNDS
#define V3_SACRIFICE_QUIET_ROUNDS 18
#endif
#ifndef V3_SACRIFICE_ALLY_QUIET_ROUNDS
#define V3_SACRIFICE_ALLY_QUIET_ROUNDS 7
#endif
#ifndef V3_QUIET_REPORT_MAX_AGE
#define V3_QUIET_REPORT_MAX_AGE 3
#endif
#ifndef V3_SACRIFICE_START_ROUND
#define V3_SACRIFICE_START_ROUND 400
#endif
#ifndef V3_SACRIFICE_MIN_ALLY_LENGTH
#define V3_SACRIFICE_MIN_ALLY_LENGTH 8
#endif
#ifndef V4_DONATION_MIDDLE_ROUND
#define V4_DONATION_MIDDLE_ROUND 430
#endif
#ifndef V4_DONATION_FINAL_ROUND
#define V4_DONATION_FINAL_ROUND 460
#endif
#ifndef V4_DONATION_REPORT_AGE
#define V4_DONATION_REPORT_AGE 3
#endif
// V5 local feeding. Legacy quiet/reserve constants above still govern expansion.
#ifndef V5_FEED_START_ROUND
#define V5_FEED_START_ROUND 400
#endif
// Short-headshot and resource-collection thresholds are independent of feeding eligibility.
#ifndef V5_FEED_SHORT_LENGTH
#define V5_FEED_SHORT_LENGTH 8
#endif
#ifndef V5_FEED_RESOURCE_COLLECTOR_LENGTH
#define V5_FEED_RESOURCE_COLLECTOR_LENGTH 12
#endif
#ifndef V5_FEED_MIN_LENGTH_GAIN
#define V5_FEED_MIN_LENGTH_GAIN 2
#endif
#ifndef V5_FEED_LARGE_DONOR_LENGTH
#define V5_FEED_LARGE_DONOR_LENGTH 18
#endif
#ifndef V5_FEED_LARGE_DONOR_POPULATION
#define V5_FEED_LARGE_DONOR_POPULATION 20
#endif
#ifndef V5_FEED_TARGET_TIMEOUT
#define V5_FEED_TARGET_TIMEOUT 4
#endif
#ifndef V5_FEED_MAX_CHASE_ROUNDS
#define V5_FEED_MAX_CHASE_ROUNDS 8
#endif
#ifndef V5_FEED_NO_PROGRESS_ROUNDS
#define V5_FEED_NO_PROGRESS_ROUNDS 3
#endif
#ifndef V5_FEED_CHASE_COOLDOWN
#define V5_FEED_CHASE_COOLDOWN 4
#endif
#ifndef V5_FEED_SWITCH_LENGTH_GAIN
#define V5_FEED_SWITCH_LENGTH_GAIN 4
#endif
#ifndef V5_FEED_PICKUP_STEPS
#define V5_FEED_PICKUP_STEPS 2
#endif
#ifndef V5_FEED_END_MARGIN
#define V5_FEED_END_MARGIN 2
#endif
#ifndef V5_FEED_ROUTE_DEPTH
#define V5_FEED_ROUTE_DEPTH 8
#endif
#ifndef V5_FEED_ROUTE_NODES
#define V5_FEED_ROUTE_NODES 256
#endif
#ifndef V5_FEED_RESOURCE_DEPTH
#define V5_FEED_RESOURCE_DEPTH 4
#endif
#ifndef V5_FEED_CLUSTER_SIZE
#define V5_FEED_CLUSTER_SIZE 3
#endif
#ifndef V3_SCOUT_MAX_PATH
#define V3_SCOUT_MAX_PATH 6
#endif
#ifndef V3_PEARL_MAX_PATH
#define V3_PEARL_MAX_PATH 4
#endif
#ifndef V3_PEARL_CLUSTER_DETOUR
#define V3_PEARL_CLUSTER_DETOUR 3
#endif
#ifndef V3_EXPLORED_BONUS
#define V3_EXPLORED_BONUS 18
#endif

#ifndef FEAT_SCOUT_ROLE
#define FEAT_SCOUT_ROLE 1
#endif
#ifndef SCOUT_END_ROUND
#define SCOUT_END_ROUND 400
#endif
#ifndef FEAT_PORTAL_EXPLORE
#define FEAT_PORTAL_EXPLORE 1
#endif
#ifndef FEAT_SONAR
#define FEAT_SONAR 1
#endif
#ifndef FEAT_SPRINT_RAM
#define FEAT_SPRINT_RAM 1
#endif
#ifndef FEAT_REGION_DENSITY
#define FEAT_REGION_DENSITY 1
#endif
#ifndef V5_DENSITY_CELLS_PER_DRAGON
#define V5_DENSITY_CELLS_PER_DRAGON 10
#endif

#ifndef FEAT_SPRINT_ESCAPE
#define FEAT_SPRINT_ESCAPE 1
#endif
#ifndef FEAT_SPRINT_EAT
#define FEAT_SPRINT_EAT 1
#endif
#ifndef FEAT_TIMED_OCC
#define FEAT_TIMED_OCC 1
#endif
#ifndef FEAT_RAM_FIRST
#define FEAT_RAM_FIRST 1
#endif
#ifndef FEAT_SACRIFICE
#define FEAT_SACRIFICE 1
#endif
#ifndef FEAT_CHAMPION
#define FEAT_CHAMPION 1
#endif
#ifndef FEAT_TRAP
#define FEAT_TRAP 1
#endif
#ifndef FEAT_FEAST
#define FEAT_FEAST 1
#endif
#ifndef FEAT_HUNT_SIGHTINGS
#define FEAT_HUNT_SIGHTINGS 1
#endif
#ifndef CH_START
#define CH_START 250
#endif
#ifndef CH_MOD
#define CH_MOD 8
#endif
#ifndef CH_LEN
#define CH_LEN 12
#endif
#ifndef SEED
#define SEED 0
#endif
#ifndef FEAT_REVERSE_SPLIT
#define FEAT_REVERSE_SPLIT 1
#endif

using namespace unswbc;

constexpr int INF = 1 << 28;
constexpr int DX[4] = {0, 1, 0, -1};
constexpr int DY[4] = {-1, 0, 1, 0};
constexpr char DCH[4] = {'N', 'E', 'S', 'W'};
enum : uint8_t { E_UNKNOWN = 0, E_OPEN = 1, E_KELP = 2, E_PORTAL = 3 };

// Queen identity is permanent and independent of role switches or round number.
inline bool isQueenId(int id) { return id == 0 || id == 1; }
struct MovementRules {
    static int freeSteps(int initialLength) { return (initialLength + 3) / 4; }
    static bool paidStep(int initialLength, int zeroBasedStep) {
        return zeroBasedStep >= freeSteps(initialLength);
    }
    static int cost(int initialLength, int steps) {
        return std::max(0, steps - freeSteps(initialLength));
    }
};

struct GameConfig {
    int width, height, cells, targetPopulation;
    explicit GameConfig(Game const &game) : GameConfig(game.width, game.height, game.get_unit_limit()) {}
    GameConfig(int width, int height, int unitLimit = 64)
        : width(width), height(height), cells(width * height), targetPopulation(std::min(62, unitLimit - 2)) {}
};
struct Part {
    int c, id, facing;
    bool mine, head;
};
struct TurnState {
    int round = 0, length = 0, population = 0, myId = 0, here = 0;
    Team::Value myTeam = Team::A;
    std::vector<Part> parts;
    std::vector<int> enemyHeads, enemyHeadLen, enemyHeadExactLen;
    std::vector<int> friendHeads, friendHeadLen, friendHeadExactLen, friendParts, visPearls;
    std::vector<std::pair<int, int>> idCount;
    // Tile order is part of sonar tie-breaking. Observer fills this once per turn.
    std::array<int, Constants::VISION_SIZE * Constants::VISION_SIZE> visibleCells{};
    int visibleCount = 0;
    void read(Controller const &ct, Game const &game, GameConfig const &config) {
        round = game.get_round_num();
        length = ct.get_length();
        population = ct.get_unit_count();
        myId = ct.get_id();
        myTeam = ct.get_team().value;
        here = ct.get_position().y * config.width + ct.get_position().x;
    }
    bool isQueen() const { return isQueenId(myId); }
    bool queenDevelopment() const { return isQueen() && round >= 0 && round < 150; }
    bool mayAttackQueen(int enemyLength) const {
        // Use our current length, before sprint costs, and an observed lower bound.
        return !isQueen() && population > 1 && enemyLength >= Constants::MIN_SIZE &&
               (round >= 400 || length <= enemyLength + 2);
    }
    int countOf(int id) const {
        for (auto const &p : idCount)
            if (p.first == id) return p.second;
        return 0;
    }
};

inline int dirIndex(Direction d) {
    switch (d.value) {
    case Direction::NORTH:
        return 0;
    case Direction::EAST:
        return 1;
    case Direction::SOUTH:
        return 2;
    default:
        return 3;
    }
}

// Value only: policies cannot output actions or mutate execution state.
enum class ActionType : uint8_t { NONE, MOVE, SPRINT, SPLIT, SUICIDE };
struct Action {
    ActionType type = ActionType::NONE;
    std::array<uint8_t, 8> dirs{};
    uint8_t steps = 0;
    int splitSize = 0;
    explicit operator bool() const { return type != ActionType::NONE; }
    static Action move(int d) { return {ActionType::MOVE, {uint8_t(d)}, 1, 0}; }
    static Action sprint2(int d1, int d2) { return {ActionType::SPRINT, {uint8_t(d1), uint8_t(d2)}, 2, 0}; }
    static Action split(int n) { return {ActionType::SPLIT, {}, 0, n}; }
    static Action suicide() { return {ActionType::SUICIDE, {}, 0, 0}; }
};

} // namespace bot
