#pragma once

#include "core.hpp"
#include <map>
#include <set>

namespace bot {

struct Sighting {
    int c, round, len;
};
struct QueenSighting {
    int c = -1, round = -1, length = 0;
    bool exact = false;
};
struct DeathClue {
    int c, round;
    bool friendly = false;
};
struct AllyRally {
    int c, round, length; // length is a conservative lower bound when saturated
    int lengthRound = -1;
};
struct AllyLength {
    int round = -1, length = 0;
    bool exact = false; // exact at the reported round, not necessarily this turn
};
struct PortalClaim {
    int owner = -1;
    int round = -1;
};
struct PortalOccupancy {
    int round = -1;
    bool busy = false; // release reports are retained to reject older packets
};
struct PortalHold {
    std::array<int, 2> rounds{-1, -1}; // sender's direction half: N/E or S/W
    std::array<int, 2> lengths{0, 0}; // reported lower bound capped at 15
};
struct ObservedPortalPassage {
    int source = -1, dir = -1, enteredRound = -1, lastBodyRound = -1;
    bool emerged = false;
};
struct LocalPortalOccupancy {
    int landing = -1, enteredRound = -1;
};
struct PearlClaim {
    int c = -1, round = -1, arrivalRound = -1; // c=-1 cancels this sender's intent
};
struct AllyPlan {
    int c = -1, round = -1, nextDir = -1;
};
struct PortalEchoHint {
    int round = -1, allies = 0, enemies = 0, kelp = 0;
};
constexpr int COORDINATION_MAX_AGE = 2;
constexpr int PORTAL_HOLD_MAX_AGE = 1;
constexpr int PEARL_INTENT_HORIZON = 4;
constexpr int ALLY_LENGTH_MAX_AGE = 3;
// Persistent facts learned from observations, sonar, and actual crossings only.
struct WorldModel {
    std::vector<uint8_t> edgeT;                 // NC*4
    std::vector<int> edgePid;                   // NC*4
    std::vector<int> learnedExit;               // NC*4, observed teleport landing cell
    std::vector<int> seenRound;                 // NC, -1 = never seen
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
    std::vector<int> pendingExplorationSources;
    int pendingExplorationOwner = -1, pendingExplorationLanding = -1;
    int lastExploredPortalKey = -1, lastExploredPortalSource = -1;
    int lastExploredPortalLanding = -1, lastExploredPortalRound = -1;
    int lastExploredPortalOwner = -1;
    int urgentPortalReport = -1;

    // Dynamic leases never change permanent terrain or explored-portal facts.
    std::map<std::pair<int, int>, PortalOccupancy> portalOccupancy; // (portal, sender)
    std::map<int, LocalPortalOccupancy> ownPortalOccupancy;
    std::map<std::pair<int,int>, ObservedPortalPassage> observedPortalPassages;
    std::map<std::pair<int, int>, PortalHold> portalHolds; // (portal, sender), per side
    std::map<int, PearlClaim> pearlClaims; // one next pearl per sender
    std::map<int, AllyPlan> allyPlans;
    int ownPearlTarget = -1;
    std::map<int, PortalEchoHint> portalEchoHints;
    int lastSonarRound = -1, lastSonarBeams = 0;
    int lastEchoProbePortal = -1, lastEchoProbeDirection = -1;
    SonarEchoes lastSonarEchoes{}; // aggregate counts, never delivery acknowledgements

    std::map<int, AllyRally> allyRallies;
    std::map<int, AllyLength> allyLengths;
    bool suppressRally = false; // no advertisement without a known surviving endpoint
    int nextRallyCell = -1, nextRallyLength = 0;
    std::vector<Sighting> sightings;
    QueenSighting enemyQueen; // identity-specific intel; disappearance is not death
    int lastEnemyIntelRound = 0;             // local enemy part or teammate sonar report
    int firstObservedRound = -1;
    int lastQuietTeammateReportRound = -1000;
    std::vector<Part> lastObservedParts;
    int lastPartsRound = -1;
    std::vector<DeathClue> deathClues;
    int sonarCursor = 0;
    size_t portalRelayCursor = 0;
    size_t crossedRelayCursor = 0;
    int allyLengthEstimate(int id, int round, int visibleLength) const {
        auto report = allyLengths.find(id);
        if (report != allyLengths.end() && report->second.round <= round &&
            round - report->second.round <= ALLY_LENGTH_MAX_AGE)
            return std::max(visibleLength, report->second.length);
        auto rally = allyRallies.find(id);
        if (rally != allyRallies.end() && rally->second.lengthRound <= round &&
            round - rally->second.lengthRound <= V4_DONATION_REPORT_AGE)
            return std::max(visibleLength, rally->second.length);
        return visibleLength;
    }
    explicit WorldModel(GameConfig const &config) {
        int n = config.cells;
        edgeT.assign(n * 4, E_UNKNOWN);
        edgePid.assign(n * 4, -1);
        learnedExit.assign(n * 4, -1);
        seenRound.assign(n, -1);
        pearlDue.assign(n, -1);
        pearlInfo.assign(n, -1000);
    }
};

} // namespace bot
