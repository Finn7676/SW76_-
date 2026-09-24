#pragma once

#include "topology.hpp"
#include "occupancy.hpp"

namespace bot {

class BodyMemory {
    TurnState const &turn;
    Topology const &topology;
    OccupancyModel &occupancy;

  public:
    BodyMemory(TurnState const &turn, Topology const &topology, OccupancyModel &occupancy)
        : turn(turn), topology(topology), occupancy(occupancy) {}

    // Newest head cell last; sole owner of hidden own-body history.
    std::vector<int> trail, pendingPath;
    int trailId = -1;
    void beginTurn() {
        if (trailId != turn.myId) {
            trail.clear();
            pendingPath.clear();
            trailId = turn.myId;
        }
        for (int c : pendingPath)
            trail.push_back(c);
        pendingPath.clear();
        if (trail.empty() || trail.back() != turn.here) trail.push_back(turn.here);
        if ((int)trail.size() > 2 * turn.length + 64) trail.erase(trail.begin(), trail.end() - (turn.length + 8));
    }
    void beginSprint() { pendingPath.clear(); }
    void recordIntermediate(int c) { pendingPath.push_back(c); }
    void reconstructHidden() {
        // own segments out of view (typically on the far side of a portal)
        int n = (int)trail.size();
        for (int j = 1; j < turn.length && j < n; ++j) {
            int c = trail[n - 1 - j];
            if (occupancy.visible(c)) {
                if (occupancy.occupied(c) && occupancy.occMine[c] && occupancy.occId[c] == turn.myId) continue;
                trail.erase(trail.begin(), trail.end() - j);
                break;
            }
            occupancy.occStamp[c] = occupancy.STAMP;
            occupancy.occId[c] = turn.myId;
            occupancy.occMine[c] = 1;
            occupancy.occHead[c] = 0;
            occupancy.occFree[c] = turn.length - 1 - j + 2;
            occupancy.occPart[c] = -1;
        }
    }
    // Own segment by index from the tail (0), or -1 when out of view.
    int ownSeg(int idx) const {
        int j = turn.length - 1 - idx, n = (int)trail.size();
        if (j >= 0 && j < n) return trail[n - 1 - j];
        for (auto const &p : turn.parts)
            if (p.mine && p.id == turn.myId && occupancy.freeAt(p.c) == idx + 2) return p.c;
        return -1;
    }
    // Two consecutive segments joined through a portal rather than a plain edge.
    bool bridged(int a, int b) const {
        if (a < 0 || b < 0) return false;
        for (int d = 0; d < 4; ++d)
            if (topology.geo(a, d) == b) return false;
        return true;
    }
    // The engine kills a length-2 dragon whose two segments straddle a portal.
    bool splitSafe(int k) const {
        if (turn.length - k == 2 && bridged(turn.here, ownSeg(turn.length - 2))) return false;
        if (k == 2 && bridged(ownSeg(0), ownSeg(1))) return false;
        return true;
    }

    // A split-off child's head is our tail cell; it needs a free cell besides its own neck.
    bool tailCanEscape() const {
        int tail = -1;
        for (auto const &p : turn.parts)
            if (p.mine && p.id == turn.myId && occupancy.freeAt(p.c) == 2) {
                tail = p.c;
                break;
            }
        if (tail < 0) return true;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(tail, d);
            if (q >= 0 && (!occupancy.visible(q) || !occupancy.occupied(q))) return true;
        }
        return false;
    }
};

} // namespace bot
