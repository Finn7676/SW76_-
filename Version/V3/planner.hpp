#pragma once

#include "search.hpp"
#include "body.hpp"
#include "roles.hpp"
#include "debug.hpp"

namespace bot {

class Planner {
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
    int claimPortalId = -1;

  public:
    Planner(GameConfig const &config, TurnState const &turn, WorldModel const &world, Topology const &topology,
            OccupancyModel const &occupancy, Search &search, BodyMemory const &body)
        : config(config), turn(turn), world(world), topology(topology), occupancy(occupancy), search(search),
          body(body) {}
    int activeClaimPortal() const { return claimPortalId; }
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
        Modes modes = RolePolicy::assign(config, turn, world);
        PortalRoute scoutRoute;
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
        unsigned scoutSafeFirst = 0;
        if (modes.scout && !world.portalKeys.empty()) {
            prepareRouteCrowd();
            scoutSafeFirst = safeScoutFirstSteps(ct);
            scoutRoute = search.scoutPortalRoute(scoutSafeFirst,
                                                  [this](int c, int step) { return scoutTileScore(c, step); });
        }
#endif
        RamCandidate ram = adjacentRam(modes.champion);
        if (Action a = sacrifice(modes)) return a;
#if FEAT_RAM_FIRST
        if (ram.good && turn.population >= 4) return Action::move(ram.dir);
#endif
        if (Action a = tacticalSplit(ct)) return a;
        if (Action a = expansion(ct, modes, scoutRoute.dir >= 0)) return a;
        if (ram.good) return Action::move(ram.dir);
        if (Action a = sprintRam(modes)) return a;
        if (Action a = sprintPearls(ct, modes)) return a;
        int deathDir = deathRouteDirection(ct);
#if FEAT_V3_PEARL_ECONOMY && FEAT_V3_SCOUT && FEAT_SCOUT_ROLE && FEAT_PORTAL_EXPLORE
        if (deathDir < 0 && scoutRoute.dir >= 0 && !turn.visPearls.empty()) {
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
            if (Action a = explorePortal(modes, scoutRoute)) return a;
        int planDir = deathDir >= 0 ? deathDir : routeDirection();
        return scoreAndFallback(ct, modes, ram.dir, planDir, deathDir >= 0
#ifdef DEBUG_PORTAL
                                ,
                                debug
#endif
        );
    }

  private:
    using Modes = RoleAssignment;
    struct RamCandidate {
        int dir;
        bool good;
    };
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
    Action sacrifice(Modes const &modes) const {
#if FEAT_SACRIFICE
        // ---- endgame feed: a short spare dragon dies against a clearly longer friend
        if (modes.sacrificeCandidate) {
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(turn.here, d);
            if (q < 0 || !occupancy.occupied(q) || !occupancy.isMine(q) || occupancy.dragonId(q) == turn.myId ||
                occupancy.teammateHeadAt(q))
                    continue;
                if (turn.countOf(occupancy.dragonId(q)) >= turn.length + 3) {
                    return Action::move(d);
                }
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
    Action expansion(Controller const &ct, Modes const &modes, bool activeScoutTask) const {
        // ---- 1) keep the swarm at the cap
        bool wantExpand = (turn.round < 470 && !modes.champion) || turn.population < 8;
        if (!activeScoutTask && wantExpand && turn.population < config.targetPopulation && turn.length >= 4 &&
            !modes.core && ct.can_split(2)) {
            return Action::split(2);
        }

        return {};
    }
    Action sprintRam([[maybe_unused]] Modes const &modes) const {
#if FEAT_SPRINT_RAM
        // ---- 2b) sprint two tiles into a valuable enemy head
        if (turn.length >= 3 && !modes.champion && turn.population >= 4) {
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
            if (modes.attrition && bestSz >= turn.length + 1 && bestSz >= 4) worth = true;
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
    Action sprintPearls([[maybe_unused]] Controller const &ct, [[maybe_unused]] Modes const &modes) {
#if FEAT_SPRINT_EAT
        // ---- 2c) grab two pearls in one turn when an enemy head is contesting them
        if (turn.length >= 3 && !turn.enemyHeads.empty()) {
            for (int d1 = 0; d1 < 4; ++d1) {
                int m = topology.step(turn.here, d1);
                if (m < 0 || !occupancy.visible(m) || !occupancy.hasPearl(m) || occupancy.blockedAt(m, 1) ||
                    occupancy.teammateHeadAt(m)) continue;
                if (!pocketWorthEntering(search.pocketBeyond(m, turn.here), d1, ct)) continue;
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
                    if (nearHead && !modes.attrition) continue;
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
            PocketInfo pocket = search.pocketBeyond(q, turn.here);
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
    Action explorePortal([[maybe_unused]] Modes const &modes, [[maybe_unused]] PortalRoute const &scoutRoute) {
#if FEAT_PORTAL_EXPLORE
#if FEAT_V3_SCOUT && FEAT_SCOUT_ROLE
        if (scoutRoute.dir >= 0) {
            claimPortalId = scoutRoute.pid;
            return Action::move(scoutRoute.dir);
        }
#else
        // Scout assignment limits intentional exploration to a stable, eligible subset.
#if FEAT_SCOUT_ROLE
        if (!modes.scout || world.portalKeys.empty()) return {};
#else
        // Ablation switch: retain the prior global exploration behavior.
        if (modes.champion || !turn.enemyHeads.empty() || world.portalKeys.empty()) return {};
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
    int routeDirection() {
        // ---- 3) route: soon-ripe pearls, then unexplored cells
        int planDir = -1;
        {
            prepareRouteCrowd();
            bool any = !turn.visPearls.empty();
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
    Action scoreAndFallback(Controller const &ct, Modes const &modes, int ramDir, int planDir, bool deathRoute
#ifdef DEBUG_PORTAL
                            ,
                            PortalDebug &debug
#endif
    ) {
        bool pearlInView = !turn.visPearls.empty();

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
            PocketInfo pocket = search.pocketBeyond(dest, turn.here);
            if (!pocketWorthEntering(pocket, d, ct)) continue;
            bool eats = occupancy.visible(dest) && occupancy.hasPearl(dest);
            int space = search.spaceFrom(dest, eats, 90, false);
            int opens = search.onwardSafe(dest, eats);
            int crowd = 0;
            for (int f : turn.friendParts)
                if (topology.cheb(dest, f) <= 3) ++crowd;

            long score = (long)search.controlledSpace(dest, eats, 80) * 11;
            if (pocket.proven && pocket.pearls >= 3)
                score += long(std::min(pocket.pearls, 4)) * 160;

            if (modes.champion || modes.fragile)
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
            for (size_t i = 0; i < turn.enemyHeads.size() && !modes.champion && !modes.fragile; ++i) {
                int eh = turn.enemyHeads[i];
                if (topology.cheb(dest, eh) > 1) continue;
                int esz = turn.enemyHeadLen[i];
                if (esz > turn.length)
                    score += std::min((esz - turn.length) * 30L, 260L);
                else if (turn.length > esz)
                    score -= std::min((turn.length - esz) * (modes.attrition ? 5L : 12L), modes.attrition ? 40L : 120L);
                else
                    score += modes.attrition ? 25 : -15;
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
            if (modes.hunter) {
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
            if (modes.champion)
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
                if (!pocketWorthEntering(search.pocketBeyond(q, turn.here), d, ct)) continue;
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
