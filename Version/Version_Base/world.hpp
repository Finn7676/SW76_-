#pragma once

#include "core.hpp"
#include <map>

namespace bot {

struct Sighting {
    int c, round, len;
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

    std::vector<Sighting> sightings;
    int sonarCursor = 0;
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
