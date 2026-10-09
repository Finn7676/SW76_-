#pragma once

#include "search.hpp"
#include "body.hpp"
#include "roles.hpp"
#include "debug.hpp"

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
    EXPLORE
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
    std::vector<uint8_t> isGoal;
    std::vector<int> routeCrowd;
    bool routeCrowdReady = false;
    TurnQueryCache queries;
    Mission mission;
    // Keep the long-range frontier while a temporary pearl task is active.
    Situation currentSituation;
    BehaviorState currentBehavior = BehaviorState::IDLE;
    int claimPortalId = -1;
    int donationTargetId = -1, donationProgressRound = -1, donationLastDistance = INF;
    std::map<int, int> donationIgnoreUntil;

  public:
    Planner(GameConfig const &config, TurnState const &turn, WorldModel const &world, Topology const &topology,
            OccupancyModel const &occupancy, Search &search, BodyMemory const &body)
        : config(config), turn(turn), world(world), topology(topology), occupancy(occupancy), search(search),
          body(body) {}
    int activeClaimPortal() const { return claimPortalId; }
    Mission const &currentMission() const { return mission; }
    Situation const &situation() const { return currentSituation; }
    BehaviorState behavior() const { return currentBehavior; }
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

    // Called after the final action guard, so a pocket mission is recorded
    // only when the dragon actually commits to entering that pocket.
    void rememberChosenAction(Action const &action) {
        if (action.type == ActionType::SPLIT && mission.type == MissionType::PEARL_POCKET) {
            mission = {};
            return;
        }
        if (action.type != ActionType::MOVE || mission.type == MissionType::PEARL_POCKET ||
            donationTargetId >= 0) return;
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
        currentBehavior = BehaviorState::IDLE;
        routeCrowdReady = false;
        queries.reset();
        DecisionContext context;
        context.roles = RolePolicy::assign(config, turn, world);
        context.profile = {turn.length, ct.can_split(2), context.roles.champion, context.roles.core,
                           context.roles.sacrificeCandidate};
        context.situation = buildSituation(context);
        currentSituation = context.situation;
        updateMission(context);
        PortalRoute scoutRoute;
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
        unsigned scoutSafeFirst = 0;
#endif
        RamCandidate ram = adjacentRam(context.profile.championProtected);

        // A profitable head trade is a tactical survival action. Handle it
        // before proximity alone sends the dragon into generic escape scoring,
        // where occupied enemy-head cells are deliberately filtered out.
#if FEAT_RAM_FIRST
        if (ram.good) {
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

        // Before round 350, give away every segment past the minimum so the
        // team stays a crowd of short dragons. The child splits again on its
        // own turn if it is still long.
        if (turn.round < V4_SUMMON_ROUND && turn.length >= 4 && ct.can_split(turn.length - 2)) {
            currentBehavior = BehaviorState::EXPAND;
            return Action::split(turn.length - 2);
        }

        // Keep the existing late-game donation policy in its own decision
        // branch; it is independent of ordinary tactical and growth states.
        if (Action a = sacrifice(ct, context)) return a;

        // P1: fight only when an enemy can affect the near-term route, or an
        // immediate attack is already profitable.
        if (context.situation.enemyRelevant) {
            currentBehavior = BehaviorState::COMBAT;
#if FEAT_RAM_FIRST
            if (ram.good) return Action::move(ram.dir);
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

        // P2: take a safe pearl that is about to be lost or can be collected
        // with one move before another long-term task. Short quiet-endgame
        // dragons leave those pearls for a longer teammate.
        if (context.situation.immediatePearlOpportunity && !context.roles.yieldPearls) {
            currentBehavior = BehaviorState::COLLECT;
            int planDir = routeDirection(ct, context);
            return scoreAndFallback(ct, context, ram.dir, planDir, false, false
#ifdef DEBUG_PORTAL
                                    , debug
#endif
            );
        }

        // P3: separate nearby explorers when several friendly heads compete
        // for the same few cells. Collection and frontier missions survive
        // this one-turn detour; an assigned portal scout keeps its route.
        if (shouldDisperse(context) && mission.type != MissionType::SCOUT_PORTAL &&
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
        if (scoutObjectiveAllowed && context.situation.knownPortals) {
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
        bool missionBlocksExpansion = scoutRoute.dir >= 0 || !missionAllowsExpansion() ||
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
    static constexpr int SCOUT_MISSION_STALE_ROUNDS = 12;

    struct RamCandidate {
        int dir;
        bool good;
    };

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
    Situation buildSituation(DecisionContext const &context) const {
        (void)context;
        Situation s;
        s.enemyVisible = !turn.enemyHeads.empty();
        s.pearlsVisible = !turn.visPearls.empty();
        s.knownPortals = !world.portalKeys.empty();
        s.deathCluesPresent = !world.deathClues.empty();
        for (int c : turn.visPearls)
            if (topology.manh(turn.here, c) <= 1) s.immediatePearlOpportunity = true;
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
            if (close || (confirmedShort && distance <= 8)) s.enemyRelevant = true;
            if (distance <= 1 || (confirmedShort && distance <= 2)) s.immediateThreat = true;
        }
#if FEAT_V32_ENEMY_RELEVANCE
#else
        s.enemyRelevant = s.enemyVisible;
        s.immediateThreat = s.enemyVisible;
#endif
        return s;
    }
    bool hasPearlTarget() const {
        if (!turn.visPearls.empty()) return true;
        for (int c = 0; c < config.cells; ++c) {
            int due = world.pearlDue[c];
            if (world.pearlInfo[c] >= turn.round - 6 && due >= turn.round && due - turn.round <= 8)
                return true;
        }
        return false;
    }
    bool shouldDisperse(DecisionContext const &context) const {
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
            if (eats && context.roles.yieldPearls) continue;
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
        if (mission.type == MissionType::PEARL_POCKET) {
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
            bool complete = world.exploredPortals.count(mission.targetId) != 0;
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
            if (!valid || stale) mission = {};
        }
#else
        (void)context;
#endif
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
    RamCandidate adjacentRam(bool champion) const {
        // ---- adjacent enemy heads we could ram this move (portal aware)
        int ramDir = -1, ramSz = -1;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(turn.here, d);
            if (q < 0 || !occupancy.enemyHeadAt(q)) continue;
            int sz = turn.countOf(occupancy.dragonId(q));
            if (sz > ramSz) {
                ramSz = sz;
                ramDir = d;
            }
        }
        bool protectChampion = champion && (turn.length > 5 || ramSz <= turn.length);
        bool ramGood = ramDir >= 0 && ramSz >= turn.length - 1 && ramSz >= 3 &&
                       !protectChampion && turn.population > 1;

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
        if (deathDir < 0 && scoutRoute.dir >= 0 && context.situation.pearlsVisible &&
            !context.roles.yieldPearls) {
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
        return scoreAndFallback(ct, context, ram.dir, planDir, deathDir >= 0, false
#ifdef DEBUG_PORTAL
                                ,
                                debug
#endif
        );
    }
    Action deliverSummon(DecisionContext const &context) {
        // A short dragon that has heard a longer teammate's sonar gives its
        // body immediately. Escorts ignore the call and keep scouting.
        if (turn.round < V4_SUMMON_ROUND || context.situation.enemyVisible) return {};
        if (!EscortPolicy::small(turn) || EscortPolicy::isEscort(turn)) return {};
        if (!EscortPolicy::hasSummon(turn, world)) return {};
        return Action::suicide();
    }
    Action sacrifice(Controller const &ct, DecisionContext const &context) {
#if FEAT_SACRIFICE
        if (Action gift = deliverSummon(context)) return gift;
        if (context.situation.enemyVisible || !DonationPolicy::quiet(config, turn, world)) return {};
        // Escorts stay alive and only report pearls. Other short dragons still donate.
        if (EscortPolicy::isEscort(turn)) return {};
        DonationPolicy policy = DonationPolicy::at(config, turn);
        unsigned donationSafeFirst = safeScoutFirstSteps(ct);
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(turn.here, d);
            if (q < 0 || enemyAttackRisk(q, turn.length +
                    int(occupancy.visible(q) && occupancy.hasPearl(q))) != 0 ||
                blocksTeammateEscape(Action::move(d))) donationSafeFirst &= ~(1u << d);
        }
        if (EscortPolicy::small(turn)) {
            unsigned avoidingPearls = donationSafeFirst;
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(turn.here, d);
                if (q >= 0 && occupancy.visible(q) && occupancy.hasPearl(q))
                    avoidingPearls &= ~(1u << d);
            }
            if (avoidingPearls) donationSafeFirst = avoidingPearls;
        }

        // A surviving large dragon gathers known pearls from a dead teammate
        // before the ordinary exploration and expansion branches can distract it.
        if (turn.length > V4_ESCORT_MAX_LENGTH && !world.deathClues.empty()) {
            if (donationSafeFirst) {
                prepareRouteCrowd();
                int dir = search.bfsFirstDir(
                    [this](int c) {
                        if (!occupancy.visible(c) || !occupancy.hasPearl(c)) return false;
                        return std::any_of(world.deathClues.begin(), world.deathClues.end(),
                            [this, c](DeathClue const &clue) {
                                return clue.friendly && clue.c == c && turn.round - clue.round <= 12;
                            });
                    },
                    [this](int c, int step) { return scoutTileScore(c, step); }, 12, 512,
                    donationSafeFirst);
                if (dir >= 0) {
                    currentBehavior = BehaviorState::COLLECT;
                    return Action::move(dir);
                }
            }
        }

        if (!policy.donor(turn)) {
            donationTargetId = -1;
            donationProgressRound = -1;
            donationLastDistance = INF;
            return {};
        }
        struct Recipient { int id, cell, length, distance; };
        std::vector<Recipient> recipients;
        for (auto const &[id, rally] : world.allyRallies) {
            if (id == turn.myId || turn.round - rally.round > V4_DONATION_REPORT_AGE ||
                rally.c < 0 || rally.c >= config.cells || !policy.recipient(turn, rally.length) ||
                rally.length <= V4_ESCORT_MAX_LENGTH) continue;
            recipients.push_back({id, rally.c, rally.length, topology.manh(turn.here, rally.c)});
        }
        // Prefer a large recipient, then a short rendezvous. Keep walking toward
        // a viable remote recipient so sparse maps can still build one giant.
        for (auto it = donationIgnoreUntil.begin(); it != donationIgnoreUntil.end();) {
            if (turn.round >= it->second) it = donationIgnoreUntil.erase(it);
            else ++it;
        }
        auto target = std::find_if(recipients.begin(), recipients.end(),
            [this](Recipient const &r) { return r.id == donationTargetId; });
        if (target != recipients.end() && donationProgressRound >= 0 &&
            turn.round - donationProgressRound > 6) {
            donationIgnoreUntil[donationTargetId] = turn.round + 6;
            donationTargetId = -1;
            target = recipients.end();
        }
        int preferredId = target != recipients.end() ? target->id : -1;
        std::sort(recipients.begin(), recipients.end(), [preferredId](Recipient const &a, Recipient const &b) {
            if (preferredId >= 0 && (a.id == preferredId) != (b.id == preferredId))
                return a.id == preferredId;
            int as = a.length * 16 - a.distance * 5;
            int bs = b.length * 16 - b.distance * 5;
            return as != bs ? as > bs : a.id < b.id;
        });
        if (recipients.size() > 8) recipients.resize(8);

        for (Recipient const &r : recipients) {
            if (donationIgnoreUntil.count(r.id)) continue;
            int routeDir = -1, routeDistance = INF;
            // If the recipient can safely reach the pearl on our head in at
            // most two steps, donate now; a longer approach searches a nearby
            // known pickup cell and continues on later turns.
            bool canCollect = false;
            if (r.distance <= 2 && occupancy.visible(r.cell) && occupancy.occupied(r.cell) &&
                occupancy.teammateHeadAt(r.cell) && occupancy.dragonId(r.cell) == r.id) {
                std::array<int, 3> path{};
                auto walk = [&](auto &&self, int c, int depth) -> void {
                    if (canCollect || depth >= 2) return;
                    for (int d = 0; d < 4; ++d) {
                        uint8_t edge = topology.edgeTypeAt(c * 4 + d);
                        if (edge == E_KELP || edge == E_UNKNOWN) continue;
                        int q = topology.step(c, d);
                        if (q < 0 || !occupancy.visible(q) ||
                            std::find(path.begin(), path.begin() + depth, q) != path.begin() + depth) continue;
                        bool donorPart = occupancy.occupied(q) && occupancy.dragonId(q) == turn.myId;
                        if (q != turn.here && occupancy.occupied(q) && !donorPart &&
                            occupancy.blockedAt(q, depth + 1)) continue;
                        path[depth] = q;
                        if (q == turn.here) {
                            bool exit = false;
                            for (int e = 0; e < 4; ++e) {
                                uint8_t nextEdge = topology.edgeTypeAt(q * 4 + e);
                                int next = topology.step(q, e);
                                if (nextEdge == E_UNKNOWN || next < 0 || next == r.cell || next == turn.here ||
                                    std::find(path.begin(), path.begin() + depth + 1, next) != path.begin() + depth + 1)
                                    continue;
                                bool openAfterPickup = !occupancy.occupied(next) ||
                                    occupancy.dragonId(next) == turn.myId ||
                                    (occupancy.dragonId(next) == r.id &&
                                     !occupancy.blockedAt(next, depth + 2, true));
                                if (occupancy.visible(next) && openAfterPickup) exit = true;
                            }
                            canCollect = exit;
                        } else if (!occupancy.occupied(q) || donorPart) self(self, q, depth + 1);
                    }
                };
                walk(walk, r.cell, 0);
            }
            if (canCollect) {
                donationTargetId = -1;
                donationProgressRound = -1;
                donationLastDistance = INF;
                return Action::suicide();
            }

            if (!donationSafeFirst) continue;
            std::array<uint8_t, 4> goals{};
            for (int d = 0; d < 4; ++d) {
                uint8_t edge = topology.edgeTypeAt(r.cell * 4 + d);
                int q = topology.step(r.cell, d);
                if (edge == E_KELP || q < 0 || q == turn.here ||
                    (occupancy.visible(q) && occupancy.occupied(q) &&
                     occupancy.dragonId(q) != r.id)) continue;
                goals[d] = 1;
            }
            routeDir = search.bfsFirstDir(
                [&goals, &r, this](int c) {
                    for (int d = 0; d < 4; ++d)
                        if (goals[d] && topology.step(r.cell, d) == c) return true;
                    return false;
                }, [](int, int) { return 0L; }, 12, 512, donationSafeFirst, nullptr, &routeDistance);
            if (routeDir < 0 && r.distance > 12) {
                routeDir = search.bfsFirstDir(
                    [this, &r](int c) {
                        return topology.manh(c, r.cell) < topology.manh(turn.here, r.cell);
                    }, [](int, int) { return 0L; }, 6, 128, donationSafeFirst, nullptr, &routeDistance);
            }
            if (routeDir >= 0 && routeDistance <= 12) {
                if (r.distance < donationLastDistance || donationTargetId != r.id) {
                    donationLastDistance = r.distance;
                    donationProgressRound = turn.round;
                }
                donationTargetId = r.id;
                currentBehavior = BehaviorState::EXPLORE;
                return Action::move(routeDir);
            }
        }
#endif
        return {};
    }
    Action tacticalSplit(Controller const &ct) const {
#if FEAT_V3_TACTICAL_SPLIT
        // The child acts later this round from the old tail. Only target an
        // enemy that has already acted, so its visible head will stay put.
        if (turn.round >= CH_START || turn.length < 8 || !ct.can_split(2)) return {};
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
    Action expansion([[maybe_unused]] Controller const &ct, DecisionContext const &context, bool activeScoutTask) {
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
        // Short dragons stop splitting once consolidation starts; new children
        // would only become more bodies to donate.
        if (consolidating && EscortPolicy::small(turn)) return {};
        bool wantExpand = consolidating ? turn.population < donation.reserve
                          : (turn.round < 470 && !context.roles.champion) || turn.population < 8;
        if (!activeScoutTask && wantExpand && turn.population < config.targetPopulation && context.profile.length >= 4 &&
            !context.profile.coreProtected && context.profile.canSplit) {
            return Action::split(2);
        }

        return {};
    }
    Action sprintRam([[maybe_unused]] DecisionContext const &context) const {
#if FEAT_SPRINT_RAM
        if (turn.length < 2 || turn.population <= 1 ||
            (context.roles.champion && turn.length > 5)) return {};

        // Bounded search of currently observed routes. Other dragons cannot
        // move between sprint steps, so their body cells remain blocked.
        // Each extra step checks payment BEFORE collision or pearl eating;
        // an earlier pearl can fund it, a pearl on this step cannot.
        constexpr int maxSteps = 4;
        std::vector<int> initialBody;
        initialBody.reserve(turn.length);
        for (int i = 0; i < turn.length; ++i)
            initialBody.push_back(i == 0 ? turn.here : body.cellAtTailOffset(turn.length - 1 - i));
        std::array<uint8_t, 8> path{};
        std::vector<int> pathCells;
        pathCells.reserve(maxSteps);
        Action best;
        int bestEnemySize = -1, bestSteps = 99;
        auto visit = [&](auto &&self, int head, int steps, std::vector<int> const &segments) -> void {
            int length = int(segments.size());
            if (steps >= maxSteps || (steps > 0 && length <= 2)) return;
            for (int d = 0; d < 4; ++d) {
                if (topology.edgeTypeAt(head * 4 + d) == E_KELP) continue;
                int q = topology.step(head, d);
                if (q < 0 || !occupancy.visible(q)) continue;
                // The tail is still occupied when collision is checked.
                if (std::find(segments.begin(), segments.end(), q) != segments.end()) continue;
                path[steps] = uint8_t(d);
                if (occupancy.occupied(q)) {
                    if (occupancy.enemyHeadAt(q)) {
                        int size = turn.countOf(occupancy.dragonId(q));
                        bool worth = size >= length + 2 && size >= 5;
                        if (context.roles.attrition && size >= length + 1 && size >= 4) worth = true;
                        if (steps > 0 && worth &&
                            (size > bestEnemySize || (size == bestEnemySize && steps + 1 < bestSteps))) {
                            best = {ActionType::SPRINT, path, uint8_t(steps + 1), 0};
                            bestEnemySize = size;
                            bestSteps = steps + 1;
                        }
                        continue; // a collision ends the sprint
                    }
                    if (!occupancy.isMine(q) || occupancy.dragonId(q) != turn.myId) continue;
                    // An own-body cell is traversable only if its known
                    // segment was actually removed by previous sprint steps.
                    if (std::find(initialBody.begin(), initialBody.end(), q) == initialBody.end()) continue;
                }
                bool eats = occupancy.hasPearl(q) &&
                    std::find(pathCells.begin(), pathCells.end(), q) == pathCells.end();
                std::vector<int> after = segments;
                if (!eats) after.pop_back();
                after.insert(after.begin(), q);
                if (steps > 0) after.pop_back();
                if (shortEnemyRiskDistance(q, int(after.size())) <= 2) continue;
                pathCells.push_back(q);
                self(self, q, steps + 1, after);
                pathCells.pop_back();
            }
        };
        visit(visit, turn.here, 0, initialBody);
        return best;
#endif

        return {};
    }
    int shortEnemyRiskDistance(int c, int ourLength = -1) const {
#if FEAT_V3_SHORT_AVOID
        // The same relative threat threshold applies at every own length.
        int maxThreatLength = (ourLength < 0 ? turn.length : ourLength) - 3;
        if (maxThreatLength < 1) return INF;
        int nearest = INF;
        for (size_t i = 0; i < turn.enemyHeads.size(); ++i) {
            if (i >= turn.enemyHeadExactLen.size() || i >= turn.enemyHeadLen.size()) continue;
            int exact = turn.enemyHeadExactLen[i];
            // A partial body is only a lower bound. If it has already
            // exceeded the threshold, this enemy cannot be short enough.
            if (exact > maxThreatLength || (exact < 0 && turn.enemyHeadLen[i] > maxThreatLength)) continue;
            int head = turn.enemyHeads[i];
            int distance = topology.manh(c, head);
            for (int d = 0; d < 4; ++d)
                if (topology.step(c, d) == head) distance = std::min(distance, 1);
            nearest = std::min(nearest, distance);
        }
        return nearest;
#else
        (void)c; (void)ourLength;
        return INF;
#endif
    }
    // 0 = no observed short-enemy attack route, 1 = uncertain (partial enemy
    // body or route visibility), 2 = confirmed attack reach. Mirrors normal
    // rams, pearl-funded sprints, and the early tail-spawned split attack.
    int enemyAttackRisk(int target, int ourLength) const {
#if FEAT_V3_SHORT_AVOID
        int bestRisk = 0;
        int const shortLimit = ourLength - 3;
        if (shortLimit < 1) return 0;

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

            if (exact >= 0 && exact > shortLimit) continue;
            if (exact < 0 && observedLength > shortLimit) continue;

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
            pathCells.reserve(4);
            bool hiddenRoute = false;
            int enemyRisk = 0;

            auto visit = [&](auto &&self, int head, int steps, int length,
                             std::vector<int> const &segments) -> void {
                if (steps >= 4 || (steps > 0 && length <= 2)) return;
                for (int d = 0; d < 4; ++d) {
                    int key = head * 4 + d;
                    uint8_t edge = topology.edgeTypeAt(key);
                    if (edge == E_KELP) continue;
                    int q = topology.step(head, d);
                    if (q < 0) {
                        if (edge == E_PORTAL) hiddenRoute = true;
                        continue;
                    }
                    if (q == target) {
                        if (length <= shortLimit)
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
                        occupancy.blockedAt(q, 2 + steps)) continue;

                    bool eats = occupancy.hasPearl(q) &&
                        std::find(pathCells.begin(), pathCells.end(), q) == pathCells.end();
                    std::vector<int> after = segments;
                    if (!eats && !after.empty()) after.pop_back();
                    after.insert(after.begin(), q);
                    if (steps > 0 && !after.empty()) after.pop_back();
                    int afterLength = length + (eats ? 1 : 0) - (steps > 0 ? 1 : 0);
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
                    ? std::min(4, std::max(1, exact - 1 + pearlFuel))
                    : 4;
                if (topology.manh(enemyHead, target) <= possibleSteps)
                    bestRisk = std::max(bestRisk, 1);
            }
        }
        return bestRisk;
#else
        (void)target; (void)ourLength;
        return 0;
#endif
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
                    if (clue.c == c && turn.round - clue.round <= 2 &&
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
        long score = world.seenRound[c] == 0 ? 130 : 0;
        if (eats && EscortPolicy::yieldsPearls(config, turn, world)) score -= 8000;
        score -= long(routeCrowd[c]) * 35;
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
        bool canSalvage = turn.population < std::min(config.targetPopulation, ct.unit_limit) && turn.length >= 2 &&
                          turn.length + pocket.pearls >= 4;
        if (!canSalvage) return false;
        return pocket.pearls >= 1 && (pocket.pearls >= 3 || !hasOtherSafeMove(dir));
    }
    unsigned safeScoutFirstSteps(Controller const &ct) {
        unsigned safeFirst = 0;
        for (int d = 0; d < 4; ++d) {
            int q = topology.step(turn.here, d);
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
    bool freshReportedPearl(int c) const {
        int due = world.pearlDue[c];
        int info = world.pearlInfo[c];
        if (c == turn.here || info < turn.round - V4_PEARL_REPORT_AGE || due < 0 || due >= INF || due > 511)
            return false;
        if (occupancy.visible(c) && !occupancy.hasPearl(c) && due <= turn.round) return false;
        return due >= turn.round - V4_PEARL_REPORT_AGE;
    }
    int routeDirection(Controller const &ct, DecisionContext const &context) {
        (void)ct;
        prepareRouteCrowd();
        constexpr int depth = 24, nodes = 320;
        auto tileScore = [this](int c, int step) { return scoutTileScore(c, step); };
        isGoal.assign(config.cells, 0);
        int goal = -1, resourceDistance = INF;
        int resourceDir = -1;
        bool anyResource = context.situation.immediatePearlOpportunity && !context.roles.yieldPearls;
        if (!context.roles.yieldPearls)
            for (int c : turn.visPearls) {
                isGoal[c] = 1;
                anyResource = true;
            }
        if (turn.round >= V4_SUMMON_ROUND && turn.length > V4_ESCORT_MAX_LENGTH) {
            int reportedDir = search.bfsFirstDir(
                [this](int c) { return freshReportedPearl(c); }, tileScore, depth, nodes, 15, &goal,
                &resourceDistance);
            if (reportedDir >= 0) {
                mission = {MissionType::COLLECT_RESOURCE, goal, -1, turn.round, turn.round, resourceDistance};
                return reportedDir;
            }
        }
        if (context.roles.escort) {
            std::vector<int> reported;
            for (int c = 0; c < config.cells; ++c)
                if (freshReportedPearl(c)) reported.push_back(c);
            int watchDir = reported.empty() ? -1 : search.bfsFirstDir(
                [this, &reported](int c) {
                    if (occupancy.visible(c) && occupancy.hasPearl(c)) return false;
                    for (int pearl : reported) {
                        int distance = topology.cheb(c, pearl);
                        if (distance >= 2 && distance <= 3 && topology.cheb(turn.here, pearl) > 3) return true;
                    }
                    return false;
                },
                tileScore, depth, nodes, 15);
            if (watchDir >= 0) return watchDir;
        }
        if (!context.roles.yieldPearls)
            for (int c = 0; c < config.cells; ++c) {
                int due = world.pearlDue[c];
                if (due >= 0 && due < INF && due - turn.round <= 8) {
                    isGoal[c] = 1;
                    anyResource = true;
                }
            }

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
        int frontierDir = search.bfsFirstDir([&](int q) { return world.seenRound[q] == 0; }, tileScore,
                                             depth, nodes, 15, &frontier, &frontierDistance);
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
            if (e == E_KELP) continue;
            int dest = topology.step(turn.here, d);
            bool isPortal = e == E_PORTAL;
            if (dest < 0) {
                if (!isPortal) continue;
                // unresolved portal: a mild exploration option
                long ps = 30;
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

            if (pearlInView && !context.roles.yieldPearls) {
                score -= (long)std::min(search.visPearlDist(dest), 14) * 25;
                if (eats) score += 500;
            }
            if (context.roles.yieldPearls && eats) score -= 8000;
            if (d == planDir) {
                if (deathRoute) score += 250;
                else if (currentBehavior == BehaviorState::COMBAT) score += 30;
                else if (mission.type == MissionType::COLLECT_RESOURCE) score += 500;
                else if (context.roles.escort) score += 220;
            }
#if FEAT_V3_EXPLORED_BONUS
            if (isPortal && world.exploredPortals.count(topology.portalIdAt(turn.here * 4 + d)))
                score += V3_EXPLORED_BONUS;
#elif !FEAT_PORTAL_EXPLORE
            if (isPortal) score += 25;
#endif
            if (world.seenRound[dest] == 0) score += 130;

            score -= (long)crowd * 35;
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
#if FEAT_REVERSE_SPLIT
            // trapped: cut everything but the front two segments loose; the child faces the way we came
            if (turn.length >= 5 && ct.can_split(turn.length - 2) && body.tailCanEscape()) {
                return Action::split(turn.length - 2);
            }
#endif
            if (ct.can_split(2)) {
                return Action::split(2);
            }
        }
        if (best < 0) {
            // nothing is safe: prefer a head we can at least trade with, else any open edge
            if (ramDir >= 0) {
                return Action::move(ramDir);
            }
            int bs = -1;
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(turn.here, d);
                if (q < 0 || occupancy.teammateHeadAt(q)) continue;
                if (!pocketWorthEntering(adjacentPocket(d), d, ct)) continue;
                int s = occupancy.occupied(q) ? 0 : 1;
                if (s > bs) {
                    bs = s;
                    best = d;
                }
            }
            if (best < 0) return {}; // default self-sacrifice spares teammate heads
        }
        return Action::move(best);
    }
};

} // namespace bot
