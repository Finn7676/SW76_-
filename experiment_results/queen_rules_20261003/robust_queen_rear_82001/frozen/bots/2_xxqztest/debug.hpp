#pragma once

#include "topology.hpp"
#include "occupancy.hpp"

namespace bot {

#ifdef DEBUG_PORTAL
struct PortalDebug {
    int pendingPred = -1, best = -1;
    long scores[4] = {LONG_MIN, LONG_MIN, LONG_MIN, LONG_MIN};
    struct ChoiceGuard {
        PortalDebug &debug;
        int const &chosen;
        ~ChoiceGuard() { debug.best = chosen; }
    };
    void beginTurn() {
        best = -1;
        std::fill(std::begin(scores), std::end(scores), LONG_MIN);
    }
    void choice(TurnState const &turn, Topology const &topology) const {
        if (best >= 0 && (topology.edgeTypeAt(turn.here * 4 + best) == E_PORTAL ||
                          (topology.cx(turn.here) == 16 && topology.cy(turn.here) == 7)))
            std::clog << "PCHOICE r" << turn.round << " id" << turn.myId << " at " << topology.cx(turn.here) << ","
                      << topology.cy(turn.here) << " len" << turn.length << " d" << best << " exit"
                      << topology.portalExit(turn.here, best) << " scores " << scores[0] << " " << scores[1] << " "
                      << scores[2] << " " << scores[3] << std::endl;
    }
    void crossing(TurnState const &turn, WorldModel const &world, Topology const &topology) const {
        std::clog << "PORTAL r" << turn.round << " id" << turn.myId << " from "
                  << topology.cx(world.pendingPortalKey / 4) << "," << topology.cy(world.pendingPortalKey / 4) << " d"
                  << world.pendingPortalKey % 4 << " pred " << (pendingPred < 0 ? -1 : topology.cx(pendingPred)) << ","
                  << (pendingPred < 0 ? -1 : topology.cy(pendingPred)) << " got " << topology.cx(turn.here) << ","
                  << topology.cy(turn.here)
                  << (pendingPred == turn.here ? " OK"
                      : pendingPred < 0        ? " UNK"
                                               : " MISMATCH")
                  << std::endl;
    }
};
struct ActionCapture {
    std::ostringstream cap;
    std::streambuf *saved;
    ActionCapture() : saved(std::cout.rdbuf(cap.rdbuf())) {}
    void finish(TurnState const &turn, Topology const &topology, OccupancyModel const &occupancy) {
        {
            std::cout.rdbuf(saved);
            std::cout << cap.str();
            std::string act = cap.str();
            if (!act.empty() && act.back() == '\n') act.pop_back();
            std::cout << "LOG ACT r" << turn.round << " id" << turn.myId << " at " << topology.cx(turn.here) << ","
                      << topology.cy(turn.here) << " len" << turn.length << " -> " << act;
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(turn.here, d);
                std::cout << " | " << DCH[d] << " e" << (int)topology.edgeTypeAt(turn.here * 4 + d) << " q" << q;
                if (q >= 0)
                    std::cout << "(" << topology.cx(q) << "," << topology.cy(q) << ") occ" << occupancy.occupied(q)
                              << " mine" << (int)occupancy.isMine(q) << " id" << occupancy.dragonId(q) << " free"
                              << occupancy.freeAt(q);
            }
            std::cout << "\n";
        }
    }
};
#endif
#ifdef DEBUG_STATS
inline void logStats(TurnState const &turn, Topology const &topology) {
    std::clog << "ST " << turn.round << " " << (char)turn.myTeam << " " << turn.length << " " << turn.myId << " "
              << topology.cx(turn.here) << " " << topology.cy(turn.here) << std::endl;
}
#endif

} // namespace bot
