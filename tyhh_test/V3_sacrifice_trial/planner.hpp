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
};

enum class MissionType : uint8_t { NONE, SCOUT_PORTAL, HUNT_TARGET };

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
    struct PocketCacheEntry {
        int start = -1;
        int entrance = -1;
        int maxNodes = 0;
        PocketInfo result;
    };
    struct TurnQueryCache {
        std::array<PocketCacheEntry, 32> pockets{};
        size_t pocketCount = 0;
        void reset() { pocketCount = 0; }
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
    Situation currentSituation;
    int claimPortalId = -1;

  public:
    Planner(GameConfig const &config, TurnState const &turn, WorldModel const &world, Topology const &topology,
            OccupancyModel const &occupancy, Search &search, BodyMemory const &body)
        : config(config), turn(turn), world(world), topology(topology), occupancy(occupancy), search(search),
          body(body) {}
    int activeClaimPortal() const { return claimPortalId; }
    Mission const &currentMission() const { return mission; }
    Situation const &situation() const { return currentSituation; }
    Action avoidTeammateHead(Action action) const {
        if (action.type != ActionType::MOVE && action.type != ActionType::SPRINT) return action;
        int c = turn.here;
        for (int i = 0; i < action.steps; ++i) {
            c = topology.step(c, action.dirs[i]);
            if (c < 0) break; // an unknown portal exit cannot be inspected
            if (occupancy.teammateHeadAt(c)) return {};
        }
        return action;
    }

    // The policy priority is explicit here. Each phase returns no action when inapplicable.
    Action choose(Controller const &ct
#ifdef DEBUG_PORTAL
                  ,
                  PortalDebug &debug
#endif
    ) {
        claimPortalId = -1;
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
        if (Action a = preExpansionPhase(ct, context, ram)) return a;
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
        if (context.roles.scout && context.situation.knownPortals) {
            prepareRouteCrowd();
            scoutSafeFirst = safeScoutFirstSteps(ct);
            if (mission.type == MissionType::SCOUT_PORTAL && FEAT_V32_SCOUT_MISSION) {
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
        bool missionBlocksExpansion = scoutRoute.dir >= 0 || !missionAllowsExpansion();
#if FEAT_V32_SEVERE_BLOCKAGE
        missionBlocksExpansion = missionBlocksExpansion || context.situation.severeBlockedness;
#endif
        if (Action a = expansion(ct, context, missionBlocksExpansion)) return a;
        if (Action a = postExpansionPhase(ct, context, ram)) return a;
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
    struct RamCandidate {
        int dir;
        bool good;
    };
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
        for (int f : turn.friendParts)
            if (topology.cheb(turn.here, f) <= 3) s.mildCrowding = true;
        for (size_t i = 0; i < turn.enemyHeads.size(); ++i) {
            int distance = topology.manh(turn.here, turn.enemyHeads[i]);
            bool close = distance <= 3;
            bool shortEnough = i < turn.enemyHeadLen.size() && turn.enemyHeadLen[i] <= turn.length - 3;
            if (close || shortEnough) s.enemyRelevant = true;
            if (distance <= 1 || (shortEnough && distance <= 2)) s.immediateThreat = true;
        }
#if FEAT_V32_ENEMY_RELEVANCE
#else
        s.enemyRelevant = s.enemyVisible;
        s.immediateThreat = s.enemyVisible;
#endif
        return s;
    }
    void beginScoutMission(PortalRoute const &route) {
#if FEAT_V32_SCOUT_MISSION
        if (route.pid < 0) return;
        if (mission.type != MissionType::SCOUT_PORTAL || mission.targetId != route.pid) {
            mission = {MissionType::SCOUT_PORTAL, -1, route.pid, turn.round, turn.round, route.distance};
        } else {
            mission.lastProgressRound = turn.round;
            mission.lastDistance = route.distance;
        }
#else
        (void)route;
#endif
    }
    void updateMission(DecisionContext const &context) {
#if FEAT_V32_SCOUT_MISSION
        if (mission.type == MissionType::SCOUT_PORTAL) {
            bool invalid = mission.targetId < 0 || context.roles.champion || context.roles.hunter ||
                           context.roles.core || turn.round > (config.cells >= 1200 ? 300 : 220);
            bool complete = world.exploredPortals.count(mission.targetId) != 0;
            if (invalid || complete) mission = {};
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
    PocketInfo pocketBeyond(int start, int entrance, int maxNodes = 24) {
        for (size_t i = 0; i < queries.pocketCount; ++i) {
            PocketCacheEntry const &entry = queries.pockets[i];
            if (entry.start == start && entry.entrance == entrance && entry.maxNodes == maxNodes)
                return entry.result;
        }
        PocketInfo result = search.pocketBeyond(start, entrance, maxNodes);
        if (queries.pocketCount < queries.pockets.size())
            queries.pockets[queries.pocketCount++] = {start, entrance, maxNodes, result};
        return result;
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
        bool ramGood = ramDir >= 0 && ramSz >= turn.length - 1 && ramSz >= 3 && !champion && turn.population > 1;

        return {ramDir, ramGood};
    }
    Action preExpansionPhase(Controller const &ct, DecisionContext const &context,
                             RamCandidate const &ram) const {
        if (Action a = sacrifice(context)) return a;
#if FEAT_RAM_FIRST
        if (ram.good && turn.population >= 4) return Action::move(ram.dir);
#endif
        if (Action a = tacticalSplit(ct)) return a;
        return {};
    }
    Action postExpansionPhase(Controller const &ct, DecisionContext const &context,
                              RamCandidate const &ram) {
        if (ram.good) return Action::move(ram.dir);
        if (Action a = sprintRam(context)) return a;
        if (Action a = sprintPearls(ct, context)) return a;
        return {};
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
        int planDir = deathDir >= 0 ? deathDir : routeDirection(context);
        return scoreAndFallback(ct, context, ram.dir, planDir, deathDir >= 0
#ifdef DEBUG_PORTAL
                                ,
                                debug
#endif
        );
    }
    Action sacrifice(DecisionContext const &context) const {
#if FEAT_SACRIFICE
        // No-action death drops pearls at our current body without moving into
        // (and possibly killing) a teammate's head. Donors must be near the
        // head of an observed, much longer friend, with no fresh enemy intel.
        if (!context.profile.sacrificeEligible || context.roles.scout || context.situation.enemyVisible) return {};
        int quietRounds = turn.round - world.lastQuietTeammateReportRound <= V3_QUIET_REPORT_MAX_AGE
                              ? V3_SACRIFICE_ALLY_QUIET_ROUNDS : V3_SACRIFICE_QUIET_ROUNDS;
        if (turn.round - world.lastEnemyIntelRound < quietRounds) return {};
        for (auto const &part : turn.parts) {
            if (!part.mine || !part.head || part.id == turn.myId ||
                topology.manh(turn.here, part.c) > 3) continue;
            int observedLength = turn.countOf(part.id);
            if (observedLength >= V3_SACRIFICE_MIN_ALLY_LENGTH &&
                observedLength >= turn.length + 4) return Action::suicide();
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
    Action expansion(Controller const &ct, DecisionContext const &context, bool activeScoutTask) const {
        // ---- 1) keep the swarm at the cap
        bool wantExpand = (turn.round < 470 && !context.roles.champion) || turn.population < 8;
        if (!activeScoutTask && wantExpand && turn.population < config.targetPopulation && context.profile.length >= 4 &&
            !context.profile.coreProtected && ct.can_split(2)) {
            return Action::split(2);
        }

        return {};
    }
    Action sprintRam([[maybe_unused]] DecisionContext const &context) const {
#if FEAT_SPRINT_RAM
        // ---- 2b) sprint two tiles into a valuable enemy head
        if (turn.length >= 3 && !context.roles.champion && turn.population >= 4) {
            int bestSz = -1, b1 = -1, b2 = -1;
            for (int d1 = 0; d1 < 4; ++d1) {
                int m = topology.step(turn.here, d1);
                if (m < 0 || occupancy.blockedAt(m, 1) || occupancy.teammateHeadAt(m)) continue;
                if (shortEnemyRiskDistance(m) <= 2) continue;
                for (int d2 = 0; d2 < 4; ++d2) {
                    int q = topology.step(m, d2);
                    if (q < 0 || q == turn.here || !occupancy.enemyHeadAt(q) || occupancy.teammateHeadAt(q)) continue;
                    if (shortEnemyRiskDistance(q) <= 2) continue;
                    int sz = turn.countOf(occupancy.dragonId(q));
                    if (sz > bestSz) {
                        bestSz = sz;
                        b1 = d1;
                        b2 = d2;
                    }
                }
            }
            bool worth = bestSz >= turn.length + 2 && bestSz >= 5;
            if (context.roles.attrition && bestSz >= turn.length + 1 && bestSz >= 4) worth = true;
            if (b1 >= 0 && worth) {
                return Action::sprint2(b1, b2);
            }
        }
#endif

        return {};
    }
    int shortEnemyRiskDistance(int c) const {
#if FEAT_V3_SHORT_AVOID
        // The same relative threat threshold applies at every own length.
        int maxThreatLength = turn.length - 3;
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
        (void)c;
        return INF;
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
                if (!pocketWorthEntering(pocketBeyond(m, turn.here), d1, ct)) continue;
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
        bool canSalvage = turn.population < ct.unit_limit && turn.length + pocket.pearls >= 4 &&
                          body.tailCanEscape();
        if (!canSalvage) return false;
        return pocket.pearls >= 3 || (pocket.pearls >= 1 && !hasOtherSafeMove(dir));
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
            PocketInfo pocket = pocketBeyond(q, turn.here);
            if (!pocketWorthEntering(pocket, d, ct)) continue;
            bool eats = occupancy.visible(q) && occupancy.hasPearl(q);
            if (pocket.proven && pocket.pearls >= 3) {
                safeFirst |= 1u << d;
                continue;
            }
            if (search.onwardSafe(q, eats) <= 1 ||
                search.spaceFrom(q, eats, 12, false) < std::min(turn.length + 1, 8)) continue;
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
            if (search.onwardSafe(q, eats) <= 1 ||
                search.spaceFrom(q, eats, 12, false) < std::min(turn.length + 1, 8)) continue;
            safeFirst |= 1u << d;
        }
        int d = search.unexploredPortalDir(safeFirst);
        if (d >= 0) return Action::move(d);
#endif
#endif
        return {};
    }
    int routeDirection(DecisionContext const &context) {
        // ---- 3) route: soon-ripe pearls, then unexplored cells
        int planDir = -1;
        {
            prepareRouteCrowd();
            bool any = context.situation.immediatePearlOpportunity;
            isGoal.assign(config.cells, 0);
            for (int c : turn.visPearls)
                isGoal[c] = 1;
            for (int c = 0; c < config.cells; ++c) {
                int due = world.pearlDue[c];
                if (due >= 0 && due < INF && due - turn.round <= 8) {
                    isGoal[c] = 1;
                    any = true;
                }
            }
            int depth = 24, nodes = 320;
            auto tileScore = [this](int c, int step) { return scoutTileScore(c, step); };
            if (any) planDir = search.bfsFirstDir([&](int q) { return isGoal[q] != 0; }, tileScore, depth, nodes);
            if (planDir < 0)
                planDir = search.bfsFirstDir([&](int q) { return world.seenRound[q] == 0; }, tileScore, depth, nodes);
        }
        return planDir;
    }
    Action scoreAndFallback(Controller const &ct, DecisionContext const &context, int ramDir, int planDir, bool deathRoute
#ifdef DEBUG_PORTAL
                            ,
                            PortalDebug &debug
#endif
    ) {
        bool pearlInView = context.situation.immediatePearlOpportunity;

        // ---- 4) score safe moves
        int const flr = std::min(turn.length + 1, 8);
        int best = -1;
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
                if (best < 0 || ps > bestScore) {
                    bestScore = ps;
                    best = d;
                }
                continue;
            }
            if (occupancy.teammateHeadAt(dest) || occupancy.blockedAt(dest, 1)) continue;
            PocketInfo pocket = pocketBeyond(dest, turn.here);
            if (!pocketWorthEntering(pocket, d, ct)) continue;
            bool eats = occupancy.visible(dest) && occupancy.hasPearl(dest);
            int space = search.spaceFrom(dest, eats, 90, false);
            int controlled = turn.enemyHeads.empty() ? std::min(space, 80) : search.controlledSpace(dest, eats, 80);
            int opens = search.onwardSafe(dest, eats);
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
                        score -= 1000;
                    else if (md == 2)
                        score -= 150;
                }
            int shortEnemyDistance = shortEnemyRiskDistance(dest);
            if (shortEnemyDistance <= 1) score -= 1200;
            else if (shortEnemyDistance == 2) score -= 350;
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
                if (eats) score += 500;
            }
            if (d == planDir) score += deathRoute ? 250 : 30;
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

            if (best < 0 || score > bestScore) {
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
                if (!pocketWorthEntering(pocketBeyond(q, turn.here), d, ct)) continue;
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
