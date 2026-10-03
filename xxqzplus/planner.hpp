#pragma once

#include "search.hpp"
#include "body.hpp"
#include "roles.hpp"
#include "debug.hpp"
#include "bugnav.hpp"
#include "sprint.hpp"
#include "density.hpp"

namespace bot {

struct UnitProfile {
    int length = 0;
    bool canSplit = false;
    bool championProtected = false;
    bool coreProtected = false;
    bool sacrificeEligible = false;
};

struct Situation {
    bool enemyVisible = false;
    bool enemyRelevant = false;
    bool immediateThreat = false;
    bool mildCrowding = false;
    bool severeBlockedness = false;
    bool pearlsVisible = false;
    bool immediatePearlOpportunity = false;
    bool knownPortals = false;
    bool deathCluesPresent = false;
    int nearbyFriendlyHeads = 0;
};

enum class BehaviorState : uint8_t {
    IDLE,
    EMERGENCY,
    COMBAT,
    DISPERSE,
    EXPAND,
    COLLECT,
    EXPLORE,
    FEED
};

enum class MissionType : uint8_t { NONE, COLLECT_RESOURCE, SCOUT_PORTAL, HUNT_TARGET, PEARL_POCKET };

struct Mission {
    MissionType type = MissionType::NONE;
    int targetCell = -1;
    int targetId = -1;
    int createdRound = -1;
    int lastProgressRound = -1;
    int lastDistance = INF;
};

class Planner {
    struct DecisionContext {
        UnitProfile profile;
        RoleAssignment roles;
        Situation situation;
    };
    // Exact per-turn memoization for the four immediate move candidates.
    // Keep this passive and lazy: no value is computed until a policy asks.
    struct TurnQueryCache {
        std::array<uint8_t, 4> pocketKnown{};
        std::array<PocketInfo, 4> pocket{};
        std::array<std::array<uint8_t, 2>, 4> onwardKnown{};
        std::array<std::array<int, 2>, 4> onward{};
        std::array<std::array<uint8_t, 2>, 4> space12Known{};
        std::array<std::array<int, 2>, 4> space12{};
        void reset() { *this = {}; }
    };

    GameConfig const &config;
    TurnState const &turn;
    WorldModel const &world;
    Topology const &topology;
    OccupancyModel const &occupancy;
    Search &search;
    BodyMemory const &body;
    Coordination coordination;
    SprintModel sprintModel;
    DensityModel densityModel;
    RegionDensity regionDensity;
    std::vector<int> evacuationRoom;
    int evacuationLastProgress=-1, evacuationDistance=INF;
    bool densityEvacuating=false;
    std::vector<uint8_t> isGoal;
    std::vector<int> routeCrowd;
    bool routeCrowdReady = false;
    TurnQueryCache queries;
    BugNav bugNav;
    bool openingInitialized = false, openingActive = false, openingCenterMode = false;
    int openingDirectDir = -1, openingTravelDir = -1;
    bool openingHeadingPolicy = false, openingCrossCenter = true;
    unsigned openingCrossedAxes = 0;
    bool shortPearlEscape = false;
    bool queenRescueSuicide = false;
    Action chosenSprintEscape;
    bool queenDeadEndMode = false;
    mutable std::map<int,bool> queenPocketEdges;
    int shortPearlExpectedCell = -1;
    bool openingPortalCheck = false, openingPortalReturning = false;
    int openingSpawn = -1, openingTarget = -1, openingAxis = -1;
    int openingPortalId = -1, openingPortalSource = -1;
    int openingPortalTarget = -1, openingPortalProgressRound = -1;
    int openingPortalDistance = INF, openingPortalCooldownUntil = -1;
    int openingPearlTarget = -1, openingPearlProgressRound = -1, openingPearlDistance = INF;
    std::map<int, int> openingPearlCooldown;
    Mission mission;
    // Keep the long-range frontier while a temporary pearl task is active.
    Situation currentSituation;
    BehaviorState currentBehavior = BehaviorState::IDLE;
    int claimPortalId = -1;
    int currentHeading = -1;
    bool earlyGrowthSplit = false;
    bool escortLocked = false, escortClaim = false;
    int lastQueenSplitRound = -100;
    int priorityPearlDir = -1;
    int resourceGate = -1, resourceGateDir = -1, resourcePortal = -1, resourceTarget = -1;
    int resourceProgressRound = -1, resourceDistance = INF;
    int blockedCluster = -1, blockedClusterRound = -1, resourceRetryRound = -1;
    BugNav resourceBug{config, turn, topology, occupancy};
    FeedTargetMemory feedTarget;

  public:
    int resourceTransitPortal() const { return resourcePortal; }
    Planner(GameConfig const &config, TurnState const &turn, WorldModel const &world, Topology const &topology,
            OccupancyModel const &occupancy, Search &search, BodyMemory const &body)
        : config(config), turn(turn), world(world), topology(topology), occupancy(occupancy), search(search),
          body(body), coordination(turn, world, topology, occupancy),
          sprintModel(config, turn, topology, occupancy, body, coordination),
          densityModel(config, turn, topology, occupancy, coordination), routeCrowd(config.cells, 0),
          bugNav(config, turn, topology, occupancy) {}
    int activeClaimPortal() const { return claimPortalId; }
    Mission const &currentMission() const { return mission; }
    Situation const &situation() const { return currentSituation; }
    BehaviorState behavior() const { return currentBehavior; }
    RegionDensity const &density() const {return regionDensity;}
    bool evacuatingCrowdedRoom() const {return densityEvacuating;}
    FeedTargetMemory const &currentFeedTarget() const { return feedTarget; }
    bool openingMissionActive() const { return openingActive; }
    int communicationPearlTarget(Action const &action) {
        if (densityEvacuating) return -1;
        if (action.type != ActionType::MOVE) return -1;
        int q = topology.step(turn.here, action.dirs[0]);
        if (q >= 0 && occupancy.visible(q) && occupancy.hasPearl(q) && coordination.pearlAvailable(q)) return q;
        if (currentBehavior != BehaviorState::COLLECT && mission.type != MissionType::COLLECT_RESOURCE &&
            mission.type != MissionType::PEARL_POCKET) return -1;
        int goal = -1;
        search.bfsFirstDir([this](int c) { return coordination.knownPearl(c); },
            [](int, int) { return 0L; }, PEARL_INTENT_HORIZON + 1, 96,
            1u << action.dirs[0], &goal);
        return goal;
    }

    Action avoidCoordinationConflict(Action action, Controller const &ct) {
        if (action.type != ActionType::MOVE && action.type != ActionType::SPRINT) return action;
        int cell = turn.here, risk = 0;
        for (int i = 0; i < action.steps && cell >= 0; ++i) {
            risk += coordination.stepRisk(cell, action.dirs[i], i + 1);
            cell = topology.step(cell, action.dirs[i]);
            if (cell >= 0 && occupancy.visible(cell) && occupancy.hasPearl(cell) &&
                !coordination.pearlAvailable(cell, 1)) risk += 6;
        }
        if (risk == 0 || (cell >= 0 && occupancy.enemyHeadAt(cell))) return action;
        int originalThreat = cell < 0 ? 1 : enemyAttackRisk(cell, turn.length +
            int(occupancy.visible(cell) && occupancy.hasPearl(cell)));
        int bestDir = -1, bestRisk = risk;
        long bestScore = LONG_MIN;
        for (int d = 0; d < 4; ++d) {
            Action candidate = Action::move(d);
            if (!physicallyPossible(candidate) || blocksTeammateEscape(candidate)) continue;
            int q = topology.step(turn.here, d);
            if (q < 0 || occupancy.enemyHeadAt(q)) continue;
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            int candidateRisk = coordination.stepRisk(turn.here, d);
            if (eats && !coordination.pearlAvailable(q)) candidateRisk += 6;
            if (candidateRisk >= risk || enemyAttackRisk(q, turn.length + int(eats)) > originalThreat ||
                !pocketWorthEntering(adjacentPocket(d), d, ct)) continue;
            int space = search.spaceFrom(q, eats, 24, false);
            if (space < std::min(turn.length + 1, 4) || adjacentOnwardSafe(d, eats) == 0) continue;
            long score = long(space) * 12 + int(eats && coordination.pearlAvailable(q)) * 200;
            if (candidateRisk < bestRisk || (candidateRisk == bestRisk && score > bestScore)) {
                bestDir = d; bestRisk = candidateRisk; bestScore = score;
            }
        }
        // A stale/incorrect message must never eliminate the last physical
        // escape or turn an otherwise legal action into implicit suicide.
        return bestDir >= 0 ? Action::move(bestDir) : action;
    }

    Action avoidTeammateHead(Action action) {
        int blockedAllyId = -1;
        bool unsafe = blocksTeammateEscape(action, &blockedAllyId);
        bool headCollision = false;
        if (action.type == ActionType::MOVE || action.type == ActionType::SPRINT) {
            int c = turn.here;
            for (int i = 0; i < action.steps; ++i) {
                c = topology.step(c, action.dirs[i]);
                if (c < 0) break; // an unknown portal exit cannot be inspected
                if (occupancy.teammateHeadAt(c)) headCollision = true;
            }
        }
        if (!unsafe && !headCollision) return action;

        // The final guard normally redirects to a roomier local move instead
        // of turning a rejected planned move into a no-op.
        int bestDir = -1, bestSpace = -1, bestThreat = INT_MAX;
        for (int d = 0; d < 4; ++d) {
            if (topology.edgeTypeAt(turn.here * 4 + d) == E_KELP) continue;
            int q = topology.step(turn.here, d);
            if (q < 0 || occupancy.teammateHeadAt(q) || occupancy.blockedAt(q, 1)) continue;
            Action candidate = Action::move(d);
            if (blocksTeammateEscape(candidate)) continue;
            int threat = enemyAttackRisk(q, turn.length +
                (occupancy.visible(q) && occupancy.hasPearl(q) ? 1 : 0));
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            int space = search.spaceFrom(q, eats, 24, false);
            if (eats) space += 40;
            if (threat < bestThreat || (threat == bestThreat && space > bestSpace)) {
                bestThreat = threat;
                bestSpace = space;
                bestDir = d;
            }
        }
        if (bestDir >= 0) return Action::move(bestDir);
        // If every alternative is gone and this action would trap a shorter
        // ally, let the shorter dragon take the loss. Never choose a direct
        // friendly-head collision, since that can kill both dragons.
        int blockedAllyLengthEstimate = -1;
        if (blockedAllyId >= 0)
            for (size_t i = 0; i < turn.friendHeads.size(); ++i)
                if (occupancy.dragonId(turn.friendHeads[i]) == blockedAllyId &&
                    i < turn.friendHeadLen.size()) {
                    int exact = i < turn.friendHeadExactLen.size() ? turn.friendHeadExactLen[i] : -1;
                    blockedAllyLengthEstimate = exact >= 0 ? exact : turn.friendHeadLen[i];
                }
        if (unsafe && !headCollision && blockedAllyLengthEstimate >= 0 &&
            turn.length > blockedAllyLengthEstimate) return action;
        // If no safe redirection exists, skip an action that would hit a
        // teammate or close its only known escape route.
        return (headCollision || unsafe) ? Action{} : action;
    }

    Action avoidEnemyContact(Action action, Controller const &ct) {
        if (confirmedQueenKill(action) || preservesPriorityPearl(action)) return action;
        if (!multipleVisibleEnemies() || !actionTouchesEnemy(action)) return action;
        int bestDir = -1, bestRisk = INT_MAX, bestSpace = -1;
        for (int d = 0; d < 4; ++d) {
            Action candidate = Action::move(d);
            if (!physicallyPossible(candidate) || actionTouchesEnemy(candidate) || blocksTeammateEscape(candidate)) continue;
            int q = topology.step(turn.here, d);
            if (q < 0) continue; // an unknown landing is not a proven safe redirection
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            int risk = enemyAttackRisk(q, turn.length + int(eats)) * 100 + coordination.stepRisk(turn.here, d);
            int space = search.spaceFrom(q, eats, 24, false) + (eats ? 40 : 0);
            if (risk < bestRisk || (risk == bestRisk && space > bestSpace)) {
                bestDir = d; bestRisk = risk; bestSpace = space;
            }
        }
        if (bestDir >= 0) return Action::move(bestDir);
        if (ct.can_split(2) && body.tailCanEscape()) return recoverySplit(ct);
        // When no separated move or escaping tail exists, keep the last
        // physical escape instead of turning proximity alone into suicide.
        return action;
    }

    // NONE is not a wait action: the engine kills a dragon with no action.
    // Soft tactical vetoes must not discard every physically possible escape.
    bool actionUsesHeldPortal(Action const &action) const {
        if (action.type != ActionType::MOVE && action.type != ActionType::SPRINT) return false;
        int c = turn.here;
        for (int i = 0; i < action.steps && c >= 0; ++i) {
            if (coordination.portalHeld(c, action.dirs[i])) return true;
            c = topology.step(c, action.dirs[i]);
        }
        return false;
    }
    Action avoidPortalHold(Action action, Controller const &ct) {
        if (!actionUsesHeldPortal(action)) return action;
        int originalRisk = 0, c = turn.here;
        for (int i = 0; i < action.steps && c >= 0; ++i) {
            c = topology.step(c, action.dirs[i]);
            originalRisk = std::max(originalRisk, c < 0 ? 1 : enemyAttackRisk(c, turn.length));
        }
        int best = -1, bestRisk = INT_MAX, bestSpace = -1;
        for (int d = 0; d < 4; ++d) {
            Action candidate = Action::move(d);
            if (!physicallyPossible(candidate) || actionUsesHeldPortal(candidate) || blocksTeammateEscape(candidate) ||
                (multipleVisibleEnemies() && actionTouchesEnemy(candidate))) continue;
            int q = topology.step(turn.here, d);
            if (q < 0 || occupancy.enemyHeadAt(q)) continue;
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            int risk = enemyAttackRisk(q, turn.length + int(eats));
            if (risk > originalRisk || !pocketWorthEntering(adjacentPocket(d), d, ct) ||
                adjacentOnwardSafe(d, eats) == 0) continue;
            int space = search.spaceFrom(q, eats, 24, false);
            if (risk < bestRisk || (risk == bestRisk && space > bestSpace)) {
                best = d; bestRisk = risk; bestSpace = space;
            }
        }
        // Waiting means moving safely inside. NONE would kill the dragon.
        // A short-lived request never removes the sole physical escape.
        return best >= 0 ? Action::move(best) : action;
    }
    Action portalHoldEmergency(Controller const &ct) {
        bool confirmed = false;
        for (int d = 0; d < 4; ++d) {
            int senderLength = 0;
            if (coordination.portalHeld(turn.here, d, &senderLength) &&
                senderLength >= Constants::MIN_SIZE) confirmed = true;
        }
        if (!confirmed) return {};
        // An ordinary internal escape remains preferable, including a pearl
        // step that a short-dragon harvest mission can turn into a later split.
        for (int d = 0; d < 4; ++d) {
            Action move = Action::move(d);
            if (!physicallyPossible(move) || coordination.portalHeld(turn.here, d) ||
                blocksTeammateEscape(move)) continue;
            int q = topology.step(turn.here, d);
            if (q < 0) return {}; // another unresolved portal prevents proving confinement
            if (occupancy.enemyHeadAt(q) ||
                (multipleVisibleEnemies() && actionTouchesEnemy(move))) continue;
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            if (enemyAttackRisk(q, turn.length + int(eats)) == 0) return {};
        }
        if (turn.isQueen()) return {}; // a soft portal lease cannot order queen suicide
        if (turn.length <= 3) {
            currentBehavior = BehaviorState::EMERGENCY;
            return Action::suicide(); // collision prevention, independent of late feeding
        }
        int childSize = turn.length - 2;
        Action split = Action::split(childSize);
        if (ct.can_split(childSize) && body.tailCanEscape(true) && !blocksTeammateEscape(split)) {
            currentBehavior = BehaviorState::EMERGENCY;
            return split; // head stays length 2; tail child receives all remaining segments
        }
        return {}; // no known escaping tail: retain ordinary final survival handling
    }
    Action finalizeAction(Controller const &ct, Action action) {
        currentHeading = dirIndex(ct.get_dir());
        refreshQueenDeadEndMode();
        if (turn.isQueen() && currentBehavior==BehaviorState::EXPLORE && !ct.can_split(2)) {
            bool walk=false;
            for(int d=0;d<4;++d)if(physicallyPossible(Action::move(d)))walk=true;
            // No-action kills the dragon. A sole portal escape is an emergency,
            // rather than an exploration decision.
            if(!walk)currentBehavior=BehaviorState::EMERGENCY;
        }
        // This exact route was ranked using its final head/body/length. Keep
        // all physical, ally and portal constraints, but do not reinterpret its
        // intermediate cells as separate enemy turns in the soft final guards.
        bool selectedEscape=currentBehavior==BehaviorState::EMERGENCY && chosenSprintEscape &&
            action.type==chosenSprintEscape.type && action.steps==chosenSprintEscape.steps &&
            action.dirs==chosenSprintEscape.dirs;
        if(selectedEscape && physicallyPossible(action) && sprintModel.trace(action).legal &&
           queenAvoidsNewPocket(action) && !blocksTeammateEscape(action) &&
           !actionUsesHeldPortal(action) && portalExitDebt(action)==0)return action;
        if (queenRescueSuicide && !turn.isQueen() && action.type == ActionType::SUICIDE)
            return action; // certified Queen rescue outranks ordinary soft portal leases
        // Check even when the requested exit was rejected as a visible ally
        // collision: finalizeActionImpl may otherwise return an unrelated fallback.
        bool sprintEscapes = action.type == ActionType::SPRINT && currentBehavior == BehaviorState::EMERGENCY &&
                             !actionUsesHeldPortal(action) && sprintModel.trace(action).legal;
        if (!sprintEscapes) if (Action emergency = portalHoldEmergency(ct)) return emergency;
        return avoidObservedPortalExit(avoidPortalHold(finalizeActionImpl(ct, action), ct),ct);
    }
    Action finalizeActionImpl(Controller const &ct, Action action) {
        currentHeading = dirIndex(ct.get_dir());
        // Opening rescue also covers a known closed corridor with a few
        // empty/pearl cells ahead. Keep SPLIT 2 through the final guard even
        // when one of those temporary moves is still physically possible.
        if (turn.round < 20 && ct.can_split(2) && openingHeadTrapped() &&
            !preservesShortPearlEscape(ct, action) && !preservesPriorityPearl(action)) return avoidEnemyContact(Action::split(2), ct);
        if (turn.isQueen() && action.type == ActionType::SUICIDE) action = {};
        if (action.type == ActionType::SUICIDE) return action; // deliberate late donation
        bool headFree = headHasKnownSafeMove();
        int firstDest = (action.type == ActionType::MOVE || action.type == ActionType::SPRINT)
            ? topology.step(turn.here, action.dirs[0]) : -1;
        bool headTrade = firstDest >= 0 && occupancy.enemyHeadAt(firstDest);
        if (!headFree && !headTrade && ct.can_split(2) && !preservesShortPearlEscape(ct, action) && !preservesPriorityPearl(action))
            return avoidEnemyContact(recoverySplit(ct), ct);
        if (action.type == ActionType::SPLIT &&
            (!ct.can_split(action.splitSize) ||
             !body.tailCanEscape((earlyGrowthSplit && action.splitSize == turn.length / 2) ||
                                 (shortPearlEscape && action.splitSize == 3)))) action = {};
        if (!physicallyPossible(action)) action = {};
        action = avoidTeammateHead(action);
        action = avoidCoordinationConflict(action, ct);
        if (action && physicallyPossible(action)) return avoidEnemyContact(action, ct);
        // A rejected move can leave a usable tail (the round-38 failure).
        if (ct.can_split(2) && body.tailCanEscape()) return avoidEnemyContact(recoverySplit(ct), ct);
        int best = -1, bestRisk = INT_MAX, bestSpace = -1;
        for (int d = 0; d < 4; ++d) {
            Action candidate = Action::move(d);
            if (!physicallyPossible(candidate)) continue;
            int q = topology.step(turn.here, d);
            if (q >= 0 && occupancy.enemyHeadAt(q)) continue;
            int risk = q < 0 ? 1 : enemyAttackRisk(q, turn.length +
                int(occupancy.visible(q) && occupancy.hasPearl(q)));
            risk = risk * 100 + coordination.stepRisk(turn.here, d);
            if (blocksTeammateEscape(candidate)) risk += 10000;
            int space = q < 0 ? 0 : search.spaceFrom(q,
                occupancy.visible(q) && occupancy.hasPearl(q), 24, false);
            if (risk < bestRisk || (risk == bestRisk && space > bestSpace)) {
                best = d; bestRisk = risk; bestSpace = space;
            }
        }
        if (best >= 0) return avoidEnemyContact(Action::move(best), ct);
        if (ct.can_split(2)) return avoidEnemyContact(recoverySplit(ct), ct);
        return {}; // no legal split or non-colliding move exists
    }

    // Called after the final action guard, so a pocket mission is recorded
    // only when the dragon actually commits to entering that pocket.
    void rememberChosenAction(Action const &action) {
        if (openingActive && !openingPortalCheck && openingCrossCenter && !openingCenterMode &&
            (action.type == ActionType::MOVE || action.type == ActionType::SPRINT)) {
            int cell = turn.here;
            for (int i = 0; i < action.steps; ++i) {
                int next = topology.step(cell, action.dirs[i]);
                if (next < 0 || topology.edgeTypeAt(cell * 4 + action.dirs[i]) == E_PORTAL) break;
                // A wrap across a map seam does not cross its center axis.
                if (std::abs(topology.cx(next) - topology.cx(cell)) +
                    std::abs(topology.cy(next) - topology.cy(cell)) == 1)
                    noteOpeningAxes(cell, next);
                cell = next;
            }
        }
        if (shortPearlEscape && (action.type != ActionType::MOVE ||
            topology.step(turn.here, action.dirs[0]) != shortPearlExpectedCell)) shortPearlEscape = false;
        if (densityEvacuating) return; // do not recreate a harvest mission on an exit route
        if (action.type == ActionType::SPLIT && mission.type == MissionType::PEARL_POCKET) {
            mission = {};
            return;
        }
        if (action.type != ActionType::MOVE || mission.type == MissionType::PEARL_POCKET ||
            feedTarget.id >= 0) return;
        int dest = topology.step(turn.here, action.dirs[0]);
        if (dest < 0 || turn.population >= config.targetPopulation || turn.length < 2) return;
        PocketInfo pocket = search.pocketBeyond(dest, turn.here);
        if (!pocket.proven || pocket.pearls < 1 || turn.length + pocket.pearls < 4) return;
        mission = {MissionType::PEARL_POCKET, dest, turn.here, turn.round, turn.round, turn.length};
    }

    // The policy priority is explicit here. Each phase returns no action when inapplicable.
    Action choose(Controller const &ct
#ifdef DEBUG_PORTAL
                  ,
                  PortalDebug &debug
#endif
    ) {
        claimPortalId = -1;
        earlyGrowthSplit = false;
        priorityPearlDir = -1;
        queenRescueSuicide = false;
        chosenSprintEscape = {};
        currentHeading = dirIndex(ct.get_dir());
        refreshQueenDeadEndMode();
        currentBehavior = BehaviorState::IDLE;
        routeCrowdReady = false;
        queries.reset();
        sprintModel.reset();
        updateRegionDensity();
        DecisionContext context;
        refreshEscortClaim();
        context.roles = RolePolicy::assign(config, turn, world);
        context.roles.escort = escortClaim;
        if (escortClaim) {
            context.roles.hunter = false;
            context.roles.scout = false;
            context.roles.sacrificeCandidate = false;
        }
        context.profile = {turn.length, ct.can_split(2), context.roles.champion, context.roles.core,
                           context.roles.sacrificeCandidate};
        context.situation = buildSituation(context);
        currentSituation = context.situation;
        if(turn.isQueen() && mission.type==MissionType::SCOUT_PORTAL)mission={};
        updateMission(context);
        initializeOpening(dirIndex(ct.get_dir()));
        PortalRoute scoutRoute;
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
        unsigned scoutSafeFirst = 0;
#endif
        bool expendable = DonationPolicy::feedingActive(turn) &&
                          turn.length <= V5_FEED_SHORT_LENGTH;
        RamCandidate ram = adjacentRam(context.profile.championProtected && !expendable);

        // Queen targets outrank ordinary length trades, including multi-enemy scenes.
        if (Action a = queenAttack()) {
            currentBehavior = BehaviorState::COMBAT;
            return a;
        }

        if (Action a = queenRescueAction()) {
            queenRescueSuicide = a.type == ActionType::SUICIDE;
            currentBehavior = BehaviorState::EMERGENCY;
            return a;
        }

        // During the first 20 rounds, free a dragon enclosed by its body
        // or a proven kelp dead end by peeling two segments from the tail.
        // Tail visibility/escape is deliberately not a precondition: repeated
        // splits can open space around the head over several turns.
        // An existing resource-pocket mission already has its own harvest
        // and yield-dependent reverse split; do not replace it mid-harvest.
        if ((shortPearlEscape || (mission.type != MissionType::PEARL_POCKET &&
             turn.length >= 3 && turn.length <= 4 && !turn.visPearls.empty())) && openingHeadTrapped()) {
            if (Action a = shortPearlEscapeAction(ct)) return a;
        } else shortPearlEscape = false;
        if (!context.roles.escort)
            if (Action pearl=safeImmediatePearlAction(ct)) {
                priorityPearlDir=pearl.dirs[0];currentBehavior=BehaviorState::COLLECT;return pearl;
            }
        if (turn.round < 20 && ct.can_split(2) && openingHeadTrapped()) return Action::split(2);

        // A profitable head trade is a tactical survival action. Handle it
        // before proximity alone sends the dragon into generic escape scoring,
        // where occupied enemy-head cells are deliberately filtered out.
#if FEAT_RAM_FIRST
        if (ram.good && !context.roles.escort) {
            currentBehavior = BehaviorState::COMBAT;
            return Action::move(ram.dir);
        }
#endif
        if (context.situation.enemyVisible) {
            if (Action a = sprintRam(context)) {
                currentBehavior = BehaviorState::COMBAT;
                return a;
            }
        }

        if (context.situation.enemyVisible) {
            if (Action a = sprintEscape(ct)) {
                currentBehavior = BehaviorState::EMERGENCY;
                return a;
            }
        }

        if (!context.situation.immediateThreat && !context.situation.severeBlockedness) {
            if (Action a = escortGuard()) {
                currentBehavior = BehaviorState::COMBAT;
                return a;
            }
            if (Action a = yieldQueenSpace()) {
                currentBehavior = BehaviorState::DISPERSE;
                return a;
            }
            if (Action a = queenPursuit(ct)) return a;
        }

        // Eligible donors: existing profitable headshot, local feeding/pursuit,
        // then the existing survival/combat/collection decisions.
        if (context.profile.sacrificeEligible)
            if (Action a = sacrifice(ct, context)) return a;

        if (!headHasKnownSafeMove() && ct.can_split(2)) {
            currentBehavior = BehaviorState::EMERGENCY;
            return recoverySplit(ct);
        }

        // A safe density exit also resolves friendly head congestion.
        if (Action a = leaveCrowdedRegion(ct)) return a;

        // P0: immediate survival. Never split for ordinary growth while the
        // head needs an escape move or the only exits are being closed.
        if (context.situation.immediateThreat || context.situation.severeBlockedness) {
            currentBehavior = BehaviorState::EMERGENCY;
            int planDir = context.situation.deathCluesPresent ? deathRouteDirection(ct) : -1;
            return scoreAndFallback(ct, context, ram.dir, planDir, planDir >= 0, true
#ifdef DEBUG_PORTAL
                                    , debug
#endif
            );
        }

        // Early development splits outrank collection and ordinary Scout
        // missions, while the opening rescue and immediate survival above stay intact.
        if (!regionDensity.crowded) if (Action a = earlyGrowthAction(ct)) return a;

        // Honor the short pearl intent advertised after our last action.
        // Survival/combat and growth above may supersede it; prepare() then
        // sends a replacement/cancellation instead of retaining a false claim.
        if (!context.situation.enemyRelevant && world.ownPearlTarget >= 0 &&
            coordination.knownPearl(world.ownPearlTarget)) {
            int distance = INF;
            int dir = search.bfsFirstDir([this](int c) { return c == world.ownPearlTarget; },
                [](int, int) { return 0L; }, PEARL_INTENT_HORIZON, 96, 15, nullptr, &distance);
            if (dir >= 0 && coordination.pearlAvailable(world.ownPearlTarget, distance)) {
                currentBehavior = BehaviorState::COLLECT;
                return Action::move(dir);
            }
        }

        // Large collectors check nearby high-value resources after survival.
        if (!context.profile.sacrificeEligible)
            if (Action a = sacrifice(ct, context)) return a;

        // P1: fight only when an enemy can affect the near-term route, or an
        // immediate attack is already profitable.
        if (context.situation.enemyRelevant) {
            currentBehavior = BehaviorState::COMBAT;
#if FEAT_RAM_FIRST
            if (ram.good && !context.roles.escort) return Action::move(ram.dir);
#endif
            if (Action a = tacticalSplit(ct)) return a;
            if (Action a = sprintPearls(ct, context)) return a;
            int planDir = routeDirection(ct, context);
            return scoreAndFallback(ct, context, ram.dir, planDir, false, false
#ifdef DEBUG_PORTAL
                                    , debug
#endif
            );
        }

        if (Action a = exploredPortalResourceAction(ct)) return a;

        if (Action a = openingPriorityAction(ct, context)) return a;

        // P2: take a safe pearl that is about to be lost or can be collected
        // with one move before another long-term task.
        if (context.situation.immediatePearlOpportunity) {
            currentBehavior = BehaviorState::COLLECT;
            int planDir = routeDirection(ct, context);
            return scoreAndFallback(ct, context, ram.dir, planDir, false, false
#ifdef DEBUG_PORTAL
                                    , debug
#endif
            );
        }

        // Expansion remains reachable during opening travel. A child records
        // its actual spawn position and facing on its own first turn.
        if (openingActive) {
            if (Action a = expansion(ct, context, regionDensity.crowded || !missionAllowsExpansion())) {
                currentBehavior = BehaviorState::EXPAND;
                return a;
            }
            if (Action a = openingNavigation(ct)) return a;
        }

        // P3: separate nearby explorers when several friendly heads compete
        // for the same few cells. Collection and frontier missions survive
        // this one-turn detour; an assigned portal scout keeps its route.
        if (!openingActive && shouldDisperse(context) && mission.type != MissionType::SCOUT_PORTAL &&
            mission.type != MissionType::PEARL_POCKET &&
            !context.situation.enemyRelevant) {
            currentBehavior = BehaviorState::DISPERSE;
            if (Action a = disperse(ct, context)) return a;
        }
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
        bool activeScoutMission = FEAT_V32_SCOUT_MISSION && mission.type == MissionType::SCOUT_PORTAL;
        bool scoutObjectiveAllowed = mission.type != MissionType::PEARL_POCKET &&
                                     (context.roles.scout ||
                                      (activeScoutMission && !context.situation.enemyRelevant));
        if (!turn.isQueen() && !openingActive && scoutObjectiveAllowed && context.situation.knownPortals) {
            prepareRouteCrowd();
            scoutSafeFirst = safeScoutFirstSteps(ct);
            if (activeScoutMission) {
                scoutRoute = search.scoutPortalRoute(scoutSafeFirst,
                                                     [this](int c, int step) { return scoutTileScore(c, step); },
                                                     V3_SCOUT_MAX_PATH, 320, mission.targetId);
            } else {
                scoutRoute = search.scoutPortalRoute(scoutSafeFirst,
                                                     [this](int c, int step) { return scoutTileScore(c, step); });
            }
            if (scoutRoute.dir >= 0) beginScoutMission(scoutRoute);
        }
#endif
        bool missionBlocksExpansion = regionDensity.crowded || scoutRoute.dir >= 0 || !missionAllowsExpansion() ||
                                      context.situation.enemyRelevant;
#if FEAT_V32_SEVERE_BLOCKAGE
        missionBlocksExpansion = missionBlocksExpansion || context.situation.severeBlockedness;
#endif
        if (Action a = expansion(ct, context, missionBlocksExpansion)) {
            currentBehavior = BehaviorState::EXPAND;
            return a;
        }
        if (context.roles.scout || mission.type == MissionType::SCOUT_PORTAL) {
            currentBehavior = BehaviorState::EXPLORE;
        } else if (mission.type == MissionType::COLLECT_RESOURCE || hasPearlTarget()) {
            currentBehavior = BehaviorState::COLLECT;
        } else {
            currentBehavior = BehaviorState::EXPLORE;
        }
        return routePhase(ct, context, ram, scoutRoute
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
                          , scoutSafeFirst
#else
                          , 0
#endif
#ifdef DEBUG_PORTAL
                          , debug
#endif
        );
    }

  private:
    void updateRegionDensity() {
        densityModel.reset();regionDensity={};
#if FEAT_REGION_DENSITY
        regionDensity=densityModel.estimate(turn.here);
        if (densityEvacuating && (std::find(evacuationRoom.begin(),evacuationRoom.end(),turn.here)==evacuationRoom.end() ||
            turn.round-evacuationLastProgress>8)) {
            densityEvacuating=false;evacuationRoom.clear();
        }
        if (!densityEvacuating && regionDensity.shouldLeave) {
            densityEvacuating=true;evacuationRoom=regionDensity.cells;
            evacuationLastProgress=turn.round;evacuationDistance=INF;
            shortPearlEscape=false;
            if (mission.type==MissionType::COLLECT_RESOURCE || mission.type==MissionType::PEARL_POCKET) mission={};
        }
#endif
    }
    Action leaveCrowdedRegion(Controller const &ct) {
#if FEAT_REGION_DENSITY
        if (!densityEvacuating) return {};
        auto allowed=[&](Action const &a) {
            if (!physicallyPossible(a)||blocksTeammateEscape(a)||actionUsesHeldPortal(a))return false;
            int q=topology.step(turn.here,a.dirs[0]);
            if(q<0||occupancy.enemyHeadAt(q)||(multipleVisibleEnemies()&&actionTouchesEnemy(a)))return false;
            int grown=turn.length+int(occupancy.visible(q)&&occupancy.hasPearl(q));
            return enemyAttackRisk(q,grown)==0;
        };
        auto route=densityModel.exitRoute(evacuationRoom,allowed);
        if(route.dir>=0) {
            if(route.distance<evacuationDistance)evacuationLastProgress=turn.round;
            evacuationDistance=route.distance;currentBehavior=BehaviorState::DISPERSE;
            return Action::move(route.dir);
        }
        // A temporarily occupied exit must not hand priority back to pearl
        // collection or growth. Approach it by a safe move without pearl reward.
        auto distance=densityModel.exitDistances(evacuationRoom);
        int best=-1;long bestScore=LONG_MIN;
        for(int d=0;d<4;++d) {
            Action a=Action::move(d);if(!allowed(a))continue;
            int q=topology.step(turn.here,d);bool eats=occupancy.visible(q)&&occupancy.hasPearl(q);
            int space=search.spaceFrom(q,eats,24,false);
            long score=(distance[q]<INF?-long(distance[q])*1000L:0L)+long(space)*10L-
                long(coordination.stepRisk(turn.here,d))*100L-(eats?50L:0L);
            if(score>bestScore){best=d;bestScore=score;}
        }
        if(best>=0){currentBehavior=BehaviorState::DISPERSE;return Action::move(best);}
#else
        (void)ct;
#endif
        (void)ct;return {};
    }

    static constexpr int SCOUT_MISSION_STALE_ROUNDS = 12;

    struct RamCandidate {
        int dir;
        bool good;
    };

    bool multipleVisibleEnemies() const {
        int first = -1;
        for (auto const &part : turn.parts) {
            if (part.mine) continue;
            if (first < 0) first = part.id;
            else if (first != part.id) return true;
        }
        return false;
    }
    bool enemyTouchesCell(int cell) const {
        if (cell < 0) return false;
        for (auto const &part : turn.parts)
            if (!part.mine && topology.manh(cell, part.c) <= 1) return true;
        return false;
    }
    bool actionTouchesEnemy(Action const &action) const {
        if (action.type == ActionType::SPLIT)
            return enemyTouchesCell(turn.here) || enemyTouchesCell(body.cellAtTailOffset(0));
        if (action.type != ActionType::MOVE && action.type != ActionType::SPRINT) return false;
        int cell = turn.here;
        for (int i = 0; i < action.steps; ++i) {
            cell = topology.step(cell, action.dirs[i]);
            if (cell < 0) break;
            if (enemyTouchesCell(cell)) return true;
        }
        return false;
    }
    bool equalShortEnemy(int head) const {
        if (turn.length < Constants::MIN_SIZE || turn.length > 4 || multipleVisibleEnemies()) return false;
        bool visibleAlly = false;
        for (auto const &part : turn.parts)
            if (part.mine && part.id != turn.myId) { visibleAlly = true; break; }
        if (!visibleAlly) return false;
        for (size_t i = 0; i < turn.enemyHeads.size(); ++i)
            if (turn.enemyHeads[i] == head && i < turn.enemyHeadExactLen.size())
                return turn.enemyHeadExactLen[i] == turn.length;
        return false;
    }

    bool blocksTeammateEscape(Action const &action, int *shortestBlockedAllyId = nullptr) const {
        if (shortestBlockedAllyId) *shortestBlockedAllyId = -1;
        if (action.type != ActionType::SPLIT && action.type != ActionType::MOVE &&
            action.type != ActionType::SPRINT) return false;
        std::array<int, 8> entered;
        entered.fill(-1);
        int enteredCount = 0;
        int finalCell = turn.here;
        if (action.type == ActionType::SPLIT) {
            int childHead = body.cellAtTailOffset(0);
            if (childHead < 0) return false;
            entered[enteredCount++] = childHead;
        } else {
            for (int i = 0; i < action.steps && i < int(entered.size()); ++i) {
                finalCell = topology.step(finalCell, action.dirs[i]);
                if (finalCell < 0) return false; // unknown portal landing: do not infer a blockage
                entered[enteredCount++] = finalCell;
            }
            // A completed head trade removes our body before any teammate acts;
            // the attack's temporary path cells do not remain as blockers.
            if (occupancy.enemyHeadAt(finalCell)) return false;
        }

        bool blocksEscape = false;
        for (auto const &ally : turn.parts) {
            if (!ally.mine || !ally.head || ally.id <= turn.myId) continue; // only allies yet to act
            int soleExit = -1, exits = 0;
            bool uncertain = false;
            for (int d = 0; d < 4; ++d) {
                uint8_t edge = topology.edgeTypeAt(ally.c * 4 + d);
                if (edge == E_KELP) continue;
                if (edge != E_OPEN) { uncertain = true; continue; }
                int q = topology.step(ally.c, d);
                if (q < 0) { uncertain = true; continue; }
                if (!occupancy.blockedAt(q, 2)) { soleExit = q; ++exits; }
            }
            // Preserve the original immediate sole-exit guard.
            if (!uncertain && exits == 1 &&
                std::find(entered.begin(), entered.begin() + enteredCount, soleExit) !=
                    entered.begin() + enteredCount) {
                blocksEscape = true;
                if (shortestBlockedAllyId &&
                    (*shortestBlockedAllyId < 0 || turn.countOf(ally.id) < turn.countOf(*shortestBlockedAllyId)))
                    *shortestBlockedAllyId = ally.id;
                continue;
            }

            auto hasTwoStepEscape = [&](bool includeOurAction) {
                bool uncertain = false;
                for (int d1 = 0; d1 < 4; ++d1) {
                    uint8_t edge1 = topology.edgeTypeAt(ally.c * 4 + d1);
                    if (edge1 == E_KELP) continue;
                    if (edge1 != E_OPEN) { uncertain = true; continue; }
                    int q1 = topology.step(ally.c, d1);
                    if (q1 < 0) { uncertain = true; continue; }
                    if (q1 == ally.c || occupancy.blockedAt(q1, 2) ||
                        (includeOurAction && std::find(entered.begin(), entered.begin() + enteredCount, q1) !=
                                                 entered.begin() + enteredCount)) continue;

                    for (int d2 = 0; d2 < 4; ++d2) {
                        uint8_t edge2 = topology.edgeTypeAt(q1 * 4 + d2);
                        if (edge2 == E_KELP) continue;
                        if (edge2 != E_OPEN) { uncertain = true; continue; }
                        int q2 = topology.step(q1, d2);
                        if (q2 < 0) { uncertain = true; continue; }
                        // The ally's current head and its first-step cell become body
                        // segments after moving, so it cannot use either as step two.
                        if (q2 == ally.c || q2 == q1 || occupancy.blockedAt(q2, 3) ||
                            (includeOurAction && std::find(entered.begin(), entered.begin() + enteredCount, q2) !=
                                                     entered.begin() + enteredCount)) continue;
                        return std::pair<bool, bool>{true, false};
                    }
                }
                return std::pair<bool, bool>{false, uncertain};
            };
            auto before = hasTwoStepEscape(false);
            if (!before.first) continue; // no known two-step escape to take away
            auto after = hasTwoStepEscape(true);
            if (!after.first && !after.second) {
                blocksEscape = true;
                if (shortestBlockedAllyId &&
                    (*shortestBlockedAllyId < 0 || turn.countOf(ally.id) < turn.countOf(*shortestBlockedAllyId)))
                    *shortestBlockedAllyId = ally.id;
            }
        }
        return blocksEscape;
    }

    void refreshQueenDeadEndMode() {
        queenDeadEndMode = false;
        queenPocketEdges.clear();
        if (!turn.isQueen() || turn.length < 2) return;
        int neck = body.cellAtTailOffset(turn.length-2);
        if (neck < 0) return;
        for (int d = 0; d < 4; ++d)
            if (topology.edgeTypeAt(turn.here*4+d)==E_OPEN && topology.step(turn.here,d)==neck) {
                // Cutting the neck edge isolates the forward terrain component.
                // A proven one-entrance pocket means we are already inside.
                queenDeadEndMode = search.pocketBeyond(turn.here,neck,config.cells).proven;
                return;
            }
    }
    bool queenAvoidsNewPocket(Action const &action) const {
        if (!turn.isQueen() || queenDeadEndMode ||
            (action.type != ActionType::MOVE && action.type != ActionType::SPRINT)) return true;
        int from = turn.here;
        for (int step = 0; step < action.steps; ++step) {
            int d = action.dirs[step], next = topology.step(from,d);
            if (next < 0) return true; // no known landing to classify; physical guard handles it
            int key = from*4+d;
            auto found = queenPocketEdges.find(key);
            if (found == queenPocketEdges.end())
                found = queenPocketEdges.emplace(key,search.pocketBeyond(next,from,config.cells).proven).first;
            if (found->second) return false;
            from = next;
        }
        return true;
    }
    int portalExitDebt(Action const &action) const {
        std::vector<int> cells;
        for(int n=0;n<turn.length;++n)cells.push_back(n==0?turn.here:body.cellAtTailOffset(turn.length-1-n));
        if(action.type==ActionType::SUICIDE)return 0;
        if(action.type==ActionType::MOVE || action.type==ActionType::SPRINT) {
            int head=turn.here; std::set<int> eaten;
            for(int n=0;n<action.steps;++n) {
                int q=topology.step(head,action.dirs[n]);if(q<0)break;
                bool eats=occupancy.visible(q)&&occupancy.hasPearl(q)&&eaten.insert(q).second;
                if(!eats&&!cells.empty())cells.pop_back();cells.insert(cells.begin(),q);
                if(MovementRules::paidStep(turn.length,n)&&!cells.empty())cells.pop_back();head=q;
            }
        }
        int debt=0;
        for(size_t n=0;n<cells.size();++n)if(coordination.protectedPortalExit(cells[n]))
            debt=std::max(debt,int(cells.size()-n));
        return debt;
    }
    Action avoidObservedPortalExit(Action action,Controller const &ct) {
        int debt=portalExitDebt(action);if(debt==0)return action;
        Action best;int bestDebt=debt,bestRisk=INF;
        int originalRisk=0;
        if(action.type==ActionType::MOVE || action.type==ActionType::SPRINT) {
            int q=topology.step(turn.here,action.dirs[0]);
            originalRisk=q<0?INF:enemyAttackRisk(q,turn.length+int(occupancy.hasPearl(q)));
        }
        for(int d=0;d<4;++d) {
            Action move=Action::move(d);if(!physicallyPossible(move)||blocksTeammateEscape(move)||actionUsesHeldPortal(move))continue;
            int q=topology.step(turn.here,d);if(q<0||occupancy.enemyHeadAt(q))continue;
            int nextDebt=portalExitDebt(move),risk=enemyAttackRisk(q,turn.length+int(occupancy.hasPearl(q)));
            if(risk>originalRisk || coordination.stepRisk(turn.here,d)>0)continue;
            if(nextDebt<bestDebt || (best && nextDebt==bestDebt && risk<bestRisk)) {
                best=move;bestDebt=nextDebt;bestRisk=risk;
            }
        }
        (void)ct;
        // If every move still covers the exit, retain a legal escape. Never
        // replace the last physical action with implicit suicide.
        return best?best:action;
    }
    bool preservesPriorityPearl(Action const &a) {
        if(priorityPearlDir<0 || a.type!=ActionType::MOVE || a.steps!=1 || a.dirs[0]!=priorityPearlDir)return false;
        int q=topology.step(turn.here,a.dirs[0]);
        return q>=0 && physicallyPossible(a) && occupancy.visible(q) && occupancy.hasPearl(q) &&
            coordination.pearlAvailable(q) && portalExitDebt(a)==0 && sprintModel.distanceToEnemyReach(q)>0;
    }
    Action safeImmediatePearlAction(Controller const &ct) {
        if (openingHeadTrapped()) return {}; // dedicated short-pocket harvest proves the reverse split
        int best=-1,bestSpace=-1;
        for(int d=0;d<4;++d) {
            Action a=Action::move(d);int q=topology.step(turn.here,d);
            if(q<0 || !occupancy.visible(q) || !occupancy.hasPearl(q) || !physicallyPossible(a) ||
               occupancy.occupied(q) || !coordination.pearlAvailable(q) || coordination.stepRisk(turn.here,d)>0 ||
               blocksTeammateEscape(a) || portalExitDebt(a)>0 || sprintModel.distanceToEnemyReach(q)==0)continue;
            if(!pocketWorthEntering(adjacentPocket(d),d,ct))continue;
            int space=search.spaceFrom(q,true,24,false);
            bool escapeSplit=turn.length+1>=4 && ct.unit_count<ct.unit_limit && body.tailCanEscape(true);
            if(adjacentOnwardSafe(d,true)==0 && !escapeSplit)continue;
            if(space>bestSpace){best=d;bestSpace=space;}
        }
        return best>=0?Action::move(best):Action{};
    }
    bool atKnownDeadEnd() const {
        bool uncertain = false;
        int ownBodyNeighbors = 0, freeExits = 0;
        for (int d = 0; d < 4; ++d) {
            uint8_t edge = topology.edgeTypeAt(turn.here * 4 + d);
            if (edge == E_KELP) continue;
            if (edge != E_OPEN) { uncertain = true; continue; }
            int q = topology.step(turn.here, d);
            if (q < 0) { uncertain = true; continue; }
            if (occupancy.occupied(q) && occupancy.isMine(q) && occupancy.dragonId(q) == turn.myId)
                ++ownBodyNeighbors;
            else
                ++freeExits;
        }
        return !uncertain && ownBodyNeighbors > 0 && freeExits == 0;
    }
    bool backThroughHiddenNeck(int d) const {
        // Reversing the arrival direction goes into the neck even when that
        // neck is behind a portal whose landing has not been learned yet.
        return turn.length >= 2 && currentHeading >= 0 && d == (currentHeading ^ 2) &&
            (topology.edgeTypeAt(turn.here * 4 + d) == E_PORTAL ||
             body.cellAtTailOffset(turn.length - 2) < 0);
    }
    // A few free cells ahead do not make a closed kelp corridor escapable.
    // Prove the terrain pocket before counting it as trapped; an unknown
    // edge, portal, cycle, or second entrance keeps the result uncertain.
    bool openingHeadTrapped() const {
        for (int d = 0; d < 4; ++d) {
            if (backThroughHiddenNeck(d)) continue;
            uint8_t edge = topology.edgeTypeAt(turn.here * 4 + d);
            if (edge == E_KELP) continue;
            if (edge != E_OPEN) return false;
            int q = topology.step(turn.here, d);
            if (q < 0 || occupancy.enemyHeadAt(q)) return false;
            if (occupancy.blockedAt(q, 1)) continue;
            if (!search.pocketBeyond(q, turn.here).proven) return false;
        }
        return true;
    }
    // A currently short dragon with an escaping tail may postpone rescue
    // to harvest a fully visible, unbranched dead end. Empty cells do not
    // reduce length; only the pearl yield must fund a three-segment child.
    // Every corridor cell must be visible and empty. Its sole entrance stays
    // occupied by our retained neck after a move, so an outside enemy cannot
    // reach the new head without hitting our body, even if visible across kelp.
    int shortPearlCorridorDir() {
        for (int d = 0; d < 4; ++d) {
            Action move = Action::move(d);
            if (!physicallyPossible(move) || blocksTeammateEscape(move)) continue;
            int cell = topology.step(turn.here, d), previous = turn.here;
            if (cell >= 0 && occupancy.hasPearl(cell) && !coordination.pearlAvailable(cell)) continue;
            if (cell < 0 || !occupancy.visible(cell)) continue;
            PocketInfo pocket = search.pocketBeyond(cell, turn.here);
            if (!pocket.proven || pocket.cells > 3 || pocket.pearls < 1 ||
                turn.length + pocket.pearls < 5) continue;
            bool corridor = true;
            for (int i = 0; i < pocket.cells; ++i) {
                if (!occupancy.visible(cell) || occupancy.occupied(cell)) {
                    corridor = false; break;
                }
                int next = -1;
                for (int k = 0; k < 4; ++k) {
                    if (topology.edgeTypeAt(cell * 4 + k) != E_OPEN) continue;
                    int q = topology.step(cell, k);
                    if (q == previous) continue;
                    if (next >= 0) { corridor = false; break; }
                    next = q;
                }
                if (!corridor || (i + 1 < pocket.cells && next < 0)) { corridor = false; break; }
                previous = cell; cell = next;
            }
            if (corridor) return d;
        }
        return -1;
    }
    bool preservesShortPearlEscape(Controller const &ct, Action const &action) {
        if (!shortPearlEscape || ct.unit_count >= ct.unit_limit || !body.tailCanEscape(true)) return false;
        if (action.type == ActionType::SPLIT)
            return action.splitSize == 3 && ct.can_split(3) && openingHeadTrapped();
        return action.type == ActionType::MOVE && action.dirs[0] == shortPearlCorridorDir();
    }
    Action shortPearlEscapeAction(Controller const &ct) {
        if (shortPearlEscape && turn.here != shortPearlExpectedCell) {
            shortPearlEscape = false;
            return {};
        }
        if ((turn.isQueen() && !queenDeadEndMode) || (!shortPearlEscape && (turn.length < 3 || turn.length > 4)) ||
            ct.unit_count >= ct.unit_limit ||
            // Start only with a visible escape; retain its known tail route
            // when pearl growth moves the tail beyond the vision window.
            !body.tailCanEscape(shortPearlEscape)) {
            shortPearlEscape = false;
            return {};
        }
        int dir = shortPearlCorridorDir();
        if (dir >= 0) {
            shortPearlEscape = true;
            shortPearlExpectedCell = topology.step(turn.here, dir);
            currentBehavior = BehaviorState::COLLECT;
            return Action::move(dir);
        }
        // Once the remaining pocket has no useful pearls, split without
        // walking into empty trailing cells just to reach the terrain end.
        if (shortPearlEscape && ct.can_split(3) && openingHeadTrapped()) {
            currentBehavior = BehaviorState::EMERGENCY;
            return Action::split(3);
        }
        shortPearlEscape = false;
        return {};
    }
    Action recoverySplit(Controller const &ct) const {
#if FEAT_REVERSE_SPLIT
        if ((!turn.isQueen() || queenDeadEndMode) && turn.round >= 20 && body.tailCanEscape(true) && turn.length >= 5 &&
            ct.can_split(turn.length - 2)) return Action::split(turn.length - 2);
#endif
        return ct.can_split(2) ? Action::split(2) : Action{};
    }
    bool queenExplorationAllows(Action const &action) const {
        if(!turn.isQueen() || currentBehavior!=BehaviorState::EXPLORE ||
           (action.type!=ActionType::MOVE && action.type!=ActionType::SPRINT))return true;
        int c=turn.here;
        for(int n=0;n<action.steps && c>=0;++n) {
            int d=action.dirs[n];if(d<0 || d>=4)return false;
            if(topology.edgeTypeAt(c*4+d)==E_PORTAL)return false;
            c=topology.step(c,d);
        }
        return true;
    }
    bool physicallyPossible(Action const &action) const {
        if (action.type == ActionType::SPLIT) return true;
        if (action.type != ActionType::MOVE && action.type != ActionType::SPRINT) return false;
        if (!queenExplorationAllows(action) || !queenAvoidsNewPocket(action)) return false;
        int d = action.dirs[0];
        if (backThroughHiddenNeck(d)) return false;
        if (d < 0 || d >= 4 || topology.edgeTypeAt(turn.here * 4 + d) == E_KELP) return false;
        if (turn.isQueen() && !queenDeadEndMode && topology.edgeTypeAt(turn.here * 4 + d) == E_PORTAL)
            return false;
        if (action.type == ActionType::SPRINT) {
            auto trace = sprintModel.trace(action, true);
            return trace.legal && !(turn.isQueen() && trace.collision);
        }
        int q = topology.step(turn.here, d);
        if (q < 0) return topology.edgeTypeAt(turn.here * 4 + d) == E_PORTAL;
        return !occupancy.teammateHeadAt(q) && !(turn.isQueen() && occupancy.enemyHeadAt(q)) &&
            (!occupancy.blockedAt(q, 1) || occupancy.enemyHeadAt(q));
    }
    bool headHasKnownSafeMove() const {
        for (int d = 0; d < 4; ++d) {
            if (backThroughHiddenNeck(d)) continue;
            uint8_t edge = topology.edgeTypeAt(turn.here * 4 + d);
            if (edge == E_KELP) continue;
            if (edge == E_UNKNOWN) return true;
            int q = topology.step(turn.here, d);
            if (q < 0) return true; // unresolved portals are uncertain exits
            if (!occupancy.teammateHeadAt(q) && !occupancy.blockedAt(q, 1)) return true;
        }
        return false;
    }
    Situation buildSituation(DecisionContext const &context) const {
        (void)context;
        Situation s;
        s.enemyVisible = !turn.enemyHeads.empty();
        s.pearlsVisible = !turn.visPearls.empty();
        s.knownPortals = !world.portalKeys.empty();
        s.deathCluesPresent = !world.deathClues.empty();
        for (int c : turn.visPearls)
            if (coordination.pearlAvailable(c) && topology.manh(turn.here, c) <= 1) s.immediatePearlOpportunity = true;
        int legal = 0, friendlyBlocked = 0;
        for (int d = 0; d < 4; ++d) {
            if (topology.edgeTypeAt(turn.here * 4 + d) == E_KELP) continue;
            int q = topology.step(turn.here, d);
            if (q < 0 || !occupancy.blockedAt(q, 1)) ++legal;
            if (q >= 0 && occupancy.teammateHeadAt(q)) ++friendlyBlocked;
        }
        s.severeBlockedness = legal <= 1 && friendlyBlocked > 0;
        for (auto const &part : turn.parts) {
            if (part.mine && part.head && part.id != turn.myId && topology.cheb(turn.here, part.c) <= 3)
                ++s.nearbyFriendlyHeads;
        }
        for (int f : turn.friendParts)
            if (topology.cheb(turn.here, f) <= 3) s.mildCrowding = true;
        for (size_t i = 0; i < turn.enemyHeads.size(); ++i) {
            int distance = topology.manh(turn.here, turn.enemyHeads[i]);
            bool close = distance <= 3;
            bool confirmedShort = false;
            if (i < turn.enemyHeadExactLen.size()) {
                int exact = turn.enemyHeadExactLen[i];
                confirmedShort = exact >= 0 && exact <= turn.length - 3;
            }
            // enemyHeadLen is only a visible lower bound.  It cannot prove
            // that an enemy is short; only an exact reconstructed length can.
            if (close || ((confirmedShort || turn.isQueen()) && distance <= 8)) s.enemyRelevant = true;
            if (distance <= 1 || ((confirmedShort || turn.isQueen()) && distance <= 2)) s.immediateThreat = true;
        }
#if FEAT_V32_ENEMY_RELEVANCE
#else
        s.enemyRelevant = s.enemyVisible;
        s.immediateThreat = s.enemyVisible;
#endif
        return s;
    }
    bool hasPearlTarget() const {
        for (int c : turn.visPearls)
            if (coordination.pearlAvailable(c)) return true;
        for (int c = 0; c < config.cells; ++c) {
            int due = world.pearlDue[c];
            if (world.pearlInfo[c] >= turn.round - 6 && due >= turn.round && due - turn.round <= 8 &&
                coordination.pearlAvailable(c))
                return true;
        }
        return false;
    }
    bool shouldDisperse(DecisionContext const &context) const {
        if (context.roles.escort) return false;
        return context.situation.nearbyFriendlyHeads >= 2 ||
               (context.situation.nearbyFriendlyHeads == 1 &&
                closestFriendlyHead() <= 1);
    }
    int closestFriendlyHead() const {
        int best = INF;
        for (auto const &part : turn.parts)
            if (part.mine && part.head && part.id != turn.myId)
                best = std::min(best, topology.manh(turn.here, part.c));
        return best;
    }
    Action disperse(Controller const &ct, DecisionContext const &context) {
        int currentDistance = closestFriendlyHead();
        if (currentDistance >= INF) return {};
        int bestDir = -1;
        long bestScore = LONG_MIN;
        for (int d = 0; d < 4; ++d) {
            if (topology.edgeTypeAt(turn.here * 4 + d) == E_KELP) continue;
            int dest = topology.step(turn.here, d);
            if (dest < 0 || occupancy.teammateHeadAt(dest) || occupancy.blockedAt(dest, 1)) continue;
            int separation = INF;
            int crowd = 0;
            for (auto const &part : turn.parts) {
                if (!part.mine || !part.head || part.id == turn.myId) continue;
                int distance = topology.manh(dest, part.c);
                separation = std::min(separation, distance);
                if (topology.cheb(dest, part.c) <= 3) ++crowd;
            }
            if (separation <= currentDistance) continue;
            bool eats = occupancy.visible(dest) && occupancy.hasPearl(dest);
            if (!pocketWorthEntering(adjacentPocket(d), d, ct)) continue;
            int space = search.spaceFrom(dest, eats, 24, false);
            long score = (long)separation * 180L + (long)space * 12L - (long)crowd * 100L;
            if (eats) score += 300;
            if (context.situation.enemyRelevant && shortEnemyRiskDistance(dest) <= 2) score -= 1200;
            if (score > bestScore) { bestScore = score; bestDir = d; }
        }
        return bestDir >= 0 ? Action::move(bestDir) : Action{};
    }
    void beginScoutMission(PortalRoute const &route) {
#if FEAT_V32_SCOUT_MISSION
        if (route.pid < 0) return;
        if (mission.type != MissionType::SCOUT_PORTAL || mission.targetId != route.pid) {
            mission = {MissionType::SCOUT_PORTAL, -1, route.pid, turn.round, turn.round, route.distance};
        } else {
            // Progress means actually getting closer to the committed target.
            // Merely finding the same route again must not refresh staleness.
            // Keep the latest route distance as the comparison baseline so a
            // temporary detour/obstacle does not make future progress invisible.
            if (route.distance < mission.lastDistance) mission.lastProgressRound = turn.round;
            mission.lastDistance = route.distance;
        }
#else
        (void)route;
#endif
    }
    void updateMission(DecisionContext const &context) {
#if FEAT_V32_SCOUT_MISSION
        if (mission.type == MissionType::HUNT_TARGET) {
            auto const &q = world.enemyQueen;
            if (q.c < 0 || q.round > turn.round || turn.round - q.round > 3 ||
                !turn.mayAttackQueen(q.length)) mission = {};
        } else if (mission.type == MissionType::PEARL_POCKET) {
            if (turn.length > mission.lastDistance) {
                mission.lastDistance = turn.length;
                mission.lastProgressRound = turn.round;
            }
            bool stale = mission.lastProgressRound >= 0 && turn.round - mission.lastProgressRound > 12;
            if (stale || turn.population >= config.targetPopulation) mission = {};
        } else if (mission.type == MissionType::SCOUT_PORTAL) {
            // Hunter is a transient per-turn label and must not destroy a
            // persistent Scout mission.  Tactical behavior may interrupt the
            // mission for a turn and it can resume later.
            bool invalid = mission.targetId < 0 || context.roles.champion ||
                           context.roles.core || turn.round > (config.cells >= 1200 ? 300 : 220);
            bool complete = world.exploredPortals.count(mission.targetId) != 0 || coordination.portalBusy(mission.targetId);
            bool stale = mission.lastProgressRound >= 0 &&
                         turn.round - mission.lastProgressRound > SCOUT_MISSION_STALE_ROUNDS;
            if (invalid || complete || stale) mission = {};
        } else if (mission.type == MissionType::COLLECT_RESOURCE) {
            bool valid = mission.targetCell >= 0 && mission.targetCell < config.cells &&
                         mission.targetCell != turn.here;
            if (valid && occupancy.visible(mission.targetCell))
                valid = occupancy.hasPearl(mission.targetCell);
            else if (valid)
                valid = world.pearlInfo[mission.targetCell] >= turn.round - 3 &&
                        world.pearlDue[mission.targetCell] <= turn.round;
            bool stale = mission.lastProgressRound >= 0 &&
                         turn.round - mission.lastProgressRound > SCOUT_MISSION_STALE_ROUNDS;
            if (!valid || stale || (valid && !coordination.pearlAvailable(mission.targetCell,
                std::max(1, mission.lastDistance)))) mission = {};
        }
#else
        (void)context;
#endif
    }
    int openingInwardDir(int cell, int axis) const {
        if (axis == 0) return 2 * topology.cx(cell) <= config.width - 1 ? 1 : 3;
        return 2 * topology.cy(cell) <= config.height - 1 ? 2 : 0;
    }
    void initializeOpening(int initialDir) {
        if (!openingInitialized) {
            openingInitialized = true;
            if (turn.round < 0 || turn.round >= 60) return;
            openingActive = true;
            openingSpawn = turn.here;
            openingHeadingPolicy = turn.round < 3;
            int sx = topology.cx(turn.here), sy = topology.cy(turn.here);
            openingTarget = topology.cid(config.width - 1 - sx, config.height - 1 - sy);
            if (config.width > config.height) openingAxis = 0;
            else if (config.height > config.width) openingAxis = 1;
            else {
                int dx = std::abs(2 * sx - (config.width - 1));
                int dy = std::abs(2 * sy - (config.height - 1));
                openingAxis = dx > dy ? 0 : dy > dx ? 1 : (turn.myId & 1);
            }
            int headingAxis = initialDir == 1 || initialDir == 3 ? 0 : 1;
            openingDirectDir = openingInwardDir(turn.here, openingHeadingPolicy ? headingAxis : openingAxis);
            openingTravelDir = openingHeadingPolicy ? initialDir : -1;
            openingCrossCenter = !openingHeadingPolicy || initialDir == openingDirectDir;
        }
        if (openingActive) observeOpeningPortalArrival();
        if (turn.round >= 60) openingActive = false;
        if (openingActive && !openingPortalCheck && openingTravelComplete()) openingActive = false;
        auto last = openingPearlCooldown.begin();
        while (last != openingPearlCooldown.end()) {
            if (last->second < turn.round) last = openingPearlCooldown.erase(last);
            else ++last;
        }
    }
    bool inTargetBox(int radius) const {
        if (openingTarget < 0) return false;
        return topology.wrapd(topology.cx(turn.here) - topology.cx(openingTarget), config.width) <= radius &&
               topology.wrapd(topology.cy(turn.here) - topology.cy(openingTarget), config.height) <= radius;
    }
    void noteOpeningAxes(int from, int to) {
        int before[2] = {topology.cx(from), topology.cy(from)};
        int after[2] = {topology.cx(to), topology.cy(to)};
        int dimensions[2] = {config.width, config.height};
        for (int axis = 0; axis < 2; ++axis) {
            // Travelling along an axis, including being born on it, does not
            // count as crossing it. Keep each transverse crossing separately.
            if (before[axis] == after[axis]) continue;
            int first = 2 * before[axis] - (dimensions[axis] - 1);
            int last = 2 * after[axis] - (dimensions[axis] - 1);
            if (std::abs(first) <= 1 || std::abs(last) <= 1) openingCrossedAxes |= 1u << axis;
        }
    }
    int openingNavigationTarget() const {
        if (openingCenterMode) return topology.cid(config.width / 2, config.height / 2);
        if (!openingCrossCenter) return openingTarget;
        int coords[2] = {topology.cx(openingTarget), topology.cy(openingTarget)};
        int dimensions[2] = {config.width, config.height};
        for (int axis = 0; axis < 2; ++axis)
            if (!(openingCrossedAxes & (1u << axis)) && 2 * coords[axis] == dimensions[axis] - 1)
                coords[axis] = std::clamp(coords[axis] + (turn.myId & 1 ? -1 : 1), 0, dimensions[axis] - 1);
        return topology.cid(coords[0], coords[1]);
    }
    bool inCenterBox(int radius) const {
        int cx = config.width / 2, cy = config.height / 2;
        return std::abs(topology.cx(turn.here) - cx) <= radius &&
               std::abs(topology.cy(turn.here) - cy) <= radius;
    }
    bool openingTravelComplete() const {
        if (openingCenterMode) return inCenterBox(2); // existing closed-boundary fallback
        return openingCrossCenter ? openingCrossedAxes == 3 : inTargetBox(1);
    }
    int openingRouteDir() const {
        if (openingCenterMode) return -1;
        if (openingCrossCenter && openingTravelDir >= 0 &&
            (openingCrossedAxes & (1u << (openingTravelDir == 1 || openingTravelDir == 3 ? 0 : 1)))) return -1;
        return openingTravelDir;
    }
    bool openingWrappedRoute() const { return !openingCenterMode && !openingCrossCenter; }

    void observeOpeningPortalArrival() {
        if (world.lastExploredPortalRound == turn.round &&
            world.lastExploredPortalOwner == turn.myId && world.lastExploredPortalKey >= 0) {
            int pid = topology.portalIdAt(world.lastExploredPortalKey);
            openingPortalTarget = -1;
            openingPortalProgressRound = -1;
            openingPortalDistance = INF;
            openingPortalCooldownUntil = -1;
            if (openingPortalReturning && turn.here == openingPortalSource) {
                openingPortalCheck = openingPortalReturning = false;
                openingPortalId = openingPortalSource = -1;
                bugNav.reset();
            } else if (!openingPortalCheck) {
                openingPortalCheck = true;
                openingPortalReturning = false;
                openingPortalId = pid;
                openingPortalSource = world.lastExploredPortalSource;
                openingPortalTarget = -1;
                openingPortalProgressRound = -1;
                openingPortalDistance = INF;
            }
        }
    }
    int openingDistance(int cell) const {
        int target = openingNavigationTarget();
        return bugNav.routeDistance(cell, target, openingWrappedRoute(), openingRouteDir());
    }
    Action openingPriorityAction(Controller const &ct, DecisionContext const &context) {
        if (turn.round >= 60) return {};
        if (openingActive && openingPortalCheck) {
            int state = portalPocketState(openingPortalId);
            if (state > 0) {
                // A room with enough known space is a new exploration area.
                openingPortalCheck = false;
                openingPortalId = openingPortalSource = -1;
                openingActive = false;
                bugNav.reset();
                return {};
            }
            if (state < 0) {
                if(turn.isQueen())currentBehavior=BehaviorState::EMERGENCY;
                openingPortalReturning = true;
                int gateCell = -1, gateDir = -1;
                findPortalReturnGate(openingPortalId, openingPortalSource, gateCell, gateDir);
                if (gateCell >= 0 && turn.here == gateCell) {
                    int dest = topology.step(turn.here, gateDir);
                    if (dest >= 0 && !occupancy.teammateHeadAt(dest) && !occupancy.blockedAt(dest, 1))
                        return Action::move(gateDir);
                    return {};
                }
                if (gateCell >= 0) {
                    int dir = search.bfsFirstDir([gateCell](int c) { return c == gateCell; }, 24, 320);
                    if (dir < 0) dir = bugNav.direction(gateCell, true, turn.myId);
                    return dir >= 0 ? Action::move(dir) : Action{};
                }
                openingPortalCheck = openingPortalReturning = false;
                return {};
            }
            // The exit is not fully observed yet; reveal its local edges.
            currentBehavior=BehaviorState::EXPLORE;
            int frontier = search.bfsFirstDir([this](int c) { return world.seenRound[c] < 0; },
                [](int,int){return 0L;},8,192,15,nullptr,nullptr,!turn.isQueen());
            if (frontier >= 0) return Action::move(frontier);
            return {};
        }

        unsigned openingSafe = openingSafeFirstSteps(ct);

        // For all first 60 rounds, a confirmed cluster outranks exploration,
        // including after the center line/target releases opening travel.
        // Keep its chosen pearl until it is collected or disproved.
        int clusterTarget = -1, clusterSize = 0, clusterDistance = INF;
        for (int candidate : turn.visPearls) {
            if (openingPearlCooldown.count(candidate) || !coordination.pearlAvailable(candidate, 1)) continue;
            int n = 0;
            for (int c : turn.visPearls)
                if (topology.cheb(candidate, c) <= 2) ++n;
            if (n < 3) continue;
            int goal = -1, distance = INF;
            search.bfsFirstDir([candidate](int c) { return c == candidate; },
                               [](int, int) { return 0L; }, 8, 320, openingSafe, &goal, &distance);
            if (goal >= 0 && coordination.pearlAvailable(candidate, distance) &&
                (n > clusterSize || (n == clusterSize && distance < clusterDistance))) {
                clusterTarget = candidate;
                clusterSize = n;
                clusterDistance = distance;
            }
        }
        if (clusterTarget >= 0) {
            if (!trackOpeningPearl(clusterTarget, clusterDistance)) return {};
            int harvestDir = search.bestPearlHarvestDir(5, openingSafe);
            if (harvestDir >= 0) {
                currentBehavior = BehaviorState::COLLECT;
                return Action::move(harvestDir);
            }
            int dir = search.bfsFirstDir([clusterTarget](int c) { return c == clusterTarget; },
                                         [](int, int) { return 0L; }, 8, 320, openingSafe);
            if (dir < 0) dir = bugNav.direction(clusterTarget, true, turn.myId, -1, openingSafe);
            if (dir >= 0) { currentBehavior = BehaviorState::COLLECT; return Action::move(dir); }
        }

        if (!openingActive) return {};

        // Opening ordinary dragons scout known unvisited portals, even when
        // their long-term role is not Scout; queens explore on foot.
        if (!turn.isQueen() && turn.round < 60 && context.situation.knownPortals) {
            if (turn.round >= openingPortalCooldownUntil) {
                prepareRouteCrowd();
                unsigned safe = safeScoutFirstSteps(ct);
                PortalRoute route = search.scoutPortalRoute(
                    safe, [this](int c, int step) { return scoutTileScore(c, step); }, 24, 320);
                if (route.dir >= 0) {
                    bool abandon = false;
                    if (openingPortalTarget != route.pid) {
                        openingPortalTarget = route.pid;
                        openingPortalDistance = route.distance;
                        openingPortalProgressRound = turn.round;
                    } else if (route.distance < openingPortalDistance) {
                        openingPortalDistance = route.distance;
                        openingPortalProgressRound = turn.round;
                    } else if (openingPortalProgressRound >= 0 &&
                               turn.round - openingPortalProgressRound > 6) {
                        openingPortalCooldownUntil = turn.round + 8;
                        openingPortalTarget = -1;
                        openingPortalProgressRound = -1;
                        openingPortalDistance = INF;
                        if (mission.type == MissionType::SCOUT_PORTAL) mission = {};
                        abandon = true;
                    }
                    if (!abandon) {
                        claimPortalId = route.pid;
                        beginScoutMission(route);
                        currentBehavior = BehaviorState::EXPLORE;
                        return Action::move(route.dir);
                    }
                }
            }
        }

        // Small pearl targets may interrupt travel only when they are close
        // and add at most two steps over the wrapped direct distance.
        int smallTarget = -1, smallDistance = INF;
        int directDistance = openingDistance(turn.here);
        for (int candidate : turn.visPearls) {
            if (openingPearlCooldown.count(candidate) || !coordination.pearlAvailable(candidate, 1)) continue;
            int goal = -1, distance = INF;
            search.bfsFirstDir([candidate](int c) { return c == candidate; },
                               [](int, int) { return 0L; }, 3, 160, openingSafe, &goal, &distance);
            int remaining = openingDistance(candidate);
            if (goal >= 0 && coordination.pearlAvailable(candidate, distance) &&
                distance + remaining <= directDistance + 2 && distance < smallDistance) {
                smallTarget = candidate;
                smallDistance = distance;
            }
        }
        if (smallTarget >= 0) {
            if (!trackOpeningPearl(smallTarget, smallDistance)) return {};
            int dir = search.bfsFirstDir([smallTarget](int c) { return c == smallTarget; },
                [](int, int) { return 0L; }, 3, 160, openingSafe);
            if (dir >= 0) { currentBehavior = BehaviorState::COLLECT; return Action::move(dir); }
        }
        (void)context;
        return {};
    }
    bool trackOpeningPearl(int target, int distance) {
        if (openingPearlTarget != target) {
            openingPearlTarget = target;
            openingPearlDistance = distance;
            openingPearlProgressRound = turn.round;
            return true;
        }
        if (distance < openingPearlDistance) {
            openingPearlDistance = distance;
            openingPearlProgressRound = turn.round;
            return true;
        }
        if (openingPearlProgressRound >= 0 && turn.round - openingPearlProgressRound > 6) {
            openingPearlCooldown[target] = turn.round + 8;
            openingPearlTarget = -1;
            openingPearlDistance = INF;
            openingPearlProgressRound = -1;
            return false;
        }
        return true;
    }
    // Walking-only proof keeps the gate identity distinct from its remote landing.
    // Occupancy is timed on entry routes; remote terrain proofs do not assume bodies will move.
    std::pair<int,int> walkingRoute(int start,int target,bool occupied) const {
        if(start==target)return {-1,0};
        struct Node {int c,first,depth,growth;};
        std::vector<Node> queue{{start,-1,0,0}};
        std::vector<uint8_t> seen(config.cells,0);seen[start]=1;
        for(size_t i=0;i<queue.size() && i<320;++i) {
            auto n=queue[i];if(n.depth>=24)continue;
            for(int d=0;d<4;++d) {
                if(topology.edgeTypeAt(n.c*4+d)!=E_OPEN)continue;
                int q=topology.step(n.c,d);if(q<0 || seen[q])continue;
                if(occupied && (occupancy.teammateHeadAt(q) || occupancy.blockedAt(q,n.depth+1,n.growth) ||
                   coordination.protectedPortalExit(q)))continue;
                int first=n.first<0?d:n.first;
                if(q==target)return {first,n.depth+1};
                seen[q]=1;queue.push_back({q,first,n.depth+1,n.growth+int(occupancy.visible(q)&&occupancy.hasPearl(q))});
            }
        }
        return {-1,INF};
    }
    bool resourceStepSafe(int d) {
        Action a=Action::move(d);int q=topology.step(turn.here,d);
        return q>=0 && physicallyPossible(a) && !search.portalBusy(turn.here,d) &&
            !occupancy.enemyHeadAt(q) && !blocksTeammateEscape(a) && portalExitDebt(a)==0 &&
            coordination.stepRisk(turn.here,d)==0 && sprintModel.distanceToEnemyReach(q)>0;
    }
    void clearResourceGate() {
        resourceGate=resourceGateDir=resourcePortal=resourceTarget=-1;
        resourceProgressRound=-1;resourceDistance=INF;resourceBug.reset();
        blockedCluster=blockedClusterRound=-1;
    }
    Action exploredPortalResourceAction(Controller const &ct) {
        (void)ct;
        if(resourceGate>=0) {
            int landing=topology.step(resourceGate,resourceGateDir);
            if(!world.exploredPortals.count(resourcePortal) || landing<0 ||
               (occupancy.visible(resourceTarget)&&!occupancy.hasPearl(resourceTarget)) ||
               turn.round-resourceProgressRound>8 || portalPocketState(resourcePortal,landing)!=1) {
                clearResourceGate();resourceRetryRound=turn.round+8;return {};
            } else {
                // Arrival releases the entrance objective; ordinary collection resumes.
                if(walkingRoute(turn.here,resourceTarget,false).second<INF) {clearResourceGate();return {};}
                auto route=walkingRoute(turn.here,resourceGate,true);
                int distance=route.second;
                if(distance<resourceDistance){resourceDistance=distance;resourceProgressRound=turn.round;}
                int d=turn.here==resourceGate?resourceGateDir:route.first;
                if(d>=0 && resourceStepSafe(d)){currentBehavior=BehaviorState::COLLECT;return Action::move(d);}
                return {}; // survival or a temporary body obstruction can interrupt transit
            }
        }
        if(turn.round<resourceRetryRound)return {};
        // Only a visible group of at least three pearls justifies this fallback.
        int target=-1,gate=-1,gateDir=-1,pid=-1,best=INF,proofBudget=48;
        for(int pearl:turn.visPearls) {
            if(!coordination.pearlAvailable(pearl) || walkingRoute(turn.here,pearl,false).second<INF)continue;
            int cluster=0;for(int other:turn.visPearls)if(topology.cheb(pearl,other)<=2)++cluster;
            if(cluster<3)continue;
            for(auto const &[id,ends]:world.portalEnds) {
                if(!world.exploredPortals.count(id))continue;
                for(int key:ends)for(int side=0;side<2;++side) {
                    int c=side?topology.geo(key/4,key%4):key/4;
                    int d=side?(key%4)^2:key%4;
                    int landing=topology.step(c,d);
                    if(landing<0 || topology.cheb(landing,pearl)>3 || proofBudget<=0)continue;
                    --proofBudget;
                    if(portalPocketState(id,landing)!=1)continue;
                    auto beyond=walkingRoute(landing,pearl,false),approach=walkingRoute(turn.here,c,true);
                    if(beyond.second>8 || approach.second==INF)continue;
                    int cost=approach.second+1+beyond.second;
                    if(cost<best){best=cost;target=pearl;gate=c;gateDir=d;pid=id;}
                }
            }
        }
        if(target<0){blockedCluster=blockedClusterRound=-1;return {};}
        if(blockedCluster!=target){blockedCluster=target;blockedClusterRound=turn.round;resourceBug.reset();}
        unsigned mask=0;for(int d=0;d<4;++d)if(resourceStepSafe(d))mask|=1u<<d;
        int bug=resourceBug.direction(target,true,turn.myId,-1,mask);
        // Give BugNav a bounded attempt before switching to the known entrance.
        if(bug>=0 && turn.round-blockedClusterRound<6) {
            currentBehavior=BehaviorState::COLLECT;return Action::move(bug);
        }
        resourceGate=gate;resourceGateDir=gateDir;resourcePortal=pid;resourceTarget=target;
        resourceProgressRound=turn.round;resourceDistance=INF;
        return exploredPortalResourceAction(ct);
    }
    int portalPocketState(int pid, int start = -1) const {
        if (pid < 0) return 0;
        std::vector<uint8_t> seen(config.cells, 0);
        std::vector<int> queue;
        queue.reserve(32);
        if (start < 0) start = turn.here;
        queue.push_back(start);
        seen[start] = 1;
        bool uncertain = false, returnEdge = false;
        for (size_t i = 0; i < queue.size(); ++i) {
            int c = queue[i];
            if (queue.size() > 16) return 1;
            for (int d = 0; d < 4; ++d) {
                int edge = c * 4 + d;
                uint8_t type = topology.edgeTypeAt(edge);
                if (type == E_KELP) continue;
                if (type == E_UNKNOWN) { uncertain = true; continue; }
                if (type == E_PORTAL) {
                    if (topology.portalIdAt(edge) == pid) { returnEdge = true; continue; }
                    int q = topology.step(c, d);
                    if (q < 0) { uncertain = true; continue; }
                    if (!seen[q]) { seen[q] = 1; queue.push_back(q); }
                    continue;
                }
                int q = topology.step(c, d);
                if (q >= 0 && !seen[q]) { seen[q] = 1; queue.push_back(q); }
            }
        }
        if (uncertain) return 0;
        return returnEdge ? -1 : 1;
    }
    void findPortalReturnGate(int pid, int source, int &cell, int &dir) const {
        cell = dir = -1;
        int fallbackCell = -1, fallbackDir = -1;
        for (int c = 0; c < config.cells; ++c)
            for (int d = 0; d < 4; ++d) {
                int edge = c * 4 + d;
                if (topology.edgeTypeAt(edge) != E_PORTAL || topology.portalIdAt(edge) != pid) continue;
                if (fallbackCell < 0) { fallbackCell = c; fallbackDir = d; }
                if (topology.step(c, d) == source) { cell = c; dir = d; return; }
            }
        if (cell < 0) { cell = fallbackCell; dir = fallbackDir; }
    }
    unsigned openingSafeFirstSteps(Controller const &ct) {
        unsigned safe = 0;
        for (int d = 0; d < 4; ++d) {
            if (!physicallyPossible(Action::move(d))) continue;
            int q = topology.step(turn.here, d);
            if (search.portalBusy(turn.here, d)) continue;
            if (q >= 0 && occupancy.enemyHeadAt(q)) continue;
            if (q >= 0 && !pocketWorthEntering(adjacentPocket(d), d, ct)) continue;
            safe |= 1u << d;
        }
        return safe;
    }
    Action openingNavigation(Controller const &ct) {
        if (!openingActive || openingPortalCheck) return {};
        if (openingTravelComplete()) {
            openingActive = false;
            bugNav.reset();
            return {};
        }
        // A known closed seam makes the outward route unsuitable. Aim for
        // the center; a neck blocking reversal is handled by the navigator.
        if (!openingCenterMode && !openingCrossCenter && openingTravelDir >= 0 && openingTravelDir != openingDirectDir) {
            int x = topology.cx(turn.here), y = topology.cy(turn.here);
            if (openingTravelDir == 1) x = config.width - 1;
            if (openingTravelDir == 3) x = 0;
            if (openingTravelDir == 2) y = config.height - 1;
            if (openingTravelDir == 0) y = 0;
            if (topology.edgeTypeAt(topology.cid(x, y) * 4 + openingTravelDir) == E_KELP) {
                openingCenterMode = true;
                openingTravelDir = openingDirectDir;
                bugNav.reset();
            }
        }
        currentBehavior=BehaviorState::EXPLORE;
        int target = openingNavigationTarget();
        unsigned safe = openingSafeFirstSteps(ct);
        int dir = bugNav.direction(target, openingWrappedRoute(), turn.myId, openingRouteDir(), safe, !turn.isQueen());
        if (!openingCenterMode && !openingCrossCenter && bugNav.boundaryStalled()) {
            openingCenterMode = true;
            bugNav.reset();
            target = topology.cid(config.width / 2, config.height / 2);
            if (inCenterBox(2)) { openingActive = false; return {}; }
            dir = bugNav.direction(target, false, turn.myId, -1, safe, !turn.isQueen());
        }
        currentBehavior = BehaviorState::EXPLORE;
        return dir >= 0 ? Action::move(dir) : Action{};
    }
    bool missionAllowsExpansion() const {
#if FEAT_V32_MISSION_EXPANSION_ARB
        return mission.type != MissionType::SCOUT_PORTAL;
#else
        return true;
#endif
    }
    PocketInfo adjacentPocket(int dir) {
        if (!queries.pocketKnown[dir]) {
            queries.pocketKnown[dir] = 1;
            int dest = topology.step(turn.here, dir);
            queries.pocket[dir] = dest >= 0 ? search.pocketBeyond(dest, turn.here) : PocketInfo{};
        }
        return queries.pocket[dir];
    }
    int adjacentOnwardSafe(int dir, bool ate) {
        int ai = ate ? 1 : 0;
        if (!queries.onwardKnown[dir][ai]) {
            queries.onwardKnown[dir][ai] = 1;
            int dest = topology.step(turn.here, dir);
            queries.onward[dir][ai] = dest >= 0 ? search.onwardSafe(dest, ate) : 0;
        }
        return queries.onward[dir][ai];
    }
    int adjacentSpace12(int dir, bool ate) {
        int ai = ate ? 1 : 0;
        if (!queries.space12Known[dir][ai]) {
            queries.space12Known[dir][ai] = 1;
            int dest = topology.step(turn.here, dir);
            queries.space12[dir][ai] = dest >= 0 ? search.spaceFrom(dest, ate, 12, false) : 0;
        }
        return queries.space12[dir][ai];
    }
    Action queenRescueAction() const {
        if (turn.isQueen() || turn.population <= 1 || turn.round >= 499) return {};
        int queen = -1;
        for (auto const &p : turn.parts)
            if (p.mine && p.head && isQueenId(p.id)) { queen = p.c; break; }
        if (queen < 0) return {};
        std::vector<int> releasedExits;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(queen,d);
            if (topology.edgeTypeAt(queen*4+d) == E_KELP) continue;
            if (q < 0 || !occupancy.visible(q)) return {}; // unresolved exit may already permit escape
            if (!occupancy.occupied(q)) return {}; // Queen already has a physical move
            if (topology.edgeTypeAt(queen*4+d) == E_OPEN && occupancy.isMine(q) &&
                occupancy.dragonId(q) == turn.myId) releasedExits.push_back(q);
        }
        if (releasedExits.empty()) return {};
        for (auto const &s : world.sightings)
            if (s.round <= turn.round && turn.round-s.round <= 3 && topology.manh(queen,s.c) <= 14)
                return {}; // cannot certify safety against an unobserved remote enemy
        auto const &intel = world.enemyQueen;
        if (intel.c >= 0 && intel.round <= turn.round && turn.round-intel.round <= 3 &&
            topology.manh(queen,intel.c) <= 14 && !occupancy.enemyHeadAt(intel.c)) return {};
        auto danger = sprintModel.enemyReachAfterSuicide();
        if (danger[queen]) return {}; // releasing our body must not expose the Queen's current head
        // A safe ordinary move that releases the blocking tail saves both dragons.
        int tail = body.cellAtTailOffset(0);
        if (std::find(releasedExits.begin(),releasedExits.end(),tail) != releasedExits.end() && !danger[tail])
            for (int d = 0; d < 4; ++d) {
                Action move = Action::move(d);
                if (actionUsesHeldPortal(move) || blocksTeammateEscape(move)) continue;
                auto trace = sprintModel.trace(move);
                if (!trace.legal || trace.collision || danger[trace.head] ||
                    std::find(trace.segments.begin(),trace.segments.end(),tail) != trace.segments.end()) continue;
                for (int nextDir = 0; nextDir < 4; ++nextDir) {
                    int next = topology.step(tail,nextDir);
                    if (next >= 0 && occupancy.visible(next) && !occupancy.occupied(next) && !danger[next] &&
                        std::find(trace.segments.begin(),trace.segments.end(),next) == trace.segments.end())
                        return move;
                }
            }
        for (int exit : releasedExits) {
            if (danger[exit]) continue;
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(exit,d);
                if (q < 0 || !occupancy.visible(q) || danger[q]) continue;
                if (!occupancy.occupied(q) || (occupancy.isMine(q) && occupancy.dragonId(q)==turn.myId))
                    return Action::suicide(); // release the exit and at least one onward cell
            }
        }
        return {};
    }
    int queenLengthAt(int head) const {
        for (size_t i = 0; i < turn.enemyHeads.size(); ++i)
            if (turn.enemyHeads[i] == head)
                return turn.enemyHeadExactLen[i] >= 0 ? turn.enemyHeadExactLen[i] : std::max(2, turn.enemyHeadLen[i]);
        return 0;
    }
    bool queenTarget(int head) const {
        return head >= 0 && occupancy.enemyHeadAt(head) &&
               isQueenId(occupancy.dragonId(head)) && turn.mayAttackQueen(queenLengthAt(head));
    }
    bool confirmedQueenKill(Action const &action) const {
        if (turn.isQueen() || turn.population <= 1 || turn.enemyHeads.empty()) return false;
        if (action.type != ActionType::MOVE && action.type != ActionType::SPRINT) return false;
        auto trace = sprintModel.trace(action, true);
        return trace.legal && trace.collision && queenTarget(trace.head);
    }
    Action queenAttack() const {
        if (turn.isQueen() || escortClaim || turn.population <= 1) return {};
        for (int d = 0; d < 4; ++d) {
            Action a = Action::move(d);
            if (queenTarget(topology.step(turn.here,d)) && !actionUsesHeldPortal(a) &&
                !blocksTeammateEscape(a) && confirmedQueenKill(a)) return a;
        }
#if FEAT_SPRINT_RAM
        return sprintModel.attack([&](int head, int, int) { return queenTarget(head); },
            [&](Action const &a) { return !blocksTeammateEscape(a); });
#else
        return {};
#endif
    }
    Action queenPursuit(Controller const &ct) {
        if (escortClaim) return {};
        auto const &q = world.enemyQueen;
        if (q.c < 0 || q.round > turn.round || turn.round - q.round > 3 ||
            !turn.mayAttackQueen(q.length)) return {};
        unsigned safe = safeScoutFirstSteps(ct);
        for (int d = 0; d < 4; ++d) {
            Action a = Action::move(d); int next = topology.step(turn.here,d);
            if (next < 0 || !physicallyPossible(a) || actionUsesHeldPortal(a) ||
                blocksTeammateEscape(a) || enemyAttackRisk(next, turn.length) > 0 ||
                (multipleVisibleEnemies() && actionTouchesEnemy(a))) safe &= ~(1u << d);
        }
        // Stage beside the reported head; only fresh direct vision authorizes a ram.
        int distance = INF;
        int dir = search.bfsFirstDir([&](int c) {
            for (int d = 0; d < 4; ++d) if (topology.step(c,d) == q.c) return true;
            return false;
        }, [](int,int) { return 0L; }, 24, 320, safe, nullptr, &distance);
        if (dir < 0) return {};
        mission = {MissionType::HUNT_TARGET, q.c, turn.myTeam == Team::A ? 1 : 0,
                   turn.round, turn.round, distance};
        currentBehavior = BehaviorState::COMBAT;
        return Action::move(dir);
    }
    RamCandidate adjacentRam(bool champion) const {
        // With another visible enemy, leave room rather than initiating a trade.
        if (turn.isQueen() || multipleVisibleEnemies()) return {-1, false};
        // ---- adjacent enemy heads we could ram this move (portal aware)
        int ramDir = -1, ramSz = -1;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(turn.here, d);
            if (q < 0 || !occupancy.enemyHeadAt(q) || isQueenId(occupancy.dragonId(q))) continue;
            int sz = turn.countOf(occupancy.dragonId(q));
            if (sz > ramSz) {
                ramSz = sz;
                ramDir = d;
            }
        }
        bool protectChampion = champion && (turn.length > 5 || ramSz <= turn.length);
        bool equalShortTrade = ramDir >= 0 && equalShortEnemy(topology.step(turn.here, ramDir));
        bool ramGood = ramDir >= 0 && turn.population > 1 &&
                       (equalShortTrade || (ramSz >= turn.length - 1 && ramSz >= 3 && !protectChampion));

        return {ramDir, ramGood};
    }
    Action routePhase(Controller const &ct, DecisionContext const &context, RamCandidate const &ram,
                      PortalRoute const &scoutRoute, [[maybe_unused]] unsigned scoutSafeFirst
#ifdef DEBUG_PORTAL
                      ,
                      PortalDebug &debug
#endif
    ) {
        int deathDir = context.situation.deathCluesPresent ? deathRouteDirection(ct) : -1;
#if FEAT_V3_PEARL_ECONOMY && FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
        if (deathDir < 0 && scoutRoute.dir >= 0 && context.situation.pearlsVisible) {
            PearlRoute pearls = search.scoutPearlRoute(scoutSafeFirst,
                                                       [this](int c, int step) { return scoutTileScore(c, step); });
            bool nearby = pearls.distance == 1 || pearls.distance <= scoutRoute.distance;
            bool cluster = pearls.reachablePearls >= 2 &&
                           pearls.distance <= scoutRoute.distance + V3_PEARL_CLUSTER_DETOUR;
            if (pearls.dir >= 0 && pearls.dir != scoutRoute.dir && (nearby || cluster))
                return Action::move(pearls.dir);
        }
#endif
        if (deathDir < 0)
            if (Action a = explorePortal(context, scoutRoute)) return a;
        int planDir = deathDir >= 0 ? deathDir : routeDirection(ct, context);
        currentBehavior = deathDir >= 0 || mission.type == MissionType::COLLECT_RESOURCE
            ? BehaviorState::COLLECT : BehaviorState::EXPLORE;
        return scoreAndFallback(ct, context, ram.dir, planDir, deathDir >= 0, false
#ifdef DEBUG_PORTAL
                                ,
                                debug
#endif
        );
    }
    struct FeedRecipient {
        int id, cell, length, seenRound, distance;
        int routeDir = -1, routeDistance = INF;
    };
    bool feedingAreaSafe(int receiver) const {
        for (auto const &enemy : turn.parts) {
            if (enemy.mine) continue;
            if (topology.manh(enemy.c, turn.here) <= V5_FEED_PICKUP_STEPS + 1 ||
                topology.manh(enemy.c, receiver) <= V5_FEED_PICKUP_STEPS + 1) return false;
            // Do not leave the rear of the donor beside an opponent either.
            for (auto const &part : turn.parts)
                if (part.mine && part.id == turn.myId && topology.manh(enemy.c, part.c) <= 1)
                    return false;
        }
        // Geometric distance misses known portals. Conservatively allow an
        // enemy to traverse occupied cells while checking its local reach.
        for (int head : turn.enemyHeads) {
            std::vector<int> frontier{head}, seen{head};
            for (int depth = 0; depth <= V5_FEED_PICKUP_STEPS + 1; ++depth) {
                std::vector<int> next;
                for (int c : frontier) {
                    if (c == turn.here || c == receiver) return false;
                    if (depth == V5_FEED_PICKUP_STEPS + 1) continue;
                    for (int d = 0; d < 4; ++d) {
                        uint8_t edge = topology.edgeTypeAt(c * 4 + d);
                        int q = topology.step(c, d);
                        if (edge == E_KELP || edge == E_UNKNOWN || q < 0 ||
                            std::find(seen.begin(), seen.end(), q) != seen.end()) continue;
                        seen.push_back(q);
                        next.push_back(q);
                    }
                }
                frontier = std::move(next);
            }
        }
        return true;
    }
    bool collectorHasBacklog(int receiver) const {
        int pearls = 0;
        for (int c : turn.visPearls) {
            if (occupancy.occupied(c) || topology.manh(receiver, c) > V5_FEED_PICKUP_STEPS) continue;
            // One confirmed teammate drop can already occupy the receiver's
            // next move. Prevent multiple donors from competing for that move.
            if (highValueResource(c) || ++pearls >= V5_FEED_CLUSTER_SIZE) return true;
        }
        return false;
    }
    bool canReceiveFeed(FeedRecipient const &r) const {
        if (r.seenRound != turn.round || !occupancy.teammateHeadAt(r.cell) ||
            occupancy.dragonId(r.cell) != r.id || !feedingAreaSafe(r.cell) ||
            collectorHasBacklog(r.cell)) return false;
        bool canCollect = false;
        std::array<int, V5_FEED_PICKUP_STEPS + 1> path{};
        path[0] = r.cell;
        auto walk = [&](auto &&self, int c, int depth) -> void {
            if (canCollect || depth >= V5_FEED_PICKUP_STEPS) return;
            for (int d = 0; d < 4; ++d) {
                uint8_t edge = topology.edgeTypeAt(c * 4 + d);
                int q = topology.step(c, d);
                if (edge == E_KELP || edge == E_UNKNOWN || q < 0 || !occupancy.visible(q) ||
                    std::find(path.begin(), path.begin() + depth + 1, q) != path.begin() + depth + 1)
                    continue;
                // The donor disappears before pickup. Other bodies stay blocked;
                // do not speculate about a collector tail freeing while it grows.
                bool donorPart = occupancy.occupied(q) && occupancy.isMine(q) &&
                                 occupancy.dragonId(q) == turn.myId;
                if (occupancy.occupied(q) && !donorPart) continue;
                path[depth + 1] = q;
                if (q == turn.here) {
                    int moves = depth + 1;
                    int delay = r.id < turn.myId ? 1 : 0; // receiver has already acted
                    if (Constants::MAX_ROUNDS - turn.round < moves + delay + V5_FEED_END_MARGIN)
                        continue;
                    for (int e = 0; e < 4; ++e) {
                        uint8_t nextEdge = topology.edgeTypeAt(q * 4 + e);
                        int next = topology.step(q, e);
                        if (nextEdge == E_KELP || nextEdge == E_UNKNOWN || next < 0 ||
                            !occupancy.visible(next) ||
                            std::find(path.begin(), path.begin() + moves + 1, next) != path.begin() + moves + 1)
                            continue;
                        if (!occupancy.occupied(next) ||
                            (occupancy.isMine(next) && occupancy.dragonId(next) == turn.myId))
                            canCollect = true;
                    }
                } else self(self, q, depth + 1);
            }
        };
        walk(walk, r.cell, 0);
        return canCollect;
    }
    bool highValueResource(int c) const {
        if (!occupancy.visible(c) || !occupancy.hasPearl(c) || occupancy.occupied(c)) return false;
        for (auto const &clue : world.deathClues)
            if (clue.friendly && clue.c == c && turn.round - clue.round <= 12) return true;
        int pearls = 0;
        for (int p : turn.visPearls)
            if (!occupancy.occupied(p) && topology.manh(c, p) <= 2 &&
                ++pearls >= V5_FEED_CLUSTER_SIZE) return true;
        return false;
    }
    Action sacrifice([[maybe_unused]] Controller const &ct, [[maybe_unused]] DecisionContext const &context) {
#if FEAT_SACRIFICE
        if (!DonationPolicy::feedingActive(turn)) return {};
        DonationPolicy policy = DonationPolicy::at(config, turn);
        unsigned safeFirst = safeScoutFirstSteps(ct);
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(turn.here, d);
            if (q < 0 || enemyAttackRisk(q, turn.length +
                    int(occupancy.visible(q) && occupancy.hasPearl(q))) != 0 ||
                blocksTeammateEscape(Action::move(d))) safeFirst &= ~(1u << d);
        }

        bool mayDonate = policy.donor(turn);
        if (!mayDonate) feedTarget.clear();
        // Resource collection remains independent of relative feeding eligibility.
        // Larger donors may collect first, then feed if there is no useful resource.
        if (turn.length >= V5_FEED_RESOURCE_COLLECTOR_LENGTH) {
            if (safeFirst && !turn.visPearls.empty()) {
                prepareRouteCrowd();
                int dir = search.bfsFirstDir(
                    [this](int c) { return highValueResource(c); },
                    [this](int c, int step) { return scoutTileScore(c, step); },
                    V5_FEED_RESOURCE_DEPTH, V5_FEED_ROUTE_NODES, safeFirst);
                if (dir >= 0) {
                    currentBehavior = BehaviorState::COLLECT;
                    return Action::move(dir);
                }
            }
        }
        if (!mayDonate) return {};

        std::vector<FeedRecipient> recipients;
        bool targetDisqualified = false;
        for (size_t i = 0; i < turn.friendHeads.size(); ++i) {
            int c = turn.friendHeads[i], id = occupancy.dragonId(c);
            int length = turn.friendHeadExactLen[i];
            if (length < 0) {
                length = world.allyLengthEstimate(id, turn.round, turn.friendHeadLen[i]);
            }
            if (!policy.recipient(turn, length)) {
                if (id == feedTarget.id) targetDisqualified = true;
                continue;
            }
            recipients.push_back({id, c, length, turn.round, topology.manh(turn.here, c)});
            if (id == feedTarget.id) {
                feedTarget.cell = c;
                feedTarget.length = length;
                feedTarget.lastSeenRound = turn.round;
            }
        }
        std::sort(recipients.begin(), recipients.end(), [](FeedRecipient const &a, FeedRecipient const &b) {
            if (a.length != b.length) return a.length > b.length;
            return a.distance != b.distance ? a.distance < b.distance : a.id < b.id;
        });

        // Current visible opportunities outrank pursuit, including in cooldown.
        for (auto const &r : recipients)
            if (canReceiveFeed(r)) {
                feedTarget.clear();
                currentBehavior = BehaviorState::FEED;
                return Action::suicide();
            }

        if (targetDisqualified) feedTarget.abandon(turn.round);
        // Sonar can refresh length evidence, but cannot create a remote target
        // or refresh this local last-seen timer.
        if (feedTarget.id >= 0 && feedTarget.lastSeenRound != turn.round &&
            turn.round - feedTarget.lastSeenRound < V5_FEED_TARGET_TIMEOUT)
            recipients.push_back({feedTarget.id, feedTarget.cell, feedTarget.length,
                                  feedTarget.lastSeenRound, topology.manh(turn.here, feedTarget.cell)});

        for (auto &r : recipients) {
            if (!safeFirst || !feedingAreaSafe(r.cell) ||
                (r.seenRound == turn.round && r.distance <= V5_FEED_PICKUP_STEPS &&
                 collectorHasBacklog(r.cell))) continue;
            std::array<int, 4> goals{};
            goals.fill(-1);
            bool reachedOldPosition = false;
            for (int d = 0; d < 4; ++d) {
                uint8_t edge = topology.edgeTypeAt(r.cell * 4 + d);
                int q = topology.step(r.cell, d);
                if (edge == E_KELP || edge == E_UNKNOWN || q < 0) continue;
                if (q == turn.here) { reachedOldPosition = true; continue; }
                if (occupancy.occupied(q)) continue;
                goals[d] = q;
            }
            if (r.seenRound != turn.round && reachedOldPosition) continue;
            r.routeDir = search.bfsFirstDir(
                [&goals](int c) { return std::find(goals.begin(), goals.end(), c) != goals.end(); },
                [](int, int) { return 0L; }, V5_FEED_ROUTE_DEPTH, V5_FEED_ROUTE_NODES,
                safeFirst, nullptr, &r.routeDistance);
            if (Constants::MAX_ROUNDS - turn.round < r.routeDistance +
                V5_FEED_PICKUP_STEPS + V5_FEED_END_MARGIN) r.routeDir = -1;
            if (r.id == feedTarget.id && r.routeDir >= 0 && r.routeDistance < feedTarget.lastDistance) {
                feedTarget.lastDistance = r.routeDistance;
                feedTarget.progressRound = turn.round;
            }
        }
        if (feedTarget.expired(turn.round)) feedTarget.abandon(turn.round);
        if (turn.round < feedTarget.cooldownUntil) return {};
        FeedRecipient const *best = nullptr, *current = nullptr;
        for (auto const &r : recipients) {
            if (r.routeDir < 0) continue;
            if (!best || r.length > best->length ||
                (r.length == best->length && r.routeDistance < best->routeDistance)) best = &r;
            if (r.id == feedTarget.id) current = &r;
        }
        if (current && (!best || best->length < current->length + V5_FEED_SWITCH_LENGTH_GAIN ||
                        best->routeDistance > current->routeDistance)) best = current;
        if (!best) return {};
        if (feedTarget.id != best->id) {
            // Switching does not replenish an existing chase's total budget.
            if (feedTarget.id < 0) feedTarget.startedRound = turn.round;
            feedTarget.progressRound = turn.round;
        }
        feedTarget.id = best->id;
        feedTarget.cell = best->cell;
        feedTarget.length = best->length;
        feedTarget.lastSeenRound = best->seenRound;
        feedTarget.lastDistance = best->routeDistance;
        currentBehavior = BehaviorState::FEED;
        return Action::move(best->routeDir);
#endif
        return {};
    }
    Action tacticalSplit(Controller const &ct) const {
#if FEAT_V3_TACTICAL_SPLIT
        // The child acts later this round from the old tail. Only target an
        // enemy that has already acted, so its visible head will stay put.
        if (turn.isQueen() || escortClaim || turn.round >= CH_START || turn.length < 8 || !ct.can_split(2) || !body.tailCanEscape()) return {};
        for (int head : turn.enemyHeads)
            if (topology.manh(turn.here, head) <= 2) return {};
        for (auto const &tail : turn.parts) {
            if (!tail.mine || tail.id != turn.myId || tail.head || occupancy.freeAt(tail.c) != 2) continue;
            for (auto const &enemy : turn.parts) {
                if (enemy.mine || !enemy.head || enemy.id >= turn.myId || turn.countOf(enemy.id) < 5) continue;
                bool adjacent = false;
                for (int d = 0; d < 4; ++d)
                    if (topology.edgeTypeAt(tail.c * 4 + d) == E_OPEN && topology.step(tail.c, d) == enemy.c)
                        adjacent = true;
                if (!adjacent) continue;
                int childVisible = 0;
                for (auto const &part : turn.parts)
                    if (!part.mine && part.id == enemy.id && topology.cheb(tail.c, part.c) <= 3)
                        ++childVisible;
                if (childVisible >= 3) return Action::split(2);
            }
        }
#else
        (void)ct;
#endif
        return {};
    }
    void refreshEscortClaim() {
        if (turn.isQueen()) return;
        if (!escortLocked) {
            escortLocked = true;
            bool seeQueen = false;
            int guards = 0;
            for (auto const &p : turn.parts) {
                if (!p.mine || p.id == turn.myId) continue;
                if (isQueenId(p.id)) seeQueen = true;
                if (p.head && !isQueenId(p.id) && topology.cheb(turn.here, p.c) <= 4) ++guards;
            }
            // The first two births stay escorts. A later length-2 child born
            // beside the queen claims the role only when the ring is empty.
            escortClaim = turn.population <= 3 ||
                          (turn.population >= 7 && seeQueen && guards < 2 && turn.length <= 2);
            return;
        }
        if (escortClaim) return;
        int queen = friendlyQueenCell();
        if (queen < 0 || topology.manh(turn.here, queen) > 6) return;
        int guards = 0;
        bool lower = false;
        for (auto const &p : turn.parts) {
            if (!p.mine || !p.head || p.id == turn.myId || isQueenId(p.id)) continue;
            if (topology.manh(p.c, queen) <= 4) {
                ++guards;
                if (p.id < turn.myId) lower = true;
            }
        }
        if (guards < 2 && !lower) escortClaim = true;
    }
    int friendlyQueenCell() const {
        if (turn.isQueen()) return turn.here;
        int id = turn.myTeam == Team::A ? 0 : 1;
        for (auto const &p : turn.parts)
            if (p.mine && p.head && p.id == id) return p.c;
        auto it = world.allyRallies.find(id);
        if (it != world.allyRallies.end() && it->second.round <= turn.round &&
            turn.round - it->second.round <= 8)
            return it->second.c;
        return -1;
    }
    // Escorts hold a ring around the queen and step toward a nearby threat.
    Action escortGuard() {
        if (!escortClaim) return {};
        int queen = friendlyQueenCell();
        if (queen < 0) return {};
        int threat = -1, threatDist = INF;
        for (int head : turn.enemyHeads) {
            int dist = topology.manh(queen, head);
            if (dist < threatDist) { threatDist = dist; threat = head; }
        }
        bool intercept = threat >= 0 && threatDist <= 6;
        int now = topology.manh(turn.here, queen);
        int best = -1, bestRank = INF;
        for (int d = 0; d < 4; ++d) {
            Action move = Action::move(d);
            if (!physicallyPossible(move) || blocksTeammateEscape(move)) continue;
            int q = topology.step(turn.here, d);
            if (q < 0 || q == queen || occupancy.enemyHeadAt(q)) continue;
            int qd = topology.manh(q, queen);
            int rank;
            if (intercept) rank = topology.manh(q, threat) * 10 + (qd < 1 ? 50 : (qd > 5 ? qd : 0));
            else if (qd >= 2 && qd <= 4) rank = qd;
            else if (qd < 2) rank = 30 + (2 - qd) * 5;
            else rank = 20 + qd;
            if (rank < bestRank) { bestRank = rank; best = d; }
        }
        if (best < 0) return {};
        int nextDist = topology.manh(topology.step(turn.here, best), queen);
        if (!intercept && now >= 2 && now <= 4 && (nextDist < 2 || nextDist > 4)) return {};
        return Action::move(best);
    }
    // Ordinary dragons step away while they are inside the queen's ring.
    Action yieldQueenSpace() {
        if (turn.isQueen() || escortClaim) return {};
        int queen = friendlyQueenCell();
        if (queen < 0) return {};
        int now = topology.manh(turn.here, queen);
        if (now > 4) return {};
        int best = -1, bestDist = now;
        for (int d = 0; d < 4; ++d) {
            Action move = Action::move(d);
            if (!physicallyPossible(move) || blocksTeammateEscape(move)) continue;
            int q = topology.step(turn.here, d);
            if (q < 0 || occupancy.enemyHeadAt(q)) continue;
            int dist = topology.manh(q, queen);
            if (dist > bestDist) { bestDist = dist; best = d; }
        }
        if (best < 0) return {};
        return Action::move(best);
    }
    Action earlyGrowthAction(Controller const &ct) {
        if (turn.isQueen()) {
            // Keep her length. Split off the shortest legal child only to seed
            // the swarm, then only to replace a missing nearby escort.
            int guards = 0;
            for (auto const &p : turn.parts)
                if (p.mine && p.head && !isQueenId(p.id) && topology.cheb(turn.here, p.c) <= 3)
                    ++guards;
            bool seeding = turn.population < 6;
            bool replacing = !seeding && guards < 2 && turn.round - lastQueenSplitRound >= 8;
            if ((!seeding && !replacing) || !turn.enemyHeads.empty() || turn.length < 4 ||
                !ct.can_split(2) || !body.tailCanEscape(true)) return {};
            Action split = Action::split(2);
            if (blocksTeammateEscape(split) || enemyAttackRisk(turn.here, turn.length - 2) > 0) return {};
            earlyGrowthSplit = true;
            lastQueenSplitRound = turn.round;
            currentBehavior = BehaviorState::EXPAND;
            return split;
        }
        if (escortClaim) return {};
        int endRound = 70;
        if (turn.round < 0 || turn.round >= endRound || turn.length < 4) return {};
        // Early development ignores the artificial population target; combat still has priority.
        if (!turn.enemyHeads.empty()) return {};
        if (turn.length <= 5) {
            if (turn.visPearls.empty()) return {};
            // "Immediately edible" is one legal, safe move, including a
            // learned portal exit; geometric adjacency alone is insufficient.
            int bestDir = -1, bestSpace = -1;
            for (int d = 0; d < 4; ++d) {
                Action move = Action::move(d);
                if (!physicallyPossible(move) || blocksTeammateEscape(move)) continue;
                int q = topology.step(turn.here, d);
                if (q < 0 || !occupancy.visible(q) || !occupancy.hasPearl(q) ||
                    occupancy.enemyHeadAt(q) || !coordination.pearlAvailable(q) ||
                    enemyAttackRisk(q, turn.length + 1) > 0) continue;
                if (!pocketWorthEntering(adjacentPocket(d), d, ct)) continue;
                int space = search.spaceFrom(q, true, 24, false);
                if (space > bestSpace) { bestSpace = space; bestDir = d; }
            }
            if (bestDir >= 0) {
                currentBehavior = BehaviorState::COLLECT;
                return Action::move(bestDir);
            }
        }
        int childSize = turn.length / 2;
        Action split = Action::split(childSize);
        if (!ct.can_split(childSize) || !body.tailCanEscape(true)) return {};
        if (turn.isQueen() && (blocksTeammateEscape(split) ||
            enemyAttackRisk(turn.here, turn.length - childSize) > 0 ||
            enemyAttackRisk(body.cellAtTailOffset(0), childSize) > 0)) return {};
        earlyGrowthSplit = true;
        currentBehavior = BehaviorState::EXPAND;
        return Action::split(childSize);
    }
    Action expansion([[maybe_unused]] Controller const &ct, DecisionContext const &context, bool activeScoutTask) {
        if (escortClaim) return {};
        if (turn.isQueen() && !queenDeadEndMode) return {}; // ordinary salvage only after proven confinement
        DonationPolicy donation = DonationPolicy::at(config, turn);
        bool consolidating = DonationPolicy::quiet(config, turn, world);
        if (mission.type == MissionType::PEARL_POCKET) {
            int childSize = turn.length - 2;
            if ((!consolidating || turn.population < donation.reserve) &&
                turn.population < config.targetPopulation && ct.can_split(childSize) &&
                atKnownDeadEnd() && body.tailCanEscape()) {
                return Action::split(childSize); // retain 2; reverse all rear segments into the child
            }
            return {};
        }
        // ---- 1) keep the swarm at the cap; in a quiet donation phase,
        // reserve a diminishing population so accumulated length stays intact.
        bool wantExpand = consolidating ? turn.population < donation.reserve
                          : (turn.round < 470 && !context.roles.champion) || turn.population < 8;
        if (!activeScoutTask && wantExpand && turn.population < config.targetPopulation && context.profile.length >= 4 &&
            !context.profile.coreProtected && context.profile.canSplit && body.tailCanEscape()) {
            return Action::split(2);
        }

        return {};
    }
    Action sprintRam([[maybe_unused]] DecisionContext const &context) const {
#if FEAT_SPRINT_RAM
        if (turn.isQueen() || escortClaim || multipleVisibleEnemies() || turn.length < 2 || turn.population <= 1 ||
            (context.roles.champion && turn.length > 5 &&
             !(DonationPolicy::feedingActive(turn) && turn.length <= V5_FEED_SHORT_LENGTH))) return {};

        return sprintModel.attack([&](int head, int collisionLength, int size) {
            if (isQueenId(occupancy.dragonId(head))) return false; // separate Queen policy
            bool worth = size >= collisionLength + 3 && size >= 5;
            if (context.roles.attrition && size >= collisionLength + 2 && size >= 4) worth = true;
            // Collision occurs before this step's payment; use the actual collision length.
            // Exact enemy length and a visible teammate are still required.
            if (collisionLength <= 4 && size == collisionLength) {
                bool ally = false;
                for (auto const &part : turn.parts) if (part.mine && part.id != turn.myId) ally = true;
                for (size_t i = 0; ally && i < turn.enemyHeads.size(); ++i)
                    if (turn.enemyHeads[i] == head && i < turn.enemyHeadExactLen.size() &&
                        turn.enemyHeadExactLen[i] == collisionLength) worth = true;
            }
            return worth;
        }, [&](Action const &action) { return !blocksTeammateEscape(action); });
#endif

        return {};
    }
    Action sprintEscape(Controller const &ct) {
#if FEAT_SPRINT_ESCAPE
        if (turn.enemyHeads.empty() || sprintModel.distanceToEnemyReach(turn.here) > 0) return {};
        auto escape = sprintModel.escape([&](Action const &action) {
            return queenAvoidsNewPocket(action) && !blocksTeammateEscape(action);
        }, [&](Action const &action) {return portalExitDebt(action)==0;});
        if (escape.action && (escape.risk == 0 || (escape.risk == 1 && escape.clearance > 0))) {
            chosenSprintEscape=escape.action;return escape.action;
        }
        // If <=2 segments cannot get the head beyond the enemy's next-action
        // region, prefer a known escaping tail instead of spending >=3.
        int tail = body.cellAtTailOffset(0), child = turn.isQueen() && !queenDeadEndMode ? 2 : turn.length - 2;
        Action split = Action::split(child);
        bool tailSafe = false;
        if (tail >= 0 && sprintModel.distanceToEnemyReach(tail) > 0)
            for (int d = 0; d < 4; ++d) {
                if (topology.edgeTypeAt(tail * 4 + d) != E_OPEN) continue;
                int q = topology.step(tail, d);
                if (q >= 0 && !occupancy.occupied(q) && sprintModel.distanceToEnemyReach(q) > 0) tailSafe = true;
            }
        if (ct.can_split(child) && body.tailCanEscape(true) && tailSafe && !blocksTeammateEscape(split) &&
            (!multipleVisibleEnemies() || !enemyTouchesCell(tail))) return split;
        chosenSprintEscape=escape.action;
        return escape.action; // uncertainty beats a proven head attack, even at equal clearance
#else
        (void)ct;
        return {};
#endif
    }
    int shortEnemyRiskDistance(int c, int ourLength = -1) const {
        if (!FEAT_V3_SHORT_AVOID && !turn.isQueen()) return INF;
        // Ordinary dragons use a relative threshold; queens avoid every enemy.
        int maxThreatLength = (ourLength < 0 ? turn.length : ourLength) - 3;
        if (!turn.isQueen() && maxThreatLength < 1) return INF;
        int nearest = INF;
        for (size_t i = 0; i < turn.enemyHeads.size(); ++i) {
            if (i >= turn.enemyHeadExactLen.size() || i >= turn.enemyHeadLen.size()) continue;
            int exact = turn.enemyHeadExactLen[i];
            // A partial body is only a lower bound. If it has already
            // exceeded the threshold, this enemy cannot be short enough.
            if (!turn.isQueen() && (exact > maxThreatLength ||
                (exact < 0 && turn.enemyHeadLen[i] > maxThreatLength))) continue;
            int head = turn.enemyHeads[i];
            int distance = topology.manh(c, head);
            for (int d = 0; d < 4; ++d)
                if (topology.step(c, d) == head) distance = std::min(distance, 1);
            nearest = std::min(nearest, distance);
        }
        return nearest;
    }
    // 0 = no observed relevant attack route, 1 = uncertain (partial enemy
    // body or route visibility), 2 = confirmed attack reach. Queens include all lengths. Mirrors normal
    // rams, pearl-funded sprints, and the early tail-spawned split attack.
    int enemyAttackRisk(int target, int ourLength) const {
        if (!FEAT_V3_SHORT_AVOID && !turn.isQueen()) return 0;
        int bestRisk = 0;
        int const shortLimit = ourLength - 3;
        if (!turn.isQueen() && shortLimit < 1) return 0;

        // Model our candidate move first so enemy pathfinding sees the body
        // that will actually occupy the board after we land.
        std::vector<int> ourBody;
        ourBody.reserve(turn.length + 1);
        for (int n = 0; n < turn.length; ++n)
            ourBody.push_back(n == 0 ? turn.here : body.cellAtTailOffset(turn.length - 1 - n));
        if (ourBody.empty()) ourBody.push_back(turn.here);
        bool ourEatsPearl = occupancy.visible(target) && occupancy.hasPearl(target);
        if (!ourEatsPearl && !ourBody.empty()) ourBody.pop_back();
        ourBody.insert(ourBody.begin(), target);

        for (size_t i = 0; i < turn.enemyHeads.size(); ++i) {
            if (i >= turn.enemyHeadLen.size()) continue;
            int exact = i < turn.enemyHeadExactLen.size() ? turn.enemyHeadExactLen[i] : -1;
            int observedLength = turn.enemyHeadLen[i];
            int enemyId = occupancy.dragonId(turn.enemyHeads[i]);
            int enemyHead = turn.enemyHeads[i];

#if FEAT_V3_TACTICAL_SPLIT
            // A lower-ID long enemy that has already acted may create a child
            // beside this landing cell. The opponent's population capacity is
            // hidden, so treat a geometrically valid split attack as uncertain.
            if (turn.round < CH_START && enemyId >= 0 && enemyId < turn.myId &&
                observedLength >= 8 && turn.countOf(turn.myId) >= 5 &&
                topology.manh(turn.here, enemyHead) > 2) {
                bool nearbyFriendlyHead = false;
                for (int friendHead : turn.friendHeads)
                    if (topology.manh(enemyHead, friendHead) <= 2) nearbyFriendlyHead = true;
                if (!nearbyFriendlyHead) {
                    for (auto const &tail : turn.parts) {
                        if (tail.mine || tail.id != enemyId || tail.head || occupancy.freeAt(tail.c) != 2) continue;
                        bool canRamLanding = false;
                        for (int d = 0; d < 4; ++d)
                            if (topology.edgeTypeAt(tail.c * 4 + d) == E_OPEN &&
                                topology.step(tail.c, d) == target) canRamLanding = true;
                        int childVisible = 0;
                        for (auto const &part : turn.parts)
                            if (part.mine && part.id == turn.myId && topology.cheb(tail.c, part.c) <= 3)
                                ++childVisible;
                        if (canRamLanding && childVisible >= 3) bestRisk = std::max(bestRisk, 1);
                    }
                }
            }
#endif

            if (!turn.isQueen() && exact >= 0 && exact > shortLimit) continue;
            if (!turn.isQueen() && exact < 0 && observedLength > shortLimit) continue;

            std::vector<int> initialBody{enemyHead};
            int current = enemyHead;
            for (int n = 1; n < std::max(observedLength, exact); ++n) {
                int next = -1;
                for (auto const &part : turn.parts) {
                    if (part.mine || part.id != enemyId || part.head) continue;
                    if (topology.step(part.c, part.facing) == current) {
                        next = part.c;
                        break;
                    }
                }
                if (next < 0 || std::find(initialBody.begin(), initialBody.end(), next) != initialBody.end()) break;
                initialBody.push_back(next);
                current = next;
            }
            bool bodyComplete = exact >= 0 && int(initialBody.size()) == exact;
            int initialLength = exact >= 0 ? exact : observedLength;
            std::vector<int> pathCells;
            int const maxSteps = turn.isQueen() ? 8 : 4;
            pathCells.reserve(maxSteps);
            int states = 0;
            bool hiddenRoute = false;
            int enemyRisk = 0;

            auto visit = [&](auto &&self, int head, int steps, int length,
                             std::vector<int> const &segments) -> void {
                bool paid = MovementRules::paidStep(initialLength, steps);
                if (steps >= maxSteps || (paid && length <= 2)) return;
                if (++states > 256) { hiddenRoute = true; return; }
                for (int d = 0; d < 4; ++d) {
                    int key = head * 4 + d;
                    uint8_t edge = topology.edgeTypeAt(key);
                    if (edge == E_KELP) continue;
                    int q = topology.step(head, d);
                    if (q < 0) {
                        if (edge == E_PORTAL) hiddenRoute = true;
                        continue;
                    }
                    if (std::find(segments.begin(), segments.end(), q) != segments.end()) continue;
                    if (q == target) {
                        if (turn.isQueen() || length <= shortLimit)
                            enemyRisk = std::max(enemyRisk, bodyComplete ? 2 : 1);
                        continue; // collision is resolved before pearl eating
                    }
                    if (!occupancy.visible(q)) {
                        hiddenRoute = true;
                        continue;
                    }
                    if (std::find(segments.begin(), segments.end(), q) != segments.end()) continue;
                    if (std::find(ourBody.begin(), ourBody.end(), q) != ourBody.end()) continue;
                    if (occupancy.occupied(q) && occupancy.dragonId(q) != enemyId &&
                        occupancy.blockedAt(q, 2 + steps)) continue;
                    if (occupancy.occupied(q) && occupancy.dragonId(q) == enemyId &&
                        std::find(initialBody.begin(), initialBody.end(), q) == initialBody.end()) continue;

                    bool eats = occupancy.hasPearl(q) &&
                        std::find(pathCells.begin(), pathCells.end(), q) == pathCells.end();
                    std::vector<int> after = segments;
                    if (!eats && !after.empty()) after.pop_back();
                    after.insert(after.begin(), q);
                    if (paid && !after.empty()) after.pop_back();
                    int afterLength = length + (eats ? 1 : 0) - int(paid);
                    pathCells.push_back(q);
                    self(self, q, steps + 1, afterLength, after);
                    pathCells.pop_back();
                }
            };
            visit(visit, enemyHead, 0, initialLength, initialBody);
            bestRisk = std::max(bestRisk, enemyRisk);

            // Missing body/visibility leaves a plausible route uncertain.
            if (bestRisk < 2 && hiddenRoute) {
                int pearlFuel = 0;
                for (int pearl : turn.visPearls)
                    if (topology.manh(enemyHead, pearl) <= 4 && topology.manh(pearl, target) <= 4)
                        ++pearlFuel;
                int possibleSteps = exact >= 0
                    ? std::min(maxSteps, MovementRules::freeSteps(exact) + exact - 2 + pearlFuel)
                    : maxSteps;
                if (topology.manh(enemyHead, target) <= possibleSteps)
                    bestRisk = std::max(bestRisk, 1);
            }
        }
        return bestRisk;
    }
    int deathRouteDirection(Controller const &ct) {
#if FEAT_V3_DEATH_PEARLS
        if (world.deathClues.empty()) return -1;
        unsigned safeFirst = safeScoutFirstSteps(ct);
        for (int d = 0; d < 4; ++d) {
            if (!(safeFirst & (1u << d))) continue;
            int q = topology.step(turn.here, d);
            if (q < 0 || shortEnemyRiskDistance(q) <= 3) {
                safeFirst &= ~(1u << d);
                continue;
            }
            for (int eh : turn.enemyHeads)
                if (topology.manh(q, eh) <= 2) safeFirst &= ~(1u << d);
        }
        if (!safeFirst) return -1;
        prepareRouteCrowd();
        return search.bfsFirstDir(
            [this](int c) {
                for (auto const &clue : world.deathClues)
                    if (clue.c == c && coordination.pearlAvailable(c) && turn.round - clue.round <= 2 &&
                        (!occupancy.visible(c) || occupancy.hasPearl(c))) return true;
                return false;
            },
            [this](int c, int step) { return scoutTileScore(c, step); }, 6, 100, safeFirst);
#else
        (void)ct;
        return -1;
#endif
    }
    Action sprintPearls([[maybe_unused]] Controller const &ct, [[maybe_unused]] DecisionContext const &context) {
#if FEAT_SPRINT_EAT
        // ---- 2c) grab two pearls in one turn when an enemy head is contesting them
        if (turn.length >= 3 && context.situation.enemyRelevant) {
            for (int d1 = 0; d1 < 4; ++d1) {
                int m = topology.step(turn.here, d1);
                if (m < 0 || !occupancy.visible(m) || !occupancy.hasPearl(m) || occupancy.blockedAt(m, 1) ||
                    occupancy.teammateHeadAt(m)) continue;
                if (!pocketWorthEntering(adjacentPocket(d1), d1, ct)) continue;
                for (int d2 = 0; d2 < 4; ++d2) {
                    int q = topology.step(m, d2);
                    if (q < 0 || q == turn.here || !occupancy.visible(q) || !occupancy.hasPearl(q) ||
                        occupancy.occupied(q) || occupancy.teammateHeadAt(q))
                        continue;
                    if (shortEnemyRiskDistance(m) <= 2 || shortEnemyRiskDistance(q) <= 2) continue;
                    bool contested = false;
                    for (int eh : turn.enemyHeads)
                        if (topology.manh(eh, q) <= 2 || topology.manh(eh, m) <= 2) contested = true;
                    if (!contested) continue;
                    bool nearHead = false;
                    for (int eh : turn.enemyHeads)
                        if (topology.cheb(eh, q) <= 1) nearHead = true;
                    if (nearHead && !context.roles.attrition) continue;
                    if (search.spaceFrom(q, true, 12, false) < std::min(turn.length + 2, 8)) continue;
                    return Action::sprint2(d1, d2);
                }
            }
        }
#endif

        return {};
    }
    void prepareRouteCrowd() {
        if (routeCrowdReady) return;
        routeCrowd.assign(config.cells, 0);
        if (config.width >= 7 && config.height >= 7) {
            for (int f : turn.friendParts) {
                int fx = topology.cx(f), fy = topology.cy(f);
                for (int dy = -3; dy <= 3; ++dy)
                    for (int dx = -3; dx <= 3; ++dx) {
                        int x = (fx + dx + config.width) % config.width;
                        int y = (fy + dy + config.height) % config.height;
                        ++routeCrowd[topology.cid(x, y)];
                    }
            }
        } else {
            for (int c = 0; c < config.cells; ++c)
                for (int f : turn.friendParts)
                    if (topology.cheb(c, f) <= 3) ++routeCrowd[c];
        }
        routeCrowdReady = true;
    }
    // Route comparison reuses the additive parts of ordinary move scoring.
    // Controlled-space and future pearl-distance scores depend on the current
    // search state, so they remain in the one-step scorer below.
    long scoutTileScore(int c, int step) const {
        bool eats = occupancy.visible(c) && occupancy.hasPearl(c);
        long score = world.seenRound[c] < 0 ? 130 : 0;
        score -= long(routeCrowd[c]) * 35;
        score -= long(coordination.headRisk(c, step)) * 100;
        int opens = 0;
        for (int d = 0; d < 4; ++d) {
            if (topology.edgeTypeAt(c * 4 + d) == E_KELP) continue;
            int q = topology.step(c, d);
            if (q < 0 || !occupancy.blockedAt(q, step + 1, eats)) ++opens;
        }
        if (opens <= 1 && !eats) score -= 80;
        return score;
    }
    bool hasOtherSafeMove(int exceptDir) const {
        for (int d = 0; d < 4; ++d) {
            if (d == exceptDir || topology.edgeTypeAt(turn.here * 4 + d) == E_KELP) continue;
            int q = topology.step(turn.here, d);
            if (q < 0) return true;
            if (!occupancy.teammateHeadAt(q) && !occupancy.blockedAt(q, 1)) return true;
        }
        return false;
    }
    bool pocketWorthEntering(PocketInfo const &pocket, int dir, Controller const &ct) const {
        if (!pocket.proven) return true;
        if (turn.isQueen() && !queenDeadEndMode) return false; // forbid new entry, allow ordinary in-pocket salvage
        bool canSalvage = turn.population < std::min(config.targetPopulation, ct.unit_limit) && turn.length >= 2 &&
                          turn.length + pocket.pearls >= 4;
        if (!canSalvage) return false;
        return pocket.pearls >= 1 && (pocket.pearls >= 3 || !hasOtherSafeMove(dir));
    }
    unsigned safeScoutFirstSteps(Controller const &ct) {
        if(turn.isQueen())return 0;
        unsigned safeFirst = 0;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(turn.here, d);
            if (search.portalBusy(turn.here, d)) continue;
            if (q < 0) {
                if (topology.unexploredPortal(turn.here, d)) safeFirst |= 1u << d;
                continue;
            }
            if (occupancy.teammateHeadAt(q) || occupancy.blockedAt(q, 1)) continue;
            PocketInfo pocket = adjacentPocket(d);
            if (!pocketWorthEntering(pocket, d, ct)) continue;
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            if (pocket.proven && pocket.pearls >= 3) {
                safeFirst |= 1u << d;
                continue;
            }
            if (adjacentOnwardSafe(d, eats) <= 1 ||
                adjacentSpace12(d, eats) < std::min(turn.length + 1, 8)) continue;
            safeFirst |= 1u << d;
        }
        return safeFirst;
    }
    Action explorePortal([[maybe_unused]] DecisionContext const &context, [[maybe_unused]] PortalRoute const &scoutRoute) {
        if(turn.isQueen())return {};
#if FEAT_PORTAL_EXPLORE
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE
        if (scoutRoute.dir >= 0) {
            claimPortalId = scoutRoute.pid;
            return Action::move(scoutRoute.dir);
        }
#else
        // Scout assignment limits intentional exploration to a stable, eligible subset.
#if FEAT_SCOUT_ROLE
        if (!context.roles.scout || !context.situation.knownPortals) return {};
#else
        // Ablation switch: retain the prior global exploration behavior.
        if (context.roles.champion || context.situation.enemyRelevant || !context.situation.knownPortals) return {};
#endif
        unsigned safeFirst = 0;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(turn.here, d);
            if (search.portalBusy(turn.here, d)) continue;
            if (q < 0) {
                if (topology.unexploredPortal(turn.here, d)) safeFirst |= 1u << d;
                continue;
            }
            if (occupancy.teammateHeadAt(q) || occupancy.blockedAt(q, 1)) continue;
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            if (adjacentOnwardSafe(d, eats) <= 1 ||
                adjacentSpace12(d, eats) < std::min(turn.length + 1, 8)) continue;
            safeFirst |= 1u << d;
        }
        int d = search.unexploredPortalDir(safeFirst);
        if (d >= 0) return Action::move(d);
#endif
#endif
        return {};
    }
    int routeDirection(Controller const &ct, DecisionContext const &context) {
        (void)ct;
        prepareRouteCrowd();
        constexpr int depth = 24, nodes = 320;
        auto tileScore = [this](int c, int step) { return scoutTileScore(c, step); };
        isGoal.assign(config.cells, 0);
        bool anyResource = context.situation.immediatePearlOpportunity;
        for (int c : turn.visPearls) {
            if (!coordination.pearlAvailable(c)) continue;
            isGoal[c] = 1;
            anyResource = true;
        }
        for (int c = 0; c < config.cells; ++c) {
            int due = world.pearlDue[c];
            if (due >= 0 && due < INF && due - turn.round <= 8 &&
                world.pearlInfo[c] >= turn.round - 6 &&
                (due >= turn.round || world.pearlInfo[c] >= turn.round - 3) &&
                (!occupancy.visible(c) || occupancy.hasPearl(c) || due > turn.round) &&
                coordination.pearlAvailable(c)) {
                isGoal[c] = 1;
                anyResource = true;
            }
        }

        int goal = -1, resourceDistance = INF;
        int resourceDir = -1;
        if (mission.type == MissionType::COLLECT_RESOURCE && mission.targetCell >= 0) {
            resourceDir = search.bfsFirstDir([&](int q) { return q == mission.targetCell; }, tileScore,
                                             depth, nodes, 15, &goal, &resourceDistance);
            if (resourceDistance < mission.lastDistance) mission.lastProgressRound = turn.round;
            if (resourceDistance < INF) mission.lastDistance = resourceDistance;
            return resourceDir;
        }

        if (anyResource)
            resourceDir = search.bfsFirstDir([&](int q) { return isGoal[q] != 0; }, tileScore,
                                             depth, nodes, 15, &goal, &resourceDistance);

        // Recompute exploration every turn. A pearl interrupts it only when
        // its wrapped displacement lies in the forward half-plane of the
        // BFS-selected frontier, preserving the original pearl-direction rule.
        int frontier = -1, frontierDistance = INF;
        int frontierDir = search.bfsFirstDir([&](int q) { return world.seenRound[q] < 0; }, tileScore,
                                             depth, nodes, 15, &frontier, &frontierDistance, !turn.isQueen());
        bool pearlKeepsExplorationHeading = false;
        if (resourceDir >= 0 && goal >= 0 && frontier >= 0 &&
            occupancy.visible(goal) && occupancy.hasPearl(goal)) {
            auto signedWrap = [](int delta, int span) {
                delta %= span;
                if (delta > span / 2) delta -= span;
                if (delta < -span / 2) delta += span;
                return delta;
            };
            int exploreDx = signedWrap(topology.cx(frontier) - topology.cx(turn.here), config.width);
            int exploreDy = signedWrap(topology.cy(frontier) - topology.cy(turn.here), config.height);
            int pearlDx = signedWrap(topology.cx(goal) - topology.cx(turn.here), config.width);
            int pearlDy = signedWrap(topology.cy(goal) - topology.cy(turn.here), config.height);
            pearlKeepsExplorationHeading = exploreDx * pearlDx + exploreDy * pearlDy >= 0;
        }
        if (pearlKeepsExplorationHeading) {
            mission = {MissionType::COLLECT_RESOURCE, goal, -1, turn.round, turn.round, resourceDistance};
            return resourceDir;
        }
        return frontierDir;
    }
    Action scoreAndFallback(Controller const &ct, DecisionContext const &context, int ramDir, int planDir,
                            bool deathRoute, bool emergency
#ifdef DEBUG_PORTAL
                            ,
                            PortalDebug &debug
#endif
    ) {
        bool pearlInView = context.situation.pearlsVisible;

        // ---- 4) score safe moves
        int const flr = std::min(turn.length + 1, 8);
        int best = -1;
        int bestThreat = INT_MAX;
        long bestScore = LONG_MIN;
#ifdef DEBUG_PORTAL
        PortalDebug::ChoiceGuard dbgOut{debug, best};
#endif
        for (int d = 0; d < 4; ++d) {
            uint8_t e = topology.edgeTypeAt(turn.here * 4 + d);
            if (e == E_KELP || !queenExplorationAllows(Action::move(d))) continue;
            int dest = topology.step(turn.here, d);
            bool isPortal = e == E_PORTAL;
            if (dest < 0) {
                if (!isPortal || (turn.isQueen() && !queenDeadEndMode)) continue;
                // unresolved portal: a mild exploration option
                long ps = 30 - long(coordination.stepRisk(turn.here, d)) * 250;
                auto it = world.portalEnds.find(topology.portalIdAt(turn.here * 4 + d));
                if (it == world.portalEnds.end() || it->second.size() < 2) ps += 50;
#if FEAT_V3_EXPLORED_BONUS
                if (world.exploredPortals.count(topology.portalIdAt(turn.here * 4 + d)))
                    ps += V3_EXPLORED_BONUS;
#endif
#ifdef DEBUG_PORTAL
                debug.scores[d] = ps;
#endif
                constexpr int unknownPortalThreat = 1;
                if (best < 0 || unknownPortalThreat < bestThreat ||
                    (unknownPortalThreat == bestThreat && ps > bestScore)) {
                    bestThreat = unknownPortalThreat;
                    bestScore = ps;
                    best = d;
                }
                continue;
            }
            if (occupancy.teammateHeadAt(dest) || occupancy.blockedAt(dest, 1)) continue;
            PocketInfo pocket = adjacentPocket(d);
            if (!pocketWorthEntering(pocket, d, ct)) continue;
            bool eats = occupancy.visible(dest) && occupancy.hasPearl(dest);
            int threat = enemyAttackRisk(dest, turn.length + (eats ? 1 : 0));
            // This applies to every role and length, including fragile short dragons.
            if (multipleVisibleEnemies() && enemyTouchesCell(dest)) threat += 3;
            int space = search.spaceFrom(dest, eats, 90, false);
            int controlled = context.situation.enemyRelevant
                                 ? search.controlledSpace(dest, eats, 80)
                                 : std::min(space, 80);
            int opens = adjacentOnwardSafe(d, eats);
            int crowd = 0;
            for (int f : turn.friendParts)
                if (topology.cheb(dest, f) <= 3) ++crowd;

            long score = (long)controlled * 11;
            if (pocket.proven && pocket.pearls >= 3)
                score += long(std::min(pocket.pearls, 4)) * 160;

            if (context.roles.champion || context.roles.fragile)
                for (int eh : turn.enemyHeads) {
                    int md = topology.manh(dest, eh);
                    if (md <= 1)
                        score -= emergency ? 2500 : 1000;
                    else if (md == 2)
                        score -= 150;
                }
            if (emergency)
                for (int eh : turn.enemyHeads) {
                    int md = topology.cheb(dest, eh);
                    if (md <= 1 && !(d == ramDir && ramDir >= 0)) score -= 2200;
                    else if (md == 2) score -= 300;
                }
            int shortEnemyDistance = shortEnemyRiskDistance(dest);
            if (shortEnemyDistance <= 1) score -= emergency ? 3000 : 1200;
            else if (shortEnemyDistance == 2) score -= emergency ? 1800 : 350;
            else if (shortEnemyDistance == 3) score -= 80;
            for (size_t i = 0; i < turn.enemyHeads.size() && !context.roles.champion && !context.roles.fragile; ++i) {
                int eh = turn.enemyHeads[i];
                if (topology.cheb(dest, eh) > 1) continue;
                int esz = turn.enemyHeadLen[i];
                if (esz > turn.length)
                    score += std::min((esz - turn.length) * 30L, 260L);
                else if (turn.length > esz)
                    score -= std::min((turn.length - esz) * (context.roles.attrition ? 5L : 12L), context.roles.attrition ? 40L : 120L);
                else
                    score += context.roles.attrition ? 25 : -15;
#if FEAT_FEAST
                {
                    int fN = 0, eN = 0;
                    for (int f : turn.friendParts)
                        if (topology.cheb(f, eh) <= 2) ++fN;
                    for (int e2 : turn.enemyHeads)
                        if (topology.cheb(e2, eh) <= 2) ++eN;
                    if (fN > eN) score += std::min((long)esz * 8L + 25L, 150L);
                }
#endif
            }
#if FEAT_TRAP
            for (int eh : turn.enemyHeads)
                if (topology.cheb(dest, eh) == 1 && topology.manh(dest, eh) == 1) {
                    int esc = search.enemyEscapes(eh);
                    if (esc <= 2) score += (long)(3 - esc) * 40L;
                }
#endif
            if (context.roles.hunter) {
                int nearD = 99;
                for (size_t i = 0; i < turn.enemyHeads.size(); ++i)
                    if (turn.enemyHeadLen[i] > turn.length + 2)
                        nearD = std::min(nearD, topology.manh(dest, turn.enemyHeads[i]));
#if FEAT_HUNT_SIGHTINGS
                if (nearD == 99)
                    for (auto const &s : world.sightings)
                        if (s.len > turn.length + 2 && turn.round - s.round <= 3 && topology.manh(turn.here, s.c) <= 14)
                            nearD = std::min(nearD, topology.manh(dest, s.c) + 4);
#endif
                if (nearD < 99) score -= (long)nearD * 14;
            }
            if (context.roles.champion)
                for (auto const &s : world.sightings)
                    if (turn.round - s.round <= 2 && topology.manh(dest, s.c) <= 3)
                        score -= (4 - topology.manh(dest, s.c)) * 60L;

            if (pearlInView) {
                score -= (long)std::min(search.visPearlDist(dest), 14) * 25;
                if (eats) score += coordination.pearlAvailable(dest) ? 500 : -500;
            }
            if (d == planDir) {
                if (deathRoute) score += 250;
                else if (currentBehavior == BehaviorState::COMBAT) score += 30;
                else if (mission.type == MissionType::COLLECT_RESOURCE) score += 500;
                else if (!emergency && currentBehavior == BehaviorState::EXPLORE) score += 300;
            }
#if FEAT_V3_EXPLORED_BONUS
            if (isPortal && world.exploredPortals.count(topology.portalIdAt(turn.here * 4 + d)))
                score += V3_EXPLORED_BONUS;
#elif !FEAT_PORTAL_EXPLORE
            if (isPortal) score += 25;
#endif
            if (world.seenRound[dest] < 0) score += 130;

            int queenCell = friendlyQueenCell();
            if (turn.isQueen()) {
                if (isPortal) score -= 900;
                int nearest = INF;
                for (int eh : turn.enemyHeads) nearest = std::min(nearest, topology.manh(dest, eh));
                if (nearest < INF) score -= (long)std::max(0, 8 - nearest) * 90;
                if (eats && nearest <= 5) score -= 700;
            } else if (queenCell >= 0) {
                int queenDist = topology.manh(dest, queenCell);
                if (context.roles.escort) {
                    if (queenDist < 2) score -= 1200;
                    else if (queenDist <= 4) score += 220;
                    else score -= (long)(queenDist - 4) * 70;
                } else if (queenDist <= 2) score -= 1800;
                else if (queenDist <= 4) score -= 700;
            }
            score -= (long)crowd * 35;
            score -= long(coordination.stepRisk(turn.here, d)) * 250;
            if (opens <= 1 && !eats) score -= 80;
            if (space < flr) score -= (long)(flr - space) * 120;
#ifdef DEBUG_PORTAL
            debug.scores[d] = score;
#endif
#if SEED
            score += ((uint32_t)(SEED * 2654435761u) ^ (uint32_t)(turn.myId * 40503u) ^ (uint32_t)(turn.round * 9973u) ^
                      (uint32_t)(d * 7919u)) *
                         2246822519u >>
                     29;
#endif

            if (best < 0 || threat < bestThreat || (threat == bestThreat && score > bestScore)) {
                bestThreat = threat;
                bestScore = score;
                best = d;
            }
        }

        if (best < 0) {
            if (ramDir >= 0) return Action::move(ramDir);
            // Relax soft vetoes and try a legal split before accepting death.
            return finalizeAction(ct, {});
        }
        return Action::move(best);
    }
};

} // namespace bot
