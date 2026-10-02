#pragma once

#include "core.hpp"

namespace bot {

class OccupancyModel {
    friend class Observer;
    friend class BodyMemory;
    [[maybe_unused]] TurnState const &turn;
    int STAMP = 0;
    std::vector<int> visStamp, occStamp, occId, occFree, occPart;
    std::vector<uint8_t> occMine, occHead, visPearl;

  public:
    OccupancyModel(GameConfig const &config, TurnState const &turn) : turn(turn) {
        int n = config.cells;
        visStamp.assign(n, 0);
        occStamp.assign(n, 0);
        occId.assign(n, -1);
        occFree.assign(n, INF);
        occPart.assign(n, -1);
        occMine.assign(n, 0);
        occHead.assign(n, 0);
        visPearl.assign(n, 0);
    }
    void beginTurn() { ++STAMP; }
    int dragonId(int c) const { return occId[c]; }
    int freeAt(int c) const { return occFree[c]; }
    bool isMine(int c) const { return occMine[c]; }
    bool isHead(int c) const { return occHead[c]; }
    bool hasPearl(int c) const { return visPearl[c]; }
    inline bool visible(int c) const { return visStamp[c] == STAMP; }
    inline bool occupied(int c) const { return occStamp[c] == STAMP; }
    inline bool enemyHeadAt(int c) const { return occupied(c) && occHead[c] && !occMine[c]; }
    inline bool teammateHeadAt(int c) const {
        return occupied(c) && occMine[c] && occHead[c] && occId[c] != turn.myId;
    }

    // Blocked for the k-th move from now (k = 1 is this turn's move).
    inline bool blockedAt(int c, [[maybe_unused]] int k, [[maybe_unused]] int growth = 0) const {
        if (!occupied(c)) return false;
#if FEAT_TIMED_OCC
        int f = occFree[c];
        if (occId[c] == turn.myId && occMine[c]) f += growth;
        return k < f;
#else
        return true;
#endif
    }
};

} // namespace bot
