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

namespace xxqz {

// Independent switches for replay comparisons / ablation experiments.
struct StrategyConfig {
    // Compile with -DXXQZ_ENABLE_STATS=1 for diagnostics; default builds have no counter hooks.
    bool debug_stats = XXQZ_ENABLE_STATS != 0;
    bool unified_perception = XXQZ_UNIFIED_PERCEPTION != 0;
    bool component_cache = XXQZ_COMPONENT_CACHE != 0;
    bool threat_cache = XXQZ_THREAT_CACHE != 0;
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
