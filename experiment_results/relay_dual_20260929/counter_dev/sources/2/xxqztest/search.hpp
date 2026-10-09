#pragma once

#include "topology.hpp"
#include "occupancy.hpp"

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
    Search(GameConfig const &config, TurnState const &turn, Topology const &topology, OccupancyModel const &occupancy)
        : turn(turn), topology(topology), occupancy(occupancy) {
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
    int spaceFrom(int dest, bool ate, int cap, bool visionOnly) {
        ++bfsStamp;
        int qh = 0, qt = 0, n = 0;
        bfsQ[qt++] = dest;
        bfsMark[dest] = bfsStamp;
        bfsDist[dest] = 1;
        while (qh < qt && n < cap) {
            int p = bfsQ[qh++];
            ++n;
            int k = bfsDist[p] + 1;
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(p, d);
                if (q < 0 || bfsMark[q] == bfsStamp) continue;
                if (visionOnly && !occupancy.visible(q)) continue;
                if (occupancy.blockedAt(q, k, ate)) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = k;
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
    int controlledSpace(int dest, bool ate, int cap) {
        ++bfsStamp;
        int qh = 0, qt = 0, n = 0;
        bfsQ[qt++] = dest;
        bfsMark[dest] = bfsStamp;
        bfsDist[dest] = 0;
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
                if (q < 0 || bfsMark[q] == bfsStamp) continue;
                if (occupancy.blockedAt(q, dd + 2, ate)) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = dd + 1;
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
            if (occupancy.hasPearl(p) && occupancy.visible(p)) return bfsDist[p];
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
                    unsigned safeFirst = 15, int *bestGoal = nullptr, int *bestGoalDistance = nullptr) {
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
            if (p != turn.here && isGoal(p) &&
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
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int q = topology.step(p, d);
                if (q < 0) continue;
                int k = bfsDist[p] + 1;
                if (occupancy.blockedAt(q, k)) continue;
                int first = p == turn.here ? d : bfsFirst[p];
                int pearls = bfsPearls[p] + int(occupancy.visible(q) && occupancy.hasPearl(q));
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
                if (isGoal(q)) return bfsFirst[q];
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
        while (qh < qt) {
            int p = bfsQ[qh++];
            if (bfsDist[p] >= maxDepth) continue;
            for (int d = 0; d < 4; ++d) {
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int e = topology.edgeTypeAt(p * 4 + d);
                if (e != E_OPEN && e != E_PORTAL) continue;
                int q = topology.step(p, d), k = bfsDist[p] + 1;
                if (q >= 0 && occupancy.blockedAt(q, k)) continue;
                int first = p == turn.here ? d : bfsFirst[p];
                if (topology.unexploredPortal(p, d)) return first;
                if (q < 0 || bfsMark[q] == bfsStamp || qt >= maxNodes) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = k;
                bfsFirst[q] = first;
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
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int edge = p * 4 + d;
                int type = topology.edgeTypeAt(edge);
                if (type != E_OPEN && type != E_PORTAL) continue;
                int q = topology.step(p, d);
                int distance = bfsDist[p] + 1;
                if (distance > best.distance) continue;
                if (q >= 0 && occupancy.blockedAt(q, distance)) continue;
                int first = p == turn.here ? d : bfsFirst[p];
                int pearls = bfsPearls[p];
#if FEAT_V3_PEARL_ECONOMY
                if (q >= 0 && occupancy.visible(q) && occupancy.hasPearl(q)) ++pearls;
#endif
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
            if (p != turn.here && occupancy.visible(p) && occupancy.hasPearl(p)) {
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
                if (p == turn.here && !(safeFirst & (1u << d))) continue;
                int type = topology.edgeTypeAt(p * 4 + d);
                if (type != E_OPEN && type != E_PORTAL) continue;
                int q = topology.step(p, d), distance = bfsDist[p] + 1;
                if (q < 0 || occupancy.blockedAt(q, distance)) continue;
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
