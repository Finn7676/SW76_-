#pragma once

#include "core.hpp"
#include <map>
#include <set>

namespace bot {

struct Sighting {
    int c, round, len;
};
struct DeathClue {
    int c, round;
    bool friendly = false;
};
struct AllyRally {
    int c, round, length; // length is a conservative lower bound when saturated
    int lengthRound = -1;
};
struct PortalClaim {
    int owner = -1;
    int round = -1;
};
// Persistent facts learned from observations, sonar, and actual crossings only.
struct WorldModel {
    std::vector<uint8_t> edgeT;                 // NC*4
    std::vector<int> edgePid;                   // NC*4
    std::vector<int> learnedExit;               // NC*4, observed teleport landing cell
    std::vector<int> seenRound;                 // NC, 0 = never seen
    std::vector<int> pearlDue;                  // NC, round a pearl is (or will be) there; -1 unknown, INF never
    std::vector<int> pearlInfo;                 // NC, round the pearlDue value was observed
    std::map<int, std::vector<int>> portalEnds; // portal id -> canonical edge keys
    std::vector<int> portalKeys;                // every canonical portal key we know, for sonar relay
    int pendingPortalKey = -1;
    // A portal pair is explored only after a crossing or an explicit sonar report.
    std::set<int> exploredPortals;
    std::set<int> crossedPortals;               // this dragon crossed; it keeps relaying the full pair
    std::map<int, PortalClaim> portalClaims;     // short-lived intent, separate from topology reports
    int activeClaimPortal = -1;
    std::vector<int> pendingExplorationKeys;
    int pendingExplorationOwner = -1, pendingExplorationLanding = -1;
    int urgentPortalReport = -1;

    std::map<int, AllyRally> allyRallies;
    bool suppressRally = false; // no advertisement on a split, sprint, or death
    int nextRallyCell = -1, nextRallyLength = 0;
    std::vector<Sighting> sightings;
    int lastEnemyIntelRound = 0;             // local enemy part or teammate sonar report
    int firstObservedRound = -1;
    int lastQuietTeammateReportRound = -1000;
    std::vector<Part> lastObservedParts;
    int lastPartsRound = -1;
    std::vector<DeathClue> deathClues;
    int sonarCursor = 0;
    size_t portalRelayCursor = 0;
    size_t crossedRelayCursor = 0;
    explicit WorldModel(GameConfig const &config) {
        int n = config.cells;
        edgeT.assign(n * 4, E_UNKNOWN);
        edgePid.assign(n * 4, -1);
        learnedExit.assign(n * 4, -1);
        seenRound.assign(n, 0);
        pearlDue.assign(n, -1);
        pearlInfo.assign(n, -1000);
    }
};

} // namespace bot
