#pragma once

#include "body.hpp"
#include "debug.hpp"

namespace bot {

// Action values are defined in core.hpp so Planner need not depend on Executor.
class Executor {
    Controller &ct;
    TurnState const &turn;
    WorldModel &world;
    Topology &topology;
    BodyMemory &body;
#ifdef DEBUG_PORTAL
    PortalDebug &debug;
#endif
  public:
    Executor(Controller &ct, TurnState const &turn, WorldModel &world, Topology &topology, BodyMemory &body
#ifdef DEBUG_PORTAL
             ,
             PortalDebug &debug
#endif
             )
        : ct(ct), turn(turn), world(world), topology(topology), body(body)
#ifdef DEBUG_PORTAL
          ,
          debug(debug)
#endif
    {
    }
    void execute(Action const &action) {
        topology.beginExplorationMove(turn.myId);
        switch (action.type) {
        case ActionType::MOVE:
            doMove(action.dirs[0]);
            break;
        case ActionType::SPRINT:
            doSprint(action);
            break;
        case ActionType::SPLIT:
            ct.do_split(action.splitSize);
            break;
        case ActionType::NONE:
        case ActionType::SUICIDE: // no action is the engine's explicit suicide default
            break;
        }
    }

  private:
    void doMove(int d) {
        topology.recordPortalCrossing(turn.here, d);
        if (topology.edgeTypeAt(turn.here * 4 + d) == E_PORTAL) {
            world.pendingPortalKey = turn.here * 4 + d;
#ifdef DEBUG_PORTAL
            world.learnedExit[world.pendingPortalKey] = -1;
            debug.pendingPred = topology.portalExit(turn.here, d);
#endif
        }
        ct.make_move(Direction(DCH[d]));
    }
    void doSprint(Action const &action) {
        std::string s = "MOVE ";
        int c = turn.here;
        body.beginSprint();
        for (int i = 0; i < action.steps; ++i) {
            int d = action.dirs[i];
            s += DCH[d];
            if (c >= 0 && c != turn.here) body.recordIntermediate(c);
            if (c >= 0) topology.recordPortalCrossing(c, d);
            c = c >= 0 ? topology.step(c, d) : -1;
        }
        world.pendingExplorationLanding = c;
        // Unknown sprint routes cannot be confirmed from a predicted endpoint.
        if (c < 0) world.pendingExplorationKeys.clear();
        std::cout << s << "\n";
    }
};

} // namespace bot
