#pragma once

#include "navigation.hpp"
#include "tactics.hpp"

namespace xxqz {

inline bool navigation_enabled(StrategyConfig const& c) {
    return c.routing && (c.expansion || c.sector_exploration || c.nav_memory
        || c.anti_oscillation || c.retargeting || c.escape_mode);
}

inline void observe_navigation(Controller const& ct, View const& view, int round,
                               NavigationState& nav, StrategyConfig const& config) {
    if (nav.dragon_id != ct.get_id() || round < nav.last_round) {
        nav = NavigationState{};
        nav.dragon_id = ct.get_id();
        // Fold high bits so teams with even/odd IDs can each use all four sectors.
        nav.sector = (ct.get_id() ^ (ct.get_id() >> 1)) & 3;
    }
    if (nav.last_round == round)
        return;
    if (nav.last_round >= 0 && round > nav.last_round + 1)
        nav.interrupt();
    nav.last_round = round;
    if (nav.target) {
        int distance = torus_dist(view.head, *nav.target, view.w, view.h);
        if (distance < nav.best_distance) {
            nav.best_distance = distance;
            nav.stagnant_rounds = 0;
            if (nav.escape_until >= round) {
                nav.escape_until = -1;
                nav.mode = NavMode::Route;
                record(&DebugStats::nav_escape_recoveries);
            }
        } else {
            ++nav.stagnant_rounds;
            record(&DebugStats::nav_stagnant_turns);
        }
    }
    if (nav.recent_count == static_cast<int>(nav.recent_heads.size())) {
        for (std::size_t i = 1; i < nav.recent_heads.size(); ++i)
            nav.recent_heads[i - 1] = nav.recent_heads[i];
        --nav.recent_count;
    }
    nav.recent_heads[nav.recent_count++] = view.head;
    int n = nav.recent_count;
    nav.oscillating = n >= 4 && nav.recent_heads[n-1] == nav.recent_heads[n-3]
        && nav.recent_heads[n-2] == nav.recent_heads[n-4];
    nav.oscillating = nav.oscillating || (n >= 6 && nav.recent_heads[n-1] == nav.recent_heads[n-5]
        && nav.recent_heads[n-2] == nav.recent_heads[n-6]);
    if (nav.oscillating)
        record(&DebugStats::nav_oscillations);
    if (nav.avoid_until < round)
        nav.temporarily_avoid.reset();
    if (config.anti_oscillation && n >= 2
        && (nav.oscillating || nav.stagnant_rounds >= config.stagnation_rounds)) {
        // In a four-step cycle, avoid the next cell from the previous lap,
        // not the occupied neck immediately behind the current head.
        Position repeated_next = n >= 6 && nav.recent_heads[n-1] == nav.recent_heads[n-5]
            && nav.recent_heads[n-2] == nav.recent_heads[n-6] ? nav.recent_heads[n-4] : nav.recent_heads[n-2];
        for (auto const& step : view.safe) {
            if (step.dest == repeated_next) {
                nav.temporarily_avoid = step.direction;
                nav.avoid_until = round + config.avoid_rounds;
                break;
            }
        }
    }
    if (nav.temporarily_avoid)
        record(&DebugStats::nav_avoid_turns);
}

inline bool navigation_target_valid(NavigationState const& nav, View const& view,
                                     Memory const& map, StrategyConfig const& config) {
    if (!nav.target || *nav.target == view.head)
        return false;
    Position pos = *nav.target;
    if (nav.target_kind == RouteKind::Explore)
        return !map.seen.count(pos);
    auto live = view.snap.find(pos);
    if (nav.target_kind == RouteKind::Soon) {
        auto eta = map.eta.find(pos);
        return eta != map.eta.end() && eta->second >= 0 && eta->second <= 10;
    }
    if (live != view.snap.end())
        return live->second.pearl && live->second.owner == Owner::Empty;
    if (map.pearls.count(pos))
        return true;
    auto shared = map.shared_pearls.find(pos);
    return config.sonar && shared != map.shared_pearls.end() && map.last_round >= shared->second
        && map.last_round - shared->second <= config.sonar_ttl;
}

inline int signed_wrap_delta(int from, int to, int size) {
    int delta = (to - from + size) % size;
    if (delta > size / 2) delta -= size;
    return delta;
}

inline int exploration_value(Position pos, View const& view, Memory const& map,
                               NavigationState const& nav, StrategyConfig const& config) {
    int information = 0;
    // Nine spaced samples approximate the next 7x7 observation footprint.
    for (int x : {-2, 0, 2}) for (int y : {-2, 0, 2}) {
        record(&DebugStats::nav_information_samples);
        Position sample{(pos.x+x+view.w)%view.w, (pos.y+y+view.h)%view.h};
        if (!map.seen.count(sample)) ++information;
    }
    int value = information * 6;
    if (config.sector_exploration && map.last_round <= config.expansion_rounds
        && view.w*view.h >= config.expansion_min_area) {
        int dx = signed_wrap_delta(view.head.x,pos.x,view.w);
        int dy = signed_wrap_delta(view.head.y,pos.y,view.h);
        auto [sx, sy] = directions()[static_cast<std::size_t>(nav.sector)].get_offset();
        int forward = dx*sx + dy*sy;
        int sideways = std::abs(dx*sy - dy*sx);
        value += std::clamp(forward,-4,4)*8 - sideways*2;
    }
    for (Position ally : view.ally_heads) {
        int distance = torus_dist(pos,ally,view.w,view.h);
        if (distance <= 6) value -= (7-distance)*8;
    }
    return value;
}

inline int navigation_route_tier(Route const& route, bool expanding) {
    if (route.kind == RouteKind::VisiblePearl && route.distance <= 3)
        return 0;
    if (expanding && route.kind == RouteKind::Explore)
        return 1;
    return 2 + static_cast<int>(route.kind);
}

inline std::optional<Route> find_navigation_route(Controller const& ct, View const& view, Memory const& map,
                                                  std::vector<StepInfo> const& allowed_first,
                                                  NavigationState& nav, StrategyConfig const& config) {
    record(&DebugStats::route_calls);
    if (allowed_first.empty() || config.route_nodes < 2 || config.route_depth < 1)
        return std::nullopt;
    int round = map.last_round;
    bool quiet = std::none_of(view.enemies.begin(),view.enemies.end(),[](Enemy const& e) { return e.has_head; });
    bool expanding = config.expansion && quiet && view.w*view.h >= config.expansion_min_area
        && round <= config.expansion_rounds && ct.get_unit_count() > 1
        && ct.get_length() <= 8 && ct.get_id()%3 == 0
        && map.seen.size()*5 < static_cast<std::size_t>(view.w*view.h)*3;
    if (nav.target && !navigation_target_valid(nav,view,map,config)) {
        nav.clear_target();
        record(&DebugStats::nav_stale_targets);
    }
    bool stuck = nav.stagnant_rounds >= config.retarget_rounds || nav.failed_routes >= 3;
    if (config.retargeting && nav.target && stuck) {
        nav.rejected_target = nav.target;
        nav.reject_until = round + config.target_cooldown;
        nav.clear_target();
        nav.mode = NavMode::Retarget;
        if (config.escape_mode) nav.escape_until = round + 3;
        record(&DebugStats::nav_retargets);
    }
    std::unordered_map<Position, RouteKind, PositionHash> goals;
    auto add = [&](Position pos, RouteKind kind) {
        if (pos == view.head || (nav.rejected_target && pos == *nav.rejected_target && round <= nav.reject_until))
            return;
        auto [it,inserted] = goals.emplace(pos,kind);
        if (!inserted && static_cast<int>(kind)<static_cast<int>(it->second)) it->second=kind;
    };
    for (auto const& [pos,cell] : view.snap) {
        record(&DebugStats::goal_tiles_scanned);
        if (cell.pearl && cell.owner == Owner::Empty) add(pos,RouteKind::VisiblePearl);
    }
    for (Position pos : map.pearls) add(pos,RouteKind::RememberedPearl);
    if (config.sonar)
        for (auto const& [pos,time] : map.shared_pearls)
            if (round >= time && round-time <= config.sonar_ttl) add(pos,RouteKind::SharedPearl);
    for (Position pos : map.soon()) add(pos,RouteKind::Soon);
    for (Position pos : map.frontier()) add(pos,RouteKind::Explore);
    // Previous targets may inform anti-oscillation progress, but cannot add goals when NavMemory is off.
    if (config.nav_memory && nav.target && navigation_target_valid(nav,view,map,config)) add(*nav.target,nav.target_kind);

    std::array<bool,4> allowed{};
    bool alternative = false;
    int avoided_risk = -1000000000;
    if (nav.temporarily_avoid) {
        for (auto const& step : allowed_first)
            if (step.direction == *nav.temporarily_avoid)
                avoided_risk = tactical_score(ct,view,step,config);
        for (auto const& step : allowed_first)
            if (step.direction != *nav.temporarily_avoid && tactical_score(ct,view,step,config) >= avoided_risk)
                alternative = true;
    }
    for (auto const& step : allowed_first) {
        if (config.anti_oscillation && alternative && nav.temporarily_avoid && round <= nav.avoid_until
            && step.direction == *nav.temporarily_avoid && !step.pearl)
            continue;
        allowed[dir_index(step.direction)] = true;
    }
    struct Node { Position pos; Direction first; int depth; };
    std::deque<Node> queue{{view.head,Direction::NORTH,0}};
    std::unordered_set<Position,PositionHash> seen{view.head};
    record(&DebugStats::route_nodes);
    std::optional<Route> best, retained;
    int best_tier = 99, best_score = -1000000000;
    while (!queue.empty()) {
        Node node=queue.front();queue.pop_front();
        record(&DebugStats::route_expanded);
        if (node.depth >= config.route_depth) continue;
        for (int i=0;i<4;++i) {
            if ((node.depth==0 && !allowed[i]) || !route_edge_open(node.pos,i,view,map)) continue;
            Position dest=node.pos.add_dir(directions()[i]);
            auto live=view.snap.find(dest);
            if ((live!=view.snap.end() && live->second.owner!=Owner::Empty) || seen.count(dest)) continue;
            if (static_cast<int>(seen.size())>=config.route_nodes) continue;
            seen.insert(dest);record(&DebugStats::route_nodes);
            Direction first=node.depth==0?directions()[i]:node.first;
            int distance=node.depth+1;
            if (auto goal=goals.find(dest);goal!=goals.end()) {
                Route candidate{dest,first,distance,goal->second};
                int tier=navigation_route_tier(candidate,expanding);
                int score=-distance*12;
                if (candidate.kind==RouteKind::Explore)
                    score+=exploration_value(dest,view,map,nav,config);
                if (!best || tier<best_tier || (tier==best_tier && score>best_score)) {
                    best=candidate;best_tier=tier;best_score=score;
                }
                if (nav.target && dest==*nav.target) retained=candidate;
                // Immediate pearl priority is absolute and BFS already found the closest.
                if (tier==0) { queue.clear(); break; }
            }
            queue.push_back(Node{dest,first,distance});
        }
    }
    if (config.nav_memory && retained && best && navigation_route_tier(*retained,expanding)<=best_tier
        && retained->distance<=best->distance+(retained->kind==RouteKind::Explore?4:1) && (best_tier!=0 || retained->distance<=best->distance)) {
        best=retained;
        record(&DebugStats::nav_retained);
    }
    if (!best) {
        ++nav.failed_routes;
        record(&DebugStats::nav_route_failures);
        return std::nullopt;
    }
    nav.failed_routes=0;
    if (!nav.target || *nav.target!=best->goal) {
        nav.target=best->goal;
        nav.best_distance=torus_dist(view.head,best->goal,view.w,view.h);
        nav.stagnant_rounds=0;
    }
    nav.target_kind=best->kind;
    nav.purpose=best->kind==RouteKind::Explore?NavPurpose::Explore:NavPurpose::Pearl;
    if (nav.escape_until < round)
        nav.mode=expanding && best->kind==RouteKind::Explore?NavMode::Expand:NavMode::Route;
    return best;
}

inline int navigation_step_score(Controller const& ct, View const& view, StepInfo const& step,
                                  NavigationState const& nav, int round, bool escaping,
                                  StrategyConfig const& config) {
    int score=0;
    if (config.anti_oscillation && nav.temporarily_avoid && round<=nav.avoid_until
        && step.direction==*nav.temporarily_avoid && !step.pearl && view.safe.size()>1) {
        int current_risk=tactical_score(ct,view,step,config);
        for(auto const& other:view.safe) {
            if(other.direction!=step.direction && other.hard_trap==step.hard_trap
                && other.soft_trap==step.soft_trap && tactical_score(ct,view,other,config)>=current_risk) {
                score-=180;
                break;
            }
        }
    }
    if (escaping) {
        score+=std::min(step.pocket,64)*12;
        auto cell=view.snap.find(step.dest);
        if (cell!=view.snap.end()) for(int i=0;i<4;++i) {
            if (cell->second.edges[i]!=EdgeType::EMPTY) continue;
            auto next=view.snap.find(step.dest.add_dir(directions()[i]));
            if (next==view.snap.end() || next->second.owner==Owner::Empty) score+=35;
        }
        score+=std::min(0,tactical_score(ct,view,step,config))*2;
    }
    return score;
}

}  // namespace xxqz
