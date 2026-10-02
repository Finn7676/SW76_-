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
    int wallEpoch = 0;
    unsigned blockedSeams = 0, bypassedWrapAxes = 0;
    std::vector<int> wallSeen, recovery;
    size_t recoveryIndex = 0;
    bool wrap = true, following = false, seamBlocked = false, allowPortalTravel = true;
  public:
    BugNav(GameConfig const &config, TurnState const &turn, Topology const &topology,
           OccupancyModel const &occupancy)
        : config(config), turn(turn), topology(topology), occupancy(occupancy), wallSeen(config.cells * 4, -1) {}
    void reset() {
        goal = -1; hitDistance = bestDistance = INF;
        heading = lastCell = expectedCell = -1;
        wallSteps = repeats = staleSteps = 0; route = -1;
        following = seamBlocked = false;
        blockedSeams = bypassedWrapAxes = 0; ++wallEpoch;
        recovery.clear(); recoveryIndex = 0;
    }
    bool boundaryStalled() const { return following && seamBlocked && staleSteps >= 12; }
    int direction(int target, bool wrapped = true, int tieBreaker = 0,
                  int routeDir = -1, unsigned safeFirst = 15, bool allowPortals = true) {
        if (target < 0 || target >= config.cells) return -1;
        if (goal != target || wrap != wrapped || route != routeDir || allowPortalTravel != allowPortals) {
            reset(); goal = target; wrap = wrapped; route = routeDir; allowPortalTravel = allowPortals;
            side = (tieBreaker & 1) ? -1 : 1;
        }
        // A detour or the final guard can move us off the wall we were following.
        if (expectedCell >= 0 && turn.here != expectedCell) {
            recovery.clear(); recoveryIndex = 0;
            if (turn.here != lastCell) {
                following = false; wallSteps = repeats = staleSteps = 0; ++wallEpoch;
            }
        }
        if (recoveryIndex < recovery.size()) {
            int d = recovery[recoveryIndex];
            if (traversable(d, topology.step(turn.here, d), safeFirst)) {
                ++recoveryIndex; return select(d);
            }
            recovery.clear(); recoveryIndex = 0;
        }
        int distance = targetDistance(turn.here);
        if (turn.here == target) return -1;
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
            wallSteps = repeats = staleSteps = 0; ++wallEpoch;
        }
        repeats = turn.here == lastCell ? repeats + 1 : 0;
        ++wallSteps;
        if (distance < bestDistance) { bestDistance = distance; staleSteps = 0; }
        else ++staleSteps;
        int state = turn.here * 4 + heading;
        bool loop = wallSeen[state] == wallEpoch;
        wallSeen[state] = wallEpoch;
        unsigned newBypass = wrap ? blockedSeams & ~bypassedWrapAxes : 0;
        if (loop || repeats >= 2 || (newBypass && staleSteps >= 12) || wallSteps >= 32) {
            if (newBypass) {
                // A known blocked seam made wrapped distance misleading.
                // Temporarily route through the interior on that axis only.
                bypassedWrapAxes |= newBypass;
                following = false; wallSteps = repeats = staleSteps = 0; ++wallEpoch;
                if (findRecovery(targetDistance(turn.here), safeFirst)) {
                    recoveryIndex = 1; return select(recovery[0]);
                }
                return direction(target, wrapped, tieBreaker, routeDir, safeFirst, allowPortals);
            }
            if (findRecovery(bestDistance, safeFirst)) {
                following = false; wallSteps = repeats = staleSteps = 0; ++wallEpoch;
                recoveryIndex = 1;
                return select(recovery[0]);
            }
            side = -side; hitDistance = distance; wallSteps = repeats = staleSteps = 0; ++wallEpoch;
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
    int routeDistance(int c, int target, bool wrapped = true, int routeDir = -1, unsigned wrappedAxes = 3) const {
        int dx = topology.cx(target) - topology.cx(c), dy = topology.cy(target) - topology.cy(c);
        int ax = std::abs(dx), ay = std::abs(dy);
        if (wrapped && (wrappedAxes & 1)) {
            ax = std::min(ax, config.width - ax);
            if (routeDir == 1) ax = (dx + config.width) % config.width;
            if (routeDir == 3) ax = (-dx + config.width) % config.width;
        }
        if (wrapped && (wrappedAxes & 2)) {
            ay = std::min(ay, config.height - ay);
            if (routeDir == 2) ay = (dy + config.height) % config.height;
            if (routeDir == 0) ay = (-dy + config.height) % config.height;
        }
        return ax + ay;
    }
  private:
    bool findRecovery(int improvementDistance, unsigned safeFirst) {
        // Bounded, observed-terrain escape from a repeated wall state. Keep
        // the whole detour so its first temporarily-away step is not undone.
        struct Node { int cell, parent, dir, depth, growth; };
        std::vector<Node> queue{{turn.here, -1, -1, 0, 0}};
        std::vector<uint8_t> visited(config.cells, 0);
        visited[turn.here] = 1;
        int found = -1;
        for (size_t n = 0; n < queue.size() && found < 0; ++n) {
            Node current = queue[n];
            if (current.depth >= 24) continue;
            for (int d = 0; d < 4; ++d) {
                if (!current.depth && !(safeFirst & (1u << d))) continue;
                uint8_t edge = topology.edgeTypeAt(current.cell * 4 + d);
                if (edge != E_OPEN && (edge != E_PORTAL || !allowPortalTravel)) continue;
                int q = topology.step(current.cell, d);
                if (q < 0 || visited[q] || occupancy.teammateHeadAt(q) ||
                    occupancy.blockedAt(q, current.depth + 1, current.growth)) continue;
                if (queue.size() >= 320) continue;
                visited[q] = 1;
                queue.push_back({q, int(n), d, current.depth + 1,
                    current.growth + int(occupancy.visible(q) && occupancy.hasPearl(q))});
                if (targetDistance(q) < improvementDistance || q == goal) {
                    found = int(queue.size()) - 1; break;
                }
            }
        }
        recovery.clear(); recoveryIndex = 0;
        if (found < 0) return false;
        for (int n = found; queue[n].parent >= 0; n = queue[n].parent)
            recovery.push_back(queue[n].dir);
        std::reverse(recovery.begin(), recovery.end());
        return !recovery.empty();
    }
    int select(int dir) {
        lastCell = turn.here; expectedCell = topology.step(turn.here, dir); return dir;
    }
    int targetDistance(int c) const { return routeDistance(c, goal, wrap, route, 3 & ~bypassedWrapAxes); }
    bool traversable(int d, int q, unsigned mask) const {
        return (mask & (1u << d)) && q >= 0 &&
            (allowPortalTravel || topology.edgeTypeAt(turn.here*4+d)!=E_PORTAL) &&
            topology.edgeTypeAt(turn.here * 4 + d) != E_KELP &&
            !occupancy.teammateHeadAt(q) && !occupancy.blockedAt(q, 1);
    }
    void noteSeam(int d, int distance) {
        int x = topology.cx(turn.here), y = topology.cy(turn.here);
        bool seam = (d == 1 && x == config.width - 1) || (d == 3 && x == 0) ||
                    (d == 2 && y == config.height - 1) || (d == 0 && y == 0);
        if (seam && topology.edgeTypeAt(turn.here * 4 + d) == E_KELP &&
            targetDistance(topology.geo(turn.here, d)) < distance) {
            seamBlocked = true;
            blockedSeams |= (d == 1 || d == 3) ? 1u : 2u;
        }
    }
    std::array<int, 4> preferredDirections() const {
        int dx = topology.cx(goal) - topology.cx(turn.here), dy = topology.cy(goal) - topology.cy(turn.here);
        if (wrap && !(bypassedWrapAxes & 1)) {
            if (dx > config.width / 2) dx -= config.width;
            else if (dx < -config.width / 2) dx += config.width;
        }
        if (wrap && !(bypassedWrapAxes & 2)) {
            if (dy > config.height / 2) dy -= config.height;
            else if (dy < -config.height / 2) dy += config.height;
        }
        int horizontal = dx >= 0 ? 1 : 3, vertical = dy >= 0 ? 2 : 0;
        if (!(bypassedWrapAxes & 1) && (route == 1 || route == 3)) horizontal = route;
        if (!(bypassedWrapAxes & 2) && (route == 0 || route == 2)) vertical = route;
        std::array<int, 4> dirs{horizontal, vertical, horizontal ^ 2, vertical ^ 2};
        bool verticalFirst = route >= 0 ? (route == 0 || route == 2) : std::abs(dy) > std::abs(dx);
        if (verticalFirst) { std::swap(dirs[0], dirs[1]); std::swap(dirs[2], dirs[3]); }
        return dirs;
    }
};
} // namespace bot
