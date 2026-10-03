#pragma once

#include "donation.hpp"
#include <cstdlib>

namespace bot {

// These are overlapping, per-turn labels. Donation uses relative size, not ID labels.
struct RoleAssignment {
    bool champion = false;
    bool sacrificeCandidate = false;
    bool hunter = false;
    bool core = false;
    bool scout = false;
    bool escort = false;
    bool yieldPearls = false;
    bool attrition = false;
    bool fragile = false;
    bool bigMap = false;

    bool generalist() const {
        return !champion && !sacrificeCandidate && !hunter && !core && !scout;
    }
};

class RolePolicy {
  public:
    static RoleAssignment assign(GameConfig const &config, TurnState const &turn, [[maybe_unused]] WorldModel const &world) {
        RoleAssignment roles;
        roles.attrition = turn.population >= 12;
        roles.bigMap = config.cells >= 600;
        roles.fragile = turn.population <= 2;

#if FEAT_CHAMPION
        // Preserve the existing late-game champion assignment exactly.
        roles.champion = turn.round >= CH_START &&
                         (turn.myId % CH_MOD == 0 || turn.length >= CH_LEN);
#endif

#if FEAT_SACRIFICE
        // Medium dragons, including ID-selected Champions, may feed a
        // strictly longer recipient. The actual largest dragon has no eligible
        // recipient and therefore remains protected without an ID reservation.
        // Escorts are the short ID slots that stay alive to spot pearls.
        roles.yieldPearls = EscortPolicy::yieldsPearls(config, turn, world);
        roles.escort = EscortPolicy::isEscort(turn);
        roles.sacrificeCandidate = DonationPolicy::at(config, turn).donor(turn) && !roles.escort;
#endif

        // This label names the existing large-map expansion limit. It adds no
        // other protection or behavior to long dragons.
        roles.core = roles.bigMap && turn.length > 22;

        bool shortHunter = turn.length <= 5 && turn.population > 6 && !roles.champion;
#if FEAT_V3_SCOUT
        // A short dragon is a Hunter only while it has an actual larger target.
        bool huntTarget = false;
        for (int lengthSeen : turn.enemyHeadLen)
            if (lengthSeen >= turn.length + 3) huntTarget = true;
#if FEAT_HUNT_SIGHTINGS
        for (auto const &s : world.sightings) {
            int dx = std::abs((turn.here % config.width) - (s.c % config.width));
            int dy = std::abs((turn.here / config.width) - (s.c / config.width));
            int distance = std::min(dx, config.width - dx) + std::min(dy, config.height - dy);
            if (s.len >= turn.length + 3 && turn.round - s.round <= 3 && distance <= 14)
                huntTarget = true;
        }
#endif
        roles.hunter = shortHunter && huntTarget;
#else
        roles.hunter = shortHunter;
#endif

#if FEAT_SCOUT_ROLE
        // Only a known, unreported portal can create an active Scout task.
#if FEAT_V3_SCOUT
        bool portalStillUnknown = false;
        for (int key : world.portalKeys) {
            if (key < 0 || key >= (int)world.edgeT.size()) continue;
            int pid = world.edgePid[key];
            if (world.edgeT[key] == E_PORTAL && pid >= 0 && !world.exploredPortals.count(pid)) {
                portalStillUnknown = true;
                break;
            }
        }
        int scoutEnd = config.cells >= 1200 ? 300 : 220;
        roles.scout = turn.round < scoutEnd && turn.population >= 6 &&
                      turn.length >= 2 && turn.length <= 8 && !roles.champion &&
                      !roles.core && !roles.hunter && turn.enemyHeads.empty() &&
                      portalStillUnknown;
#else
        // V2 assignment retained for individual ablations.
        int unseen = 0;
        for (int roundSeen : world.seenRound)
            if (roundSeen == 0) ++unseen;
        bool mapStillUnknown = unseen * 10 >= config.cells;
        bool portalStillUnknown = false;
        for (int key : world.portalKeys) {
            if (key < 0 || key >= (int)world.edgeT.size()) continue;
            int pid = world.edgePid[key];
            if (world.edgeT[key] == E_PORTAL && pid >= 0 && !world.exploredPortals.count(pid)) {
                portalStillUnknown = true;
                break;
            }
        }

        // ID modulo four provides a stable one-in-four candidate pool. Residue
        // 1 avoids the existing ID%8==0 Champion cohort. Length 6-8 avoids
        // taking the current length<=5 Hunter cohort away from its behavior.
        bool selectedById = turn.myId % 4 == 1;
        roles.scout = turn.round < SCOUT_END_ROUND && turn.population >= 8 &&
                      turn.length >= 6 && turn.length <= 8 && !roles.champion &&
                      !roles.fragile && !roles.hunter && turn.enemyHeads.empty() &&
                      selectedById && (mapStillUnknown || portalStillUnknown);
#endif
#endif
        return roles;
    }
};

} // namespace bot
