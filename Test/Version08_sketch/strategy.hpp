#pragma once

#ifndef XXQZ_ENABLE_STATS
#define XXQZ_ENABLE_STATS 0
#endif

#ifndef XXQZ_UNIFIED_PERCEPTION
#define XXQZ_UNIFIED_PERCEPTION 1
#endif

#ifndef XXQZ_COMPONENT_CACHE
#define XXQZ_COMPONENT_CACHE 1
#endif
#ifndef XXQZ_THREAT_CACHE
#define XXQZ_THREAT_CACHE 1
#endif

#ifndef XXQZ_V08
#define XXQZ_V08 1
#endif

#ifndef XXQZ_EXPANSION
#define XXQZ_EXPANSION XXQZ_V08
#endif
#ifndef XXQZ_SECTOR_EXPLORATION
#define XXQZ_SECTOR_EXPLORATION XXQZ_V08
#endif
#ifndef XXQZ_NAV_MEMORY
#define XXQZ_NAV_MEMORY XXQZ_V08
#endif
#ifndef XXQZ_ANTI_OSCILLATION
#define XXQZ_ANTI_OSCILLATION XXQZ_V08
#endif
#ifndef XXQZ_RETARGETING
#define XXQZ_RETARGETING XXQZ_V08
#endif
#ifndef XXQZ_ESCAPE_MODE
#define XXQZ_ESCAPE_MODE XXQZ_V08
#endif

namespace xxqz {

// Independent switches for replay comparisons / ablation experiments.
struct StrategyConfig {
    // Compile with -DXXQZ_ENABLE_STATS=1 for diagnostics; default builds have no counter hooks.
    bool debug_stats = XXQZ_ENABLE_STATS != 0;
    bool unified_perception = XXQZ_UNIFIED_PERCEPTION != 0;
    bool component_cache = XXQZ_COMPONENT_CACHE != 0;
    bool threat_cache = XXQZ_THREAT_CACHE != 0;
    bool expansion = XXQZ_EXPANSION != 0;
    bool sector_exploration = XXQZ_SECTOR_EXPLORATION != 0;
    bool nav_memory = XXQZ_NAV_MEMORY != 0;
    bool anti_oscillation = XXQZ_ANTI_OSCILLATION != 0;
    bool retargeting = XXQZ_RETARGETING != 0;
    bool escape_mode = XXQZ_ESCAPE_MODE != 0;
    int expansion_rounds = 80;
    int expansion_min_area = 400;
    int stagnation_rounds = 6;
    int retarget_rounds = 10;
    int target_cooldown = 8;
    int avoid_rounds = 2;
    bool routing = true;
    bool sonar = true;
    bool head_risk = true;
    bool corner_pressure = true;
    bool short_trade = true;
    int route_depth = 24;
    int route_nodes = 320;
    int route_bonus = 140;
    int own_pearl_ttl = 32;
    int sonar_ttl = 8;
    int head_risk_area = 200;
    int hunt_area = 300;
    int adjacent_risk = 70;
    int forward_risk = 180;
    int corner_bonus = 70;
};

inline constexpr StrategyConfig strategy{};

}  // namespace xxqz
