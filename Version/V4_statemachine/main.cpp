// Lifecycle only: observe facts, choose an action, then execute its side effects.
// V3 Scout routes compare all shortest portal paths before executing the chosen first step.
// Combat scoring uses a uniform three-segment shorter-enemy threshold.
// A visible tail can spawn a short same-round attacker under strict conditions.
// Nearby confirmed pearl drops get a short-lived route preference.
#include "observer.hpp"
#include "sonar.hpp"
#include "planner.hpp"
#include "action.hpp"

int main() {
    using namespace bot;
    auto [ct, game] = init();
    GameConfig const config(game);
    TurnState turn;
    WorldModel world(config);
    Topology topology(config, world);
    OccupancyModel occupancy(config, turn);
    BodyMemory body(turn, topology, occupancy);
    Observer observer(turn, world, topology, occupancy, body);
    Search search(config, turn, topology, occupancy);
    Sonar sonar(config, turn, world, topology, occupancy);
    Planner planner(config, turn, world, topology, occupancy, search, body);
#ifdef DEBUG_PORTAL
    PortalDebug debug;
#endif
    Executor executor(ct, turn, world, topology, body
#ifdef DEBUG_PORTAL
                      ,
                      debug
#endif
    );
    while (update(ct, game)) {
        turn.read(ct, game, config);
#ifdef DEBUG_PORTAL
        if (world.pendingPortalKey >= 0) debug.crossing(turn, world, topology);
#endif
        topology.resolvePendingLanding(turn.here);
        topology.confirmExploration(turn.myId, turn.here);
        body.beginTurn();
        sonar.receive(ct);
        observer.observe(ct);
#ifdef DEBUG_STATS
        logStats(turn, topology);
#endif
#ifdef DEBUG_PORTAL
        debug.beginTurn();
        ActionCapture capture;
#endif
        Action action = planner.choose(ct
#ifdef DEBUG_PORTAL
                                       ,
                                       debug
#endif
        );
        action = planner.avoidTeammateHead(action);
        planner.rememberChosenAction(action);
        world.activeClaimPortal = action ? planner.activeClaimPortal() : -1;
        world.nextRallyCell = action.type == ActionType::MOVE
            ? topology.step(turn.here, action.dirs[0]) : -1;
        world.suppressRally = action.type != ActionType::MOVE || world.nextRallyCell < 0;
        world.nextRallyLength = turn.length +
            int(world.nextRallyCell >= 0 && occupancy.visible(world.nextRallyCell) &&
                occupancy.hasPearl(world.nextRallyCell));
        executor.execute(action);
#ifdef DEBUG_PORTAL
        debug.choice(turn, topology);
        capture.finish(turn, topology, occupancy);
#endif
#if FEAT_SONAR
        if (action.type != ActionType::SUICIDE) sonar.send(ct);
#endif
        // Protocol negotiation is per turn; the engine sends ECHOES and
        // accepts 64-bit directional sonar on the following turn.
        std::cout << "PROTOCOL 3\n";
        end_turn();
    }
}
