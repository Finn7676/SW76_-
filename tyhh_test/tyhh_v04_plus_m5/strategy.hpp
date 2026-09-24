#pragma once

namespace xxqz {

// Independent switches for replay comparisons / ablation experiments.
struct StrategyConfig {
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
