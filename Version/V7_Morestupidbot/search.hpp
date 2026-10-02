#pragma once

#include "topology.hpp"
#include "occupancy.hpp"
#include "coordination.hpp"

namespace bot {

struct PortalRoute {
    int dir = -1;
    int pid = -1;
    int distance = INF;
};

struct PearlRoute {
    int dir = -1;
    int distance = INF;
    int reachablePearls = 0;
};

struct PocketInfo {
    bool proven = false;
    int pearls = 0;
    int cells = 0;
};

class Search {
    TurnState const &turn;
    WorldModel const *coordinationWorld = nullptr;
    Topology const &topology;
    OccupancyModel const &occupancy;
    std::vector<int> bfsMark, bfsDist, bfsFirst, bfsQ, bfsPearls;
    std::vector<long> bfsRouteScore;
    std::vector<int> tileScoreStamp;
    std::vector<long> tileScoreMemo;
    int bfsStamp = 0;

    template <class TileScore> long routeTileValue(int c, int step, TileScore const &tileScore) {
        if (tileScoreStamp[c] != bfsStamp) {
            tileScoreStamp[c] = bfsStamp;
            tileScoreMemo[c] = tileScore(c, step);
        }
        return tileScoreMemo[c];
    }
    bool localScoutAhead(int pid, int myDistance) const {
#if FEAT_V3_PORTAL_CLAIM
        for (auto const &part : turn.parts) {
            if (!part.mine || !part.head || part.id == turn.myId || turn.countOf(part.id) > 8)
                continue;
            int otherDistance = topology.portalApproachDistance(part.c, pid);
            if (otherDistance <= 3 &&
                (otherDistance < myDistance || (otherDistance == myDistance && part.id < turn.myId)))
                return true;
        }
#else
        (void)pid; (void)myDistance;
#endif
        return false;
    }

  public:
    Search(GameConfig const &config, TurnState const &turn, Topology const &topology, OccupancyModel const &occupancy,
           WorldModel const *world = nullptr)
        : turn(turn), coordinationWorld(world), topology(topology), occupancy(occupancy) {
        int n = config.cells;
        bfsMark.assign(n, 0);
        bfsDist.assign(n, 0);
        bfsFirst.assign(n, -1);
        bfsQ.assign(n + 8, 0);
        bfsPearls.assign(n, 0);
        bfsRouteScore.assign(n, 0);
        tileScoreStamp.assign(n, 0);
        tileScoreMemo.assign(n, 0);
    }

    bool pearlAvailable(int c, int moves = 1) const {
        return !coordinationWorld || Coordination(turn, *coordinationWorld, topology, occupancy).pearlAvailable(c, moves);
    }
    bool portalBusy(int c, int d) const {
        if(coordinationWorld && Coordination(turn,*coordinationWorld,topology,occupancy)
            .protectedPortalExit(topology.step(c,d)))return true;
        return coordinationWorld && topology.edgeTypeAt(c * 4 + d) == E_PORTAL &&
               (Coordination(turn, *coordinationWorld, topology, occupancy).portalBusy(topology.portalIdAt(c * 4 + d)) ||
                Coordination(turn, *coordinationWorld, topology, occupancy).portalHeld(c, d));
    }

    // Prove a small, cycle-free, one-entrance pocket using known terrain only.
    // Unknown edges, portals, a second connection to the entrance, or the node
    // cap make the result uncertain, never a reason for a hard veto.
    PocketInfo pocketBeyond(int start, int entrance, int maxNodes = 24) {
        if (start < 0 || start == entrance || maxNodes < 1) return {};
        ++bfsStamp;
        int qh = 0, qt = 0, pearls = 0;
        bfsQ[qt++] = start;
        bfsMark[start] = bfsStamp;
        bfsFirst[start] = entrance;
        while (qh < qt) {
            int p = bfsQ[qh++];
            if (occupancy.visible(p) && occupancy.hasPearl(p)) ++pearls;
            for (int d = 0; d < 4; ++d) {
                uint8_t edge = topology.edgeTypeAt(p * 4 + d);
                if (edge == E_KELP) continue;
                if (edge != E_OPEN) return {};
                int q = topology.step(p, d);
                if (q < 0) return {};
                if (q == entrance) {
                    if (p != start) return {};
                    continue;
                }
                if (bfsMark[q] == bfsStamp) {
                    if (q != bfsFirst[p]) return {};
                    continue;
                }
                if (qt >= maxNodes) return {};
                bfsMark[q] = bfsStamp;
                bfsFirst[q] = p;
                bfsQ[qt++] = q;
            }
        }
        return {true, pearls, qt};
    }

    // Cells reachable from dest (reached on move 1) honouring time-aware occupancy.
    int spaceFrom(int dest, int pearlGrowth, int cap, bool visionOnly) {
        ++bfsStamp;
        int qh = 0, qt = 0, n = 0;
        bfsQ[qt++] = dest;
        bfsMark[dest] = bfsStamp;
        bfsDist[dest] = 1;
        bfsPearls[dest] = pearlGrowth;
        while (qh < qt && n < cap) {
            int p = bfsQ[qh++];
            ++n;
            int k = bfsDist[p] + 1;
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(p, d);
                if (q < 0) continue;
                if (visionOnly && !occupancy.visible(q)) continue;
                int growth = bfsPearls[p];
                if (occupancy.blockedAt(q, k, growth)) continue;
                int nextGrowth = growth + int(occupancy.visible(q) && occupancy.hasPearl(q));
                if (bfsMark[q] == bfsStamp) {
                    if (bfsDist[q] == k && nextGrowth < bfsPearls[q]) bfsPearls[q] = nextGrowth;
                    continue;
                }
                bfsMark[q] = bfsStamp;
                bfsDist[q] = k;
                bfsPearls[q] = nextGrowth;
                bfsQ[qt++] = q;
            }
        }
        return n;
    }

    int onwardSafe(int dest, bool ate) {
        int n = 0;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(dest, d);
            if (q < 0) {
                if (topology.edgeTypeAt(dest * 4 + d) == E_PORTAL) ++n;
                continue;
            }
            if (!occupancy.blockedAt(q, 2, ate)) ++n;
        }
        return n;
    }

    // Tron-style Voronoi: cells we reach no later than the nearest visible enemy head.
    int controlledSpace(int dest, int pearlGrowth, int cap) {
        ++bfsStamp;
        int qh = 0, qt = 0, n = 0;
        bfsQ[qt++] = dest;
        bfsMark[dest] = bfsStamp;
        bfsDist[dest] = 0;
        bfsPearls[dest] = pearlGrowth;
        while (qh < qt && n < cap) {
            int p = bfsQ[qh++];
            int dd = bfsDist[p];
            int em = INF;
            for (int eh : turn.enemyHeads)
                em = std::min(em, topology.manh(p, eh));
            if (dd > em) continue;
            ++n;
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(p, d);
                if (q < 0) continue;
                int growth = bfsPearls[p];
                if (occupancy.blockedAt(q, dd + 2, growth)) continue;
                int nextGrowth = growth + int(occupancy.visible(q) && occupancy.hasPearl(q));
                if (bfsMark[q] == bfsStamp) {
                    if (bfsDist[q] == dd + 1 && nextGrowth < bfsPearls[q]) bfsPearls[q] = nextGrowth;
                    continue;
                }
                bfsMark[q] = bfsStamp;
                bfsDist[q] = dd + 1;
                bfsPearls[q] = nextGrowth;
                bfsQ[qt++] = q;
            }
        }
        return n;
    }

    int visPearlDist(int s) {
        if (turn.visPearls.empty()) return 999;
        ++bfsStamp;
        int qh = 0, qt = 0;
        bfsQ[qt++] = s;
        bfsMark[s] = bfsStamp;
        bfsDist[s] = 0;
        while (qh < qt) {
            int p = bfsQ[qh++];
            if (occupancy.hasPearl(p) && occupancy.visible(p) && pearlAvailable(p, bfsDist[p] + 1)) return bfsDist[p];
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(p, d);
                if (q < 0 || bfsMark[q] == bfsStamp || !occupancy.visible(q)) continue;
                if (occupancy.blockedAt(q, bfsDist[p] + 2)) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = bfsDist[p] + 1;
                bfsQ[qt++] = q;
            }
        }
        return 999;
    }

    // Compare every reachable goal at the shortest depth. For paths of that
    // depth, maximize collected pearls, then additive tile score.
    template <class Goal, class TileScore>
    int bfsFirstDir(Goal isGoal, [[maybe_unused]] TileScore tileScore, int maxDepth, int maxNodes,
                    unsigned safeFirst = 15, int *bestGoal = nullptr, int *bestGoalDistance = nullptr,
                    bool allowPortals = true) {
        ++bfsStamp;
        int qh = 0, qt = 0;
        bfsQ[qt++] = turn.here;
        bfsMark[turn.here] = bfsStamp;
        bfsDist[turn.here] = 0;
        bfsFirst[turn.here] = -1;
        bfsPearls[turn.here] = 0;
        bfsRouteScore[turn.here] = 0;
        int bestDir = -1, bestDistance = INF, bestPearls = -1;
        long bestScore = LONG_MIN;
        if (bestGoal) *bestGoal = -1;
        if (bestGoalDistance) *bestGoalDistance = INF;
        while (qh < qt) {
            int p = bfsQ[qh++];
            if (bfsDist[p] > bestDistance) break;
            if (p != turn.here && isGoal(p) && pearlAvailable(p, bfsDist[p]) &&
                (bfsDist[p] < bestDistance ||
                 (bfsDist[p] == bestDistance &&
                  (bfsPearls[p] > bestPearls ||
                   (bfsPearls[p] == bestPearls && bfsRouteScore[p] > bestScore))))) {
                bestDir = bfsFirst[p];
                bestDistance = bfsDist[p];
                bestPearls = bfsPearls[p];
                bestScore = bfsRouteScore[p];
                if (bestGoal) *bestGoal = p;
                if (bestGoalDistance) *bestGoalDistance = bestDistance;
            }
            if (bfsDist[p] >= maxDepth || bfsDist[p] >= bestDistance) continue;
            for (int d = 0; d < 4; ++d) {
                if ((!allowPortals && topology.edgeTypeAt(p*4+d)==E_PORTAL) || portalBusy(p, d)) continue;
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int q = topology.step(p, d);
                if (q < 0) continue;
                int k = bfsDist[p] + 1;
                // Pearls eaten on earlier route steps delay our own tail by one
                // move each; include that accumulated growth before testing the
                // destination. A pearl on q cannot help enter q itself.
                int pearls = bfsPearls[p];
                if (occupancy.blockedAt(q, k, pearls)) continue;
                int first = p == turn.here ? d : bfsFirst[p];
                pearls += int(occupancy.visible(q) && occupancy.hasPearl(q));
#if FEAT_V3_PEARL_ECONOMY
                long score = bfsRouteScore[p] + routeTileValue(q, k, tileScore);
#else
                long score = 0;
#endif
                if (bfsMark[q] == bfsStamp) {
                    if (bfsDist[q] == k &&
                        (pearls > bfsPearls[q] ||
                         (pearls == bfsPearls[q] && score > bfsRouteScore[q]))) {
                        bfsFirst[q] = first;
                        bfsPearls[q] = pearls;
                        bfsRouteScore[q] = score;
                    }
                    continue;
                }
                if (qt >= maxNodes) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = k;
                bfsFirst[q] = first;
                bfsPearls[q] = pearls;
                bfsRouteScore[q] = score;
#if !FEAT_V3_PEARL_ECONOMY
                if (isGoal(q) && pearlAvailable(q, k)) return bfsFirst[q];
#endif
                bfsQ[qt++] = q;
            }
        }
        return bestDir;
    }
    template <class Goal> int bfsFirstDir(Goal isGoal, int maxDepth, int maxNodes) {
        return bfsFirstDir(isGoal, [](int, int) { return 0L; }, maxDepth, maxNodes);
    }

    // Search for an edge goal: an unresolved portal is a destination, never a corridor.
    // Only observed open edges and resolved portals can connect the route to it.
    int unexploredPortalDir(unsigned safeFirst, int maxDepth = 24, int maxNodes = 320) {
        if (!safeFirst) return -1;
        ++bfsStamp;
        int qh = 0, qt = 0;
        bfsQ[qt++] = turn.here;
        bfsMark[turn.here] = bfsStamp;
        bfsDist[turn.here] = 0;
        bfsFirst[turn.here] = -1;
        bfsPearls[turn.here] = 0;
        while (qh < qt) {
            int p = bfsQ[qh++];
            if (bfsDist[p] >= maxDepth) continue;
            for (int d = 0; d < 4; ++d) {
                if (portalBusy(p, d)) continue;
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int e = topology.edgeTypeAt(p * 4 + d);
                if (e != E_OPEN && e != E_PORTAL) continue;
                int q = topology.step(p, d), k = bfsDist[p] + 1;
                int pearls = bfsPearls[p];
                if (q >= 0 && occupancy.blockedAt(q, k, pearls)) continue;
                int first = p == turn.here ? d : bfsFirst[p];
                if (topology.unexploredPortal(p, d)) return first;
                if (q < 0 || qt >= maxNodes) continue;
                pearls += int(occupancy.visible(q) && occupancy.hasPearl(q));
                if (bfsMark[q] == bfsStamp) {
                    if (bfsDist[q] == k && pearls < bfsPearls[q]) {
                        bfsPearls[q] = pearls;
                        bfsFirst[q] = first;
                    }
                    continue;
                }
                bfsMark[q] = bfsStamp;
                bfsDist[q] = k;
                bfsFirst[q] = first;
                bfsPearls[q] = pearls;
                bfsQ[qt++] = q;
            }
        }
        return -1;
    }

    // Keep the best route to each cell at the same BFS depth. Additive tile
    // scores make this equivalent to comparing all shortest paths without
    // enumerating exponentially many paths. Unknown exits are goals only.
    template <class TileScore>
    PortalRoute scoutPortalRoute(unsigned safeFirst, [[maybe_unused]] TileScore tileScore,
                                 int maxDepth = V3_SCOUT_MAX_PATH, int maxNodes = 320,
                                 int targetPid = -1) {
        if (!safeFirst) return {};
        ++bfsStamp;
        int qh = 0, qt = 0;
        bfsQ[qt++] = turn.here;
        bfsMark[turn.here] = bfsStamp;
        bfsDist[turn.here] = 0;
        bfsFirst[turn.here] = -1;
        bfsPearls[turn.here] = 0;
        bfsRouteScore[turn.here] = 0;
        PortalRoute best;
        [[maybe_unused]] int bestPearls = -1;
        [[maybe_unused]] long bestScore = LONG_MIN;
        while (qh < qt) {
            int p = bfsQ[qh++];
            if (bfsDist[p] >= maxDepth || bfsDist[p] >= best.distance) continue;
            for (int d = 0; d < 4; ++d) {
                if (portalBusy(p, d)) continue;
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int edge = p * 4 + d;
                int type = topology.edgeTypeAt(edge);
                if (type != E_OPEN && type != E_PORTAL) continue;
                int q = topology.step(p, d);
                int distance = bfsDist[p] + 1;
                if (distance > best.distance) continue;
                int pearls = bfsPearls[p];
                if (q >= 0 && occupancy.blockedAt(q, distance, pearls)) continue;
                int first = p == turn.here ? d : bfsFirst[p];
                if (q >= 0 && occupancy.visible(q) && occupancy.hasPearl(q)) ++pearls;
#if FEAT_V3_PEARL_ECONOMY
                long score = bfsRouteScore[p] + (q >= 0 ? routeTileValue(q, distance, tileScore) : 0);
#else
                long score = 0;
#endif
                if (topology.unexploredPortal(p, d)) {
                    int pid = topology.portalIdAt(edge);
                    if (targetPid >= 0 && pid != targetPid) continue;
#if !FEAT_V3_PEARL_ECONOMY
                    if (!topology.claimedByEarlierScout(pid, turn.myId, turn.round) &&
                        !localScoutAhead(pid, distance))
                        return {first, pid, distance};
#else
                    if (!topology.claimedByEarlierScout(pid, turn.myId, turn.round) &&
                        !localScoutAhead(pid, distance) &&
                        (distance < best.distance ||
                         (distance == best.distance &&
                          (pearls > bestPearls || (pearls == bestPearls && score > bestScore))))) {
                        best = {first, pid, distance};
                        bestPearls = pearls;
                        bestScore = score;
                    }
#endif
                }
                if (q < 0 || distance >= best.distance) continue;
                if (bfsMark[q] == bfsStamp) {
                    if (bfsDist[q] == distance &&
                        (pearls > bfsPearls[q] ||
                         (pearls == bfsPearls[q] && score > bfsRouteScore[q]))) {
                        bfsPearls[q] = pearls;
                        bfsRouteScore[q] = score;
                        bfsFirst[q] = first;
                    }
                    continue;
                }
                if (qt >= maxNodes) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = distance;
                bfsFirst[q] = first;
                bfsPearls[q] = pearls;
                bfsRouteScore[q] = score;
                bfsQ[qt++] = q;
            }
        }
        return best;
    }
    PortalRoute scoutPortalRoute(unsigned safeFirst, int maxDepth = V3_SCOUT_MAX_PATH, int maxNodes = 320) {
        return scoutPortalRoute(safeFirst, [](int, int) { return 0L; }, maxDepth, maxNodes);
    }

    // Nearby visible pearls can briefly interrupt a Scout's portal task. Only
    // known traversable edges are used; every turn checks the next step again.
    template <class TileScore>
    PearlRoute scoutPearlRoute(unsigned safeFirst, TileScore tileScore,
                              int maxDepth = V3_PEARL_MAX_PATH, int maxNodes = 320) {
        if (!safeFirst) return {};
        ++bfsStamp;
        int qh = 0, qt = 0;
        bfsQ[qt++] = turn.here;
        bfsMark[turn.here] = bfsStamp;
        bfsDist[turn.here] = 0;
        bfsFirst[turn.here] = -1;
        bfsPearls[turn.here] = 0;
        bfsRouteScore[turn.here] = 0;
        PearlRoute best;
        int bestPearls = -1;
        long bestScore = LONG_MIN;
        while (qh < qt) {
            int p = bfsQ[qh++];
            if (p != turn.here && occupancy.visible(p) && occupancy.hasPearl(p) && pearlAvailable(p, bfsDist[p])) {
                ++best.reachablePearls;
                if (bfsDist[p] < best.distance ||
                    (bfsDist[p] == best.distance &&
                     (bfsPearls[p] > bestPearls ||
                      (bfsPearls[p] == bestPearls && bfsRouteScore[p] > bestScore)))) {
                    best.dir = bfsFirst[p];
                    best.distance = bfsDist[p];
                    bestPearls = bfsPearls[p];
                    bestScore = bfsRouteScore[p];
                }
            }
            if (bfsDist[p] >= maxDepth) continue;
            for (int d = 0; d < 4; ++d) {
                if (portalBusy(p, d)) continue;
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int type = topology.edgeTypeAt(p * 4 + d);
                if (type != E_OPEN && type != E_PORTAL) continue;
                int q = topology.step(p, d), distance = bfsDist[p] + 1;
                if (q < 0 || occupancy.blockedAt(q, distance, bfsPearls[p])) continue;
                int pearls = bfsPearls[p] + int(occupancy.visible(q) && occupancy.hasPearl(q));
                long score = bfsRouteScore[p] + routeTileValue(q, distance, tileScore);
                int first = p == turn.here ? d : bfsFirst[p];
                if (bfsMark[q] == bfsStamp) {
                    if (bfsDist[q] == distance &&
                        (pearls > bfsPearls[q] ||
                         (pearls == bfsPearls[q] && score > bfsRouteScore[q]))) {
                        bfsPearls[q] = pearls;
                        bfsRouteScore[q] = score;
                        bfsFirst[q] = first;
                    }
                    continue;
                }
                if (qt >= maxNodes) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = distance;
                bfsFirst[q] = first;
                bfsPearls[q] = pearls;
                bfsRouteScore[q] = score;
                bfsQ[qt++] = q;
            }
        }
        return best;
    }
    PearlRoute scoutPearlRoute(unsigned safeFirst, int maxDepth = V3_PEARL_MAX_PATH, int maxNodes = 320) {
        return scoutPearlRoute(safeFirst, [](int, int) { return 0L; }, maxDepth, maxNodes);
    }

    // Choose the first move of a short route that maximizes confirmed pearls
    // collected over the whole horizon, rather than steering to the nearest
    // individual pearl.
    int bestPearlHarvestDir(int horizon = 5, unsigned safeFirst = 15) {
        if (turn.visPearls.empty() || horizon <= 0) return -1;
        horizon = std::min(horizon, 5);
        std::array<int, 6> path{};
        std::array<int, 5> eaten{}, eatenStep{};
        path[0] = turn.here;
        int bestDir = -1, bestPearls = 0, bestSteps = 0, bestSpace = -1;
        auto evaluate = [&](int c, int steps, int pearls, int first) {
            if (steps <= 0 || first < 0 || pearls <= 0) return;
            int available = 0;
            for (int i = 0; i < pearls; ++i)
                available += int(pearlAvailable(eaten[i], eatenStep[i]));
            if (available <= 0) return;
            // Original segments have aged by `steps`, while every route
            // cell still in the grown body is a new obstacle. Looking only
            // at the initial occupancy falsely makes N-W-S corners escapable.
            auto blocked = [&](int q, int move, int growth = 0) {
                if (occupancy.teammateHeadAt(q)) return true;
                if (occupancy.blockedAt(q, steps + 1, pearls) &&
                    occupancy.blockedAt(q, steps + move, pearls + growth)) return true;
                for (int i = 1; i <= steps; ++i) {
                    int freeAfter = turn.length + pearls + 1 - steps + i;
                    // Future growth delays living segments, never restores
                    // a segment that had already left the body at this prefix.
                    if (path[i] == q && freeAfter > 1 && move < freeAfter + growth) return true;
                }
                return false;
            };
            bool escape = false;
            for (int d = 0; d < 4; ++d) {
                if (topology.edgeTypeAt(c * 4 + d) == E_KELP || portalBusy(c, d)) continue;
                int q = topology.step(c, d);
                if (q >= 0 && !blocked(q, 1)) escape = true;
            }
            if (!escape) return;
            ++bfsStamp;
            int qh = 0, qt = 0, space = 0;
            bfsQ[qt++] = c; bfsMark[c] = bfsStamp; bfsDist[c] = 0; bfsPearls[c] = 0;
            while (qh < qt && space < 16) {
                int from = bfsQ[qh++]; ++space;
                for (int d = 0; d < 4; ++d) {
                    if (topology.edgeTypeAt(from * 4 + d) == E_KELP || portalBusy(from, d)) continue;
                    int q = topology.step(from, d), move = bfsDist[from] + 1;
                    int growth = bfsPearls[from];
                    if (q < 0 || bfsMark[q] == bfsStamp || blocked(q, move, growth)) continue;
                    bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
                    for (int i = 0; i < pearls; ++i)
                        if (eaten[i] == q) eats = false;
                    bfsMark[q] = bfsStamp; bfsDist[q] = move; bfsPearls[q] = growth + int(eats);
                    bfsQ[qt++] = q;
                }
            }
            if (space < std::min(turn.length + 1, 8)) return;
            if (available > bestPearls ||
                (available == bestPearls && (steps < bestSteps ||
                 (steps == bestSteps && space > bestSpace)))) {
                bestDir = first;
                bestPearls = available;
                bestSpace = space;
                bestSteps = steps;
            }
        };
        auto visit = [&](auto &&self, int c, int steps, int pearls, int first) -> void {
            // A productive prefix is a complete collection option; do not
            // force it to spend the entire horizon wandering for more space.
            evaluate(c, steps, pearls, first);
            if (steps >= horizon) return;
            for (int d = 0; d < 4; ++d) {
                if (portalBusy(c, d)) continue;
                if (steps == 0 && !(safeFirst & (1u << d))) continue;
                if (topology.edgeTypeAt(c * 4 + d) == E_KELP) continue;
                int q = topology.step(c, d);
                if (q < 0 || occupancy.teammateHeadAt(q) ||
                    occupancy.blockedAt(q, steps + 1, pearls)) continue;
                bool loop = false;
                for (int i = 0; i <= steps; ++i)
                    if (path[i] == q) { loop = true; break; }
                if (loop) continue;
                bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
                for (int i = 0; i < pearls; ++i)
                    if (eaten[i] == q) { eats = false; break; }
                path[steps + 1] = q;
                if (eats) { eaten[pearls] = q; eatenStep[pearls] = steps + 1; }
                self(self, q, steps + 1, pearls + int(eats), steps == 0 ? d : first);
            }
        };
        visit(visit, turn.here, 0, 0, -1);
        return bestPearls > 0 ? bestDir : -1;
    }

    int enemyEscapes(int eh) {
        if (!occupancy.visible(eh)) return 4;
        int n = 0;
        for (int d = 0; d < 4; ++d) {
            if (topology.edgeTypeAt(eh * 4 + d) == E_KELP) continue;
            int q = topology.step(eh, d);
            if (q < 0) {
                ++n;
                continue;
            }
            if (!occupancy.visible(q) || !occupancy.blockedAt(q, 1)) ++n;
        }
        return n;
    }
};

} // namespace bot
