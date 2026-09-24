#pragma once

#include "search.hpp"
#include "body.hpp"
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

  public:
    Planner(GameConfig const &config, TurnState const &turn, WorldModel const &world, Topology const &topology,
            OccupancyModel const &occupancy, Search &search, BodyMemory const &body)
        : config(config), turn(turn), world(world), topology(topology), occupancy(occupancy), search(search),
          body(body) {}

    // The policy priority is explicit here. Each phase returns no action when inapplicable.
    Action choose(Controller const &ct
#ifdef DEBUG_PORTAL
                  ,
                  PortalDebug &debug
#endif
    ) {
        Modes modes = deriveModes();
        RamCandidate ram = adjacentRam(modes.champion);
        if (Action a = sacrifice()) return a;
#if FEAT_RAM_FIRST
        if (ram.good && turn.population >= 4) return Action::move(ram.dir);
#endif
        if (Action a = expansion(ct, modes)) return a;
        if (ram.good) return Action::move(ram.dir);
        if (Action a = sprintRam(modes)) return a;
        if (Action a = sprintPearls(modes)) return a;
        int planDir = routeDirection();
        return scoreAndFallback(ct, modes, ram.dir, planDir
#ifdef DEBUG_PORTAL
                                ,
                                debug
#endif
        );
    }

  private:
    struct Modes {
        bool attrition, bigMap, fragile, champion;
    };
    struct RamCandidate {
        int dir;
        bool good;
    };
    Modes deriveModes() const {
        bool attrition = turn.population >= 12;
        bool bigMap = config.width * config.height >= 600;
        // a head-on trade could wipe out the whole team
        bool fragile = turn.population <= 2;

        // champion: probably our longest dragon, protect it late in the game
        // champion: a few dragons stop splitting late and play safe, for the longest-dragon tiebreak
        bool champion = false;
#if FEAT_CHAMPION
        champion = turn.round >= CH_START && (turn.myId % CH_MOD == 0 || turn.length >= CH_LEN);
#endif

        return {attrition, bigMap, fragile, champion};
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
    Action sacrifice() const {
#if FEAT_SACRIFICE
        // ---- endgame feed: a short spare dragon dies against a clearly longer friend
        if (turn.round > 465 && turn.length <= 6 && turn.population > 4) {
            for (int d = 0; d < 4; ++d) {
                int q = topology.step(turn.here, d);
                if (q < 0 || !occupancy.occupied(q) || !occupancy.isMine(q) || occupancy.dragonId(q) == turn.myId)
                    continue;
                if (turn.countOf(occupancy.dragonId(q)) >= turn.length + 3) {
                    return Action::move(d);
                }
            }
        }
#endif

        return {};
    }
    Action expansion(Controller const &ct, Modes const &modes) const {
        // ---- 1) keep the swarm at the cap
        bool wantExpand = (turn.round < 470 && !modes.champion) || turn.population < 8;
        if (wantExpand && turn.population < config.targetPopulation && turn.length >= 4 &&
            (!modes.bigMap || turn.length <= 22) && ct.can_split(2) && body.splitSafe(2)) {
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
                if (m < 0 || occupancy.blockedAt(m, 1)) continue;
                for (int d2 = 0; d2 < 4; ++d2) {
                    int q = topology.step(m, d2);
                    if (q < 0 || q == turn.here || !occupancy.enemyHeadAt(q)) continue;
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
    Action sprintPearls([[maybe_unused]] Modes const &modes) {
#if FEAT_SPRINT_EAT
        // ---- 2c) grab two pearls in one turn when an enemy head is contesting them
        if (turn.length >= 3 && !turn.enemyHeads.empty()) {
            for (int d1 = 0; d1 < 4; ++d1) {
                if (turn.length <= 3 && topology.edgeTypeAt(turn.here * 4 + d1) == E_PORTAL) continue;
                int m = topology.step(turn.here, d1);
                if (m < 0 || !occupancy.visible(m) || !occupancy.hasPearl(m) || occupancy.blockedAt(m, 1)) continue;
                for (int d2 = 0; d2 < 4; ++d2) {
                    if (turn.length <= 3 && topology.edgeTypeAt(m * 4 + d2) == E_PORTAL) continue;
                    int q = topology.step(m, d2);
                    if (q < 0 || q == turn.here || !occupancy.visible(q) || !occupancy.hasPearl(q) ||
                        occupancy.occupied(q))
                        continue;
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
    int routeDirection() {
        // ---- 3) route: soon-ripe pearls, then unexplored cells
        int planDir = -1;
        {
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
            if (any) planDir = search.bfsFirstDir([&](int q) { return isGoal[q] != 0; }, depth, nodes);
            if (planDir < 0) planDir = search.bfsFirstDir([&](int q) { return world.seenRound[q] == 0; }, depth, nodes);
        }
        return planDir;
    }
    Action scoreAndFallback(Controller const &ct, Modes const &modes, int ramDir, int planDir
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
            if (isPortal && turn.length == 2) continue;
            if (dest < 0) {
                if (!isPortal) continue;
                // unresolved portal: a mild exploration option
                long ps = 30;
                auto it = world.portalEnds.find(topology.portalIdAt(turn.here * 4 + d));
                if (it == world.portalEnds.end() || it->second.size() < 2) ps += 50;
#ifdef DEBUG_PORTAL
                debug.scores[d] = ps;
#endif
                if (best < 0 || ps > bestScore) {
                    bestScore = ps;
                    best = d;
                }
                continue;
            }
            if (occupancy.blockedAt(dest, 1)) continue;
            bool eats = occupancy.visible(dest) && occupancy.hasPearl(dest);
            int space = search.spaceFrom(dest, eats, 90, false);
            int opens = search.onwardSafe(dest, eats);
            int crowd = 0;
            for (int f : turn.friendParts)
                if (topology.cheb(dest, f) <= 3) ++crowd;

            long score = (long)search.controlledSpace(dest, eats, 80) * 11;

            if (modes.champion || modes.fragile)
                for (int eh : turn.enemyHeads) {
                    int md = topology.manh(dest, eh);
                    if (md <= 1)
                        score -= 1000;
                    else if (md == 2)
                        score -= 150;
                }
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
            if (turn.length <= 5 && turn.population > 6 && !modes.champion) {
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
            if (d == planDir) score += 30;
            if (isPortal) score += 25;
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
            if (turn.length >= 5 && ct.can_split(turn.length - 2) && body.tailCanEscape() &&
                body.splitSafe(turn.length - 2)) {
                return Action::split(turn.length - 2);
            }
#endif
            if (ct.can_split(2) && body.splitSafe(2)) {
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
                if (q < 0) continue;
                int s = occupancy.occupied(q) ? 0 : 1;
                if (s > bs) {
                    bs = s;
                    best = d;
                }
            }
            if (best < 0) best = dirIndex(ct.get_dir());
        }
        return Action::move(best);
    }
};

} // namespace bot
