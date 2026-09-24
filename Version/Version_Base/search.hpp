#pragma once

#include "topology.hpp"
#include "occupancy.hpp"

namespace bot {

class Search {
    TurnState const &turn;
    Topology const &topology;
    OccupancyModel const &occupancy;
    std::vector<int> bfsMark, bfsDist, bfsFirst, bfsQ;
    int bfsStamp = 0;

  public:
    Search(GameConfig const &config, TurnState const &turn, Topology const &topology, OccupancyModel const &occupancy)
        : turn(turn), topology(topology), occupancy(occupancy) {
        int n = config.cells;
        bfsMark.assign(n, 0);
        bfsDist.assign(n, 0);
        bfsFirst.assign(n, -1);
        bfsQ.assign(n + 8, 0);
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

    // First direction of a shortest path to a goal cell (portal aware), -1 if none.
    template <class Goal> int bfsFirstDir(Goal isGoal, int maxDepth, int maxNodes) {
        ++bfsStamp;
        int qh = 0, qt = 0;
        bfsQ[qt++] = turn.here;
        bfsMark[turn.here] = bfsStamp;
        bfsDist[turn.here] = 0;
        bfsFirst[turn.here] = -1;
        while (qh < qt && qt < maxNodes) {
            int p = bfsQ[qh++];
            if (bfsDist[p] >= maxDepth) continue;
            for (int d = 0; d < 4; ++d) {
                if (turn.length == 2 && topology.edgeTypeAt(p * 4 + d) == E_PORTAL) continue;
                int q = topology.step(p, d);
                if (q < 0 || bfsMark[q] == bfsStamp) continue;
                int k = bfsDist[p] + 1;
                if (occupancy.blockedAt(q, k)) continue;
                bfsMark[q] = bfsStamp;
                bfsDist[q] = k;
                bfsFirst[q] = p == turn.here ? d : bfsFirst[p];
                if (isGoal(q)) return bfsFirst[q];
                bfsQ[qt++] = q;
            }
        }
        return -1;
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
