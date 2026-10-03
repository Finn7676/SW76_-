// Lifecycle only: observe facts, choose an action, then execute its side effects.
// V3 Scout routes compare all shortest portal paths before executing the chosen first step.
// Ordinary combat scoring uses a three-segment threshold; queens assess every enemy.
// Queens are permanent IDs 0/1; sprint free allowance is ceil(start length / 4).
// Queen keeps her length and refuses portals outside a proven dead end.
// The first births stay as escorts; other dragons step out of her ring.
// Ordinary dragons may attack a Queen up to Queen length + 2; round 400 removes that limit.
// A visible trapped friendly Queen may be rescued by a blocking ordinary dragon's suicide.
// A visible tail can spawn a short same-round attacker under strict conditions.
// Nearby confirmed pearl drops get a short-lived route preference.
// V5 sonar broadcasts post-action self length on every protocol-3 turn.
// V5 separates heads from enemies in crowds and permits equal short duels.
// V5 breaks repeated wall states and honors BFS exploration direction.
// V5 portal holds include length in the same packet; trapped heads split to 2 or yield at length 2/3.
// V5 uses payment-aware sprint attacks and shortest safe escapes (at most two paid steps).
// Empty, fully visible single-entry short corridors eat pearls before rescue splits, even with outside enemies.
// Queens avoid every known pocket-entry step; already confined Queens use ordinary pocket salvage.
// Queen exploration uses walking routes and never scouts or crosses portals.
// Safe adjacent pearls precede ordinary navigation, development and density evacuation.
// Witnessed teammate portal passages reserve the known exit until +8 or observed emergence.
// Failed BugNav to pearl clusters can reroute through a matching explored portal entrance.
// V5 closes kelp gaps for density; evacuation resumes after safe adjacent collection.
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
    Search search(config, turn, topology, occupancy, &world);
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
        topology.confirmExploration(turn.myId, turn.here, turn.round);
        body.beginTurn();
        sonar.receive(ct);
        observer.observe(ct);
        sonar.reconcile();
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
        Action plannedAction = action;
        action = planner.finalizeAction(ct, action);
#ifdef DEBUG_FEED
        if (planner.behavior() == BehaviorState::FEED)
            ct.output_log("FEED r", turn.round, " id", turn.myId,
                          action.type == ActionType::SUICIDE ? " donate" : " chase",
                          " target", planner.currentFeedTarget().id);
#endif
        planner.rememberChosenAction(action);
        world.activeClaimPortal = action.type == ActionType::MOVE &&
            plannedAction.type == ActionType::MOVE && action.dirs[0] == plannedAction.dirs[0]
            ? planner.activeClaimPortal() : -1;
        sonar.prepare(action, ct, body, planner.communicationPearlTarget(action),
                      planner.behavior() == BehaviorState::COLLECT);
        executor.execute(action);
#ifdef DEBUG_PORTAL
        debug.choice(turn, topology);
        capture.finish(turn, topology, occupancy);
#endif
#if FEAT_SONAR
        if (action.type != ActionType::SUICIDE) sonar.send(ct);
#endif
        // Negotiation changes the next input to include 64-bit messages and
        // ECHOES; both sonar output syntaxes are accepted by the engine.
        std::cout << "PROTOCOL 3\n";
        end_turn();
    }
}

// Revision 2026-09-29: spawn-heading opening routes, both-axis completion, pearl escape, first-70-round balanced growth.
// Revision 2026-09-29: local feeding from round 400 (inclusive), receiver >= donor+2; donors >18 need population >20.
// Local feeding header fingerprint: 6d1b07a4cc64713fd6498d9270215afb354da6525bdc689a6939080083b2d4ff

// Revision 2026-10-01: portal occupancy leases, next-pearl claims, action-aware sonar and ally intentions.
