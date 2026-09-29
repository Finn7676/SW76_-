#pragma once
#include "topology.hpp"
#include "occupancy.hpp"
namespace bot {
// Direct travel and wall following share the same inward/outward route metric.
class BugNav {
    GameConfig const &config;
    TurnState const &turn;
    Topology const &topology;
    OccupancyModel const &occupancy;
    int goal = -1, hitDistance = INF, bestDistance = INF;
    int heading = -1, lastCell = -1, expectedCell = -1;
    int wallSteps = 0, repeats = 0, staleSteps = 0, side = 1, route = -1;
    bool wrap = true, following = false, seamBlocked = false;
  public:
    BugNav(GameConfig const &config, TurnState const &turn, Topology const &topology,
           OccupancyModel const &occupancy)
        : config(config), turn(turn), topology(topology), occupancy(occupancy) {}
    void reset() {
        goal = -1; hitDistance = bestDistance = INF;
        heading = lastCell = expectedCell = -1;
        wallSteps = repeats = staleSteps = 0; route = -1;
        following = seamBlocked = false;
    }
    bool boundaryStalled() const { return following && seamBlocked && staleSteps >= 12; }
    int direction(int target, bool wrapped = true, int tieBreaker = 0,
                  int routeDir = -1, unsigned safeFirst = 15) {
        if (target < 0 || target >= config.cells) return -1;
        if (goal != target || wrap != wrapped || route != routeDir) {
            reset(); goal = target; wrap = wrapped; route = routeDir;
            side = (tieBreaker & 1) ? -1 : 1;
        }
        // A detour or the final guard can move us off the wall we were following.
        if (expectedCell >= 0 && turn.here != expectedCell && turn.here != lastCell) {
            following = false; wallSteps = repeats = staleSteps = 0;
        }
        int distance = targetDistance(turn.here);
        auto preferred = preferredDirections();
        int direct = -1;
        for (int d : preferred) {
            int q = topology.step(turn.here, d);
            if (!traversable(d, q, safeFirst)) { noteSeam(d, distance); continue; }
            if (targetDistance(q) < distance) { direct = d; break; }
        }
        if (following && direct >= 0 && distance < hitDistance) {
            following = false; wallSteps = repeats = staleSteps = 0; seamBlocked = false;
        }
        if (direct >= 0 && !following) return select(direct);
        if (!following) {
            following = true; hitDistance = bestDistance = distance;
            heading = preferred[0]; lastCell = turn.here;
            wallSteps = repeats = staleSteps = 0;
        }
        repeats = turn.here == lastCell ? repeats + 1 : 0;
        ++wallSteps;
        if (distance < bestDistance) { bestDistance = distance; staleSteps = 0; }
        else ++staleSteps;
        if (repeats >= 2 || wallSteps >= 32) {
            side = -side; hitDistance = distance; wallSteps = repeats = 0;
        }
        int order[4] = {(heading + side + 4) & 3, heading,
                        (heading - side + 4) & 3, heading ^ 2};
        for (int d : order) {
            int q = topology.step(turn.here, d);
            if (traversable(d, q, safeFirst)) { heading = d; return select(d); }
            noteSeam(d, distance);
        }
        return -1;
    }
    int routeDistance(int c, int target, bool wrapped = true, int routeDir = -1) const {
        int dx = topology.cx(target) - topology.cx(c), dy = topology.cy(target) - topology.cy(c);
        int ax = std::abs(dx), ay = std::abs(dy);
        if (wrapped) {
            ax = std::min(ax, config.width - ax); ay = std::min(ay, config.height - ay);
            if (routeDir == 1) ax = (dx + config.width) % config.width;
            if (routeDir == 3) ax = (-dx + config.width) % config.width;
            if (routeDir == 2) ay = (dy + config.height) % config.height;
            if (routeDir == 0) ay = (-dy + config.height) % config.height;
        }
        return ax + ay;
    }
  private:
    int select(int dir) {
        lastCell = turn.here; expectedCell = topology.step(turn.here, dir); return dir;
    }
    int targetDistance(int c) const { return routeDistance(c, goal, wrap, route); }
    bool traversable(int d, int q, unsigned mask) const {
        return (mask & (1u << d)) && q >= 0 &&
            topology.edgeTypeAt(turn.here * 4 + d) != E_KELP &&
            !occupancy.teammateHeadAt(q) && !occupancy.blockedAt(q, 1);
    }
    void noteSeam(int d, int distance) {
        int x = topology.cx(turn.here), y = topology.cy(turn.here);
        bool seam = (d == 1 && x == config.width - 1) || (d == 3 && x == 0) ||
                    (d == 2 && y == config.height - 1) || (d == 0 && y == 0);
        if (seam && topology.edgeTypeAt(turn.here * 4 + d) == E_KELP &&
            targetDistance(topology.geo(turn.here, d)) < distance) seamBlocked = true;
    }
    std::array<int, 4> preferredDirections() const {
        int dx = topology.cx(goal) - topology.cx(turn.here), dy = topology.cy(goal) - topology.cy(turn.here);
        if (wrap) {
            if (dx > config.width / 2) dx -= config.width;
            else if (dx < -config.width / 2) dx += config.width;
            if (dy > config.height / 2) dy -= config.height;
            else if (dy < -config.height / 2) dy += config.height;
        }
        int horizontal = dx >= 0 ? 1 : 3, vertical = dy >= 0 ? 2 : 0;
        if (route == 1 || route == 3) horizontal = route;
        if (route == 0 || route == 2) vertical = route;
        std::array<int, 4> dirs{horizontal, vertical, horizontal ^ 2, vertical ^ 2};
        bool verticalFirst = route >= 0 ? (route == 0 || route == 2) : std::abs(dy) > std::abs(dx);
        if (verticalFirst) { std::swap(dirs[0], dirs[1]); std::swap(dirs[2], dirs[3]); }
        return dirs;
    }
};
} // namespace bot
