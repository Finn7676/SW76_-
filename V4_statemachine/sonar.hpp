#pragma once

#include "topology.hpp"
#include "occupancy.hpp"
#include "donation.hpp"
#include <iterator>

namespace bot {

class Sonar {
    GameConfig const &config;
    TurnState const &turn;
    WorldModel &world;
    Topology &topology;
    OccupancyModel const &occupancy;

  public:
    Sonar(GameConfig const &config, TurnState const &turn, WorldModel &world, Topology &topology,
          OccupancyModel const &occupancy)
        : config(config), turn(turn), world(world), topology(topology), occupancy(occupancy) {}

    static constexpr uint32_t SALT = 0x5A17C3E1u;
    uint32_t tagOf(uint32_t low) { return ((low ^ SALT) * 0x9E3779B1u) >> 28; }
    uint32_t pack(int type, int c, int payload) {
        uint32_t low = (uint32_t)topology.cx(c) | ((uint32_t)topology.cy(c) << 6) |
                       ((uint32_t)(payload & 0x3FFF) << 12) | ((uint32_t)type << 26);
        return low | (tagOf(low) << 28);
    }
    enum { M_PEARL = 0, M_ENEMY = 1, M_PORTAL = 2, M_EXPLORED_PORTAL = 3 };

    // A team discriminator avoids accepting the opposing copy of this bot's reports.
    // This is protocol identification, not authentication of an arbitrary sender.
    uint32_t wireMarker() const { return 0x58515030u | (turn.myTeam == Team::B ? 1u : 0u); }
    uint64_t wireMessage(uint32_t message) const { return (uint64_t(wireMarker()) << 32) | message; }
    void send(Controller &ct) {
        if (!ct.sonar_protocol3) {
            // Protocol 2 engines reject directional SONAR and discard 64-bit input.
            // A complete portal pair cannot fit in their 32-bit message.
            if (uint32_t legacy = chooseLegacyMessage()) ct.send_sonar(legacy);
            return;
        }
        uint64_t ordinary = chooseMessage();
#if FEAT_V3_RELAY
        uint64_t crossed = chooseCrossedPortalMessage();
#else
        uint64_t crossed = 0;
#endif
#if FEAT_V3_PORTAL_CLAIM
        uint64_t claim = claimMessage(world.activeClaimPortal);
#else
        uint64_t claim = 0;
#endif
        // Rotate the reserved beams. A crossed pair goes out every turn; an
        // active explorer also reserves two beams for its short-lived claim.
        uint64_t rally = allyRallyMessage();
        // Short scouts spend every beam on a pearl call so a long dragon can hear it.
        uint64_t escortPearl = EscortPolicy::yieldsPearls(config, turn, world) ? escortPearlMessage() : 0;
        int base = (turn.round + turn.myId) & 3;
        for (int d = 0; d < 4; ++d) {
            uint64_t message = ordinary;
            if (crossed && (d == base || (!claim && d == ((base + 2) & 3)))) message = crossed;
            if (claim && (d == ((base + 1) & 3) || d == ((base + 3) & 3) ||
                          (!crossed && d == ((base + 2) & 3)))) message = claim;
            // Keep one crossed-pair beam; use its opposite beam to recruit
            // donors once quiet endgame consolidation has started.
            if (rally && !claim && d == ((base + 2) & 3)) message = rally;
            if (escortPearl) message = escortPearl;
            // A long dragon spends every beam on its summon so short dragons can hear it.
            else if (rally && turn.length > V4_ESCORT_MAX_LENGTH) message = rally;
            if (message) ct.send_sonar(Direction(DCH[d]), message);
        }
    }
    void receive(Controller const &ct) {
        std::set<uint64_t> received;
        for (auto raw64 : ct.get_sonar_messages()) {
            if (!received.insert(raw64).second) continue;
            if (((raw64 >> 40) & 255) == PAIR_MAGIC) {
                receivePortalPair(raw64);
                continue;
            }
            if (((raw64 >> 40) & 255) == CLAIM_MAGIC) {
                receiveClaim(raw64);
                continue;
            }
            if (((raw64 >> 40) & 255) == HISTORY_MAGIC) {
                receiveEnemyHistory(raw64);
                continue;
            }
            if (((raw64 >> 40) & 255) == RALLY_MAGIC) {
                receiveAllyRally(raw64);
                continue;
            }
            if (raw64 > UINT32_MAX && (raw64 >> 32) != wireMarker()) continue;
            uint32_t raw = (uint32_t)raw64, low = raw & 0x0FFFFFFFu;
            if ((raw >> 28) != tagOf(low)) continue;
            int x = low & 63, y = (low >> 6) & 63, payload = (low >> 12) & 0x3FFF, type = (low >> 26) & 3;
            if (x >= config.width || y >= config.height) continue;
            int c = topology.cid(x, y);
            if (type == M_PEARL) {
                int due = payload & 511;
                if (world.pearlInfo[c] < turn.round - 1) {
                    world.pearlDue[c] = due;
                    world.pearlInfo[c] = turn.round - 1;
                }
            } else if (type == M_ENEMY) {
                int r = payload & 511, l = payload >> 9;
                if (turn.round - r > 6 || r > turn.round) continue;
                world.sightings.push_back({c, r, l});
                world.lastEnemyIntelRound = turn.round;

            }
        }
        world.sightings.erase(std::remove_if(world.sightings.begin(), world.sightings.end(),
                                             [&](Sighting const &s) { return turn.round - s.round > 6; }),
                              world.sightings.end());
        if (world.sightings.size() > 24) world.sightings.erase(world.sightings.begin(), world.sightings.end() - 24);
        for (auto it = world.portalClaims.begin(); it != world.portalClaims.end();) {
            if (turn.round - it->second.round > 2) it = world.portalClaims.erase(it);
            else ++it;
        }
    }

    // Pair format: x1,y1,x2,y2 (6 bits each), id (13), orientation (1),
    // explored (1), team (1), magic/version (8), checksum (16). Total: 64.
    // Canonical edges are E or S; both ends have the same orientation.
    static constexpr uint64_t PAIR_MAGIC = 0xB7;
    static constexpr uint64_t CLAIM_MAGIC = 0xC3;
    static constexpr uint64_t HISTORY_MAGIC = 0xD1;
    static constexpr uint64_t RALLY_MAGIC = 0xE2;
    static uint16_t pairTag(uint64_t low) {
        uint32_t hash = 2166136261u ^ SALT;
        for (int i = 0; i < 6; ++i) {
            hash = (hash ^ uint8_t(low >> (8 * i))) * 16777619u;
        }
        return uint16_t(hash ^ (hash >> 16));
    }
    // History format: last enemy-intel round (9), report round (9), sender ID
    // (16), 18-round quiet eligibility (1), team (1), reserved (4), magic (8),
    // checksum (16). The sender's first observed turn gates eligibility, so a
    // newly split dragon cannot claim 18 quiet rounds merely because its
    // private memory has never contained an enemy sighting.
    uint64_t enemyHistoryMessage() const {
        if (turn.round < 0 || turn.round > 511 || turn.myId < 0 || turn.myId > 65535 ||
            world.lastEnemyIntelRound > turn.round) return 0;
        bool quiet = world.firstObservedRound >= 0 &&
                     turn.round - world.firstObservedRound >= V3_SACRIFICE_QUIET_ROUNDS &&
                     turn.round - world.lastEnemyIntelRound >= V3_SACRIFICE_QUIET_ROUNDS;
        uint64_t low = uint64_t(world.lastEnemyIntelRound) |
                       (uint64_t(turn.round) << 9) | (uint64_t(turn.myId) << 18) |
                       (uint64_t(quiet) << 34) |
                       (uint64_t(turn.myTeam == Team::B) << 35) | (HISTORY_MAGIC << 40);
        return low | (uint64_t(pairTag(low)) << 48);
    }
    void receiveEnemyHistory(uint64_t raw) {
        uint64_t low = raw & ((1ull << 48) - 1);
        if ((raw >> 48) != pairTag(low) || ((low >> 40) & 255) != HISTORY_MAGIC ||
            ((low >> 35) & 1) != uint64_t(turn.myTeam == Team::B) ||
            ((low >> 36) & 15)) return;
        int lastEnemyRound = low & 511, reportRound = (low >> 9) & 511;
        int senderId = (low >> 18) & 65535;
        bool quiet = (low >> 34) & 1;
        if (senderId == turn.myId || reportRound > turn.round || turn.round - reportRound > 1 ||
            lastEnemyRound > reportRound) return;
        world.lastEnemyIntelRound = std::max(world.lastEnemyIntelRound, lastEnemyRound);
        if (quiet && reportRound - lastEnemyRound >= V3_SACRIFICE_QUIET_ROUNDS)
            world.lastQuietTeammateReportRound = turn.round;
    }
    // x/y (6 each), sender ID (12), length lower bound (6), round (9),
    // team (1), magic (8), checksum (16). High IDs use local vision only;
    // never truncate an ID and accidentally recruit donors to another dragon.
    uint64_t escortPearlMessage() {
        int bestC = -1, bestRank = INF;
        auto consider = [&](int c, bool visiblePearl) {
            int due = world.pearlDue[c];
            int info = world.pearlInfo[c];
            if (due < 0 || due >= INF || due > 511 || info < turn.round - V4_PEARL_REPORT_AGE) return;
            if (occupancy.visible(c) && !occupancy.hasPearl(c) && due <= turn.round) return;
            int rank = (visiblePearl ? 0 : 100000) + (turn.round - info) * 1024 + due;
            if (rank < bestRank) {
                bestRank = rank;
                bestC = c;
            }
        };
        for (int i = 0; i < turn.visibleCount; ++i) {
            int c = turn.visibleCells[i];
            if (occupancy.hasPearl(c)) consider(c, true);
        }
        if (bestC < 0)
            for (int c = 0; c < config.cells; ++c) consider(c, false);
        return bestC < 0 ? 0 : wireMessage(pack(M_PEARL, bestC, world.pearlDue[bestC]));
    }
    void sendHeadPearl(Controller &ct) {
        if (turn.here < 0 || turn.here >= config.cells) return;
        world.pearlDue[turn.here] = turn.round;
        world.pearlInfo[turn.here] = turn.round;
        uint32_t packed = pack(M_PEARL, turn.here, turn.round & 511);
        if (!ct.sonar_protocol3) {
            ct.send_sonar(packed);
            return;
        }
        uint64_t message = wireMessage(packed);
        for (int d = 0; d < 4; ++d) ct.send_sonar(Direction(DCH[d]), message);
    }
    uint64_t allyRallyMessage() const {
        // Only a dragon that has grown past the short cap calls others in.
        if (world.suppressRally || turn.round < V4_SUMMON_ROUND || turn.length <= V4_ESCORT_MAX_LENGTH ||
            turn.myId < 0 || turn.myId > 4095 || turn.round > 511 || topology.cx(turn.here) >= 64 ||
            topology.cy(turn.here) >= 64) return 0;
        uint64_t low = uint64_t(topology.cx(world.nextRallyCell >= 0 ? world.nextRallyCell : turn.here)) |
                       (uint64_t(topology.cy(world.nextRallyCell >= 0 ? world.nextRallyCell : turn.here)) << 6) |
                       (uint64_t(turn.myId) << 12) | (uint64_t(std::min(world.nextRallyCell >= 0 ? world.nextRallyLength : turn.length, 63)) << 24) |
                       (uint64_t(turn.round) << 30) | (uint64_t(turn.myTeam == Team::B) << 39) |
                       (RALLY_MAGIC << 40);
        return low | (uint64_t(pairTag(low)) << 48);
    }
    void receiveAllyRally(uint64_t raw) {
        uint64_t low = raw & ((1ull << 48) - 1);
        if (turn.round < V4_SUMMON_ROUND || (raw >> 48) != pairTag(low) ||
            ((low >> 40) & 255) != RALLY_MAGIC ||
            ((low >> 39) & 1) != uint64_t(turn.myTeam == Team::B)) return;
        int x = low & 63, y = (low >> 6) & 63, id = (low >> 12) & 4095;
        int length = (low >> 24) & 63, round = (low >> 30) & 511;
        if (x >= config.width || y >= config.height || id == turn.myId ||
            length <= V4_ESCORT_MAX_LENGTH || round > turn.round ||
            turn.round - round > V4_SUMMON_AGE) return;
        auto it = world.allyRallies.find(id);
        if (it == world.allyRallies.end() || it->second.round <= round)
            world.allyRallies[id] = {topology.cid(x, y), round, length, round, true};
    }
    uint64_t portalMessage(int key) {
        int pid = topology.portalIdAt(key);
        auto it = world.portalEnds.find(pid);
        if (pid < 0 || pid >= 8192 || it == world.portalEnds.end() || it->second.size() != 2) return 0;
        int a = std::min(it->second[0], it->second[1]);
        int b = std::max(it->second[0], it->second[1]);
        int d = a % 4;
        if (a == b || (d != 1 && d != 2) || b % 4 != d) return 0;
        if (topology.edgeTypeAt(a) != E_PORTAL || topology.edgeTypeAt(b) != E_PORTAL ||
            topology.portalIdAt(a) != pid || topology.portalIdAt(b) != pid) return 0;
        int x1 = topology.cx(a / 4), y1 = topology.cy(a / 4);
        int x2 = topology.cx(b / 4), y2 = topology.cy(b / 4);
        if (x1 >= 64 || y1 >= 64 || x2 >= 64 || y2 >= 64) return 0;
        uint64_t low = uint64_t(x1) | (uint64_t(y1) << 6) | (uint64_t(x2) << 12) |
                       (uint64_t(y2) << 18) | (uint64_t(pid) << 24) | (uint64_t(d == 2) << 37) |
                       (uint64_t(FEAT_PORTAL_EXPLORE && world.exploredPortals.count(pid)) << 38) |
                       (uint64_t(turn.myTeam == Team::B) << 39) | (PAIR_MAGIC << 40);
        return low | (uint64_t(pairTag(low)) << 48);
    }
    // A claim is an intent keyed by portal ID, not a topology report. The
    // complete endpoint pair is always sent separately after it is known.
    uint64_t claimMessage(int pid) const {
        if (pid < 0 || pid >= 8192 || world.exploredPortals.count(pid) ||
            turn.myId < 0 || turn.myId > 65535) return 0;
        uint64_t low = uint64_t(pid) | (uint64_t(turn.myId) << 13) |
                       (uint64_t(turn.round & 511) << 29) |
                       (uint64_t(turn.myTeam == Team::B) << 38) |
                       (CLAIM_MAGIC << 40);
        return low | (uint64_t(pairTag(low)) << 48);
    }
    void receiveClaim(uint64_t raw) {
        uint64_t low = raw & ((1ull << 48) - 1);
        if ((raw >> 48) != pairTag(low) || ((low >> 40) & 255) != CLAIM_MAGIC ||
            ((low >> 38) & 1) != uint64_t(turn.myTeam == Team::B) || ((low >> 39) & 1)) return;
        int pid = low & 8191, owner = (low >> 13) & 65535, reportRound = (low >> 29) & 511;
        if (owner == turn.myId || reportRound > turn.round || turn.round - reportRound > 2 ||
            world.exploredPortals.count(pid)) return;
        auto it = world.portalClaims.find(pid);
        bool current = it != world.portalClaims.end() && turn.round >= it->second.round &&
                       turn.round - it->second.round <= 2;
        // Preserve the lowest live claimant even if a higher-ID dragon sends
        // a newer message. Otherwise repeated claims can steal the reservation.
        if (!current || owner < it->second.owner ||
            (owner == it->second.owner && reportRound > it->second.round))
            world.portalClaims[pid] = {owner, reportRound};
    }
    uint64_t chooseCrossedPortalMessage() {
        if (world.crossedPortals.empty()) return 0;
        size_t start = world.crossedRelayCursor++ % world.crossedPortals.size();
        auto it = world.crossedPortals.begin();
        std::advance(it, start);
        for (size_t n = 0; n < world.crossedPortals.size(); ++n) {
            if (it == world.crossedPortals.end()) it = world.crossedPortals.begin();
            auto ends = world.portalEnds.find(*it++);
            if (ends != world.portalEnds.end() && !ends->second.empty())
                if (uint64_t message = portalMessage(ends->second.front())) return message;
        }
        return 0;
    }
    void receivePortalPair(uint64_t raw) {
        uint64_t low = raw & ((1ull << 48) - 1);
        if ((raw >> 48) != pairTag(low) || ((low >> 40) & 255) != PAIR_MAGIC ||
            ((low >> 39) & 1) != uint64_t(turn.myTeam == Team::B)) return;
        int x1 = low & 63, y1 = (low >> 6) & 63, x2 = (low >> 12) & 63, y2 = (low >> 18) & 63;
        if (x1 >= config.width || x2 >= config.width || y1 >= config.height || y2 >= config.height) return;
        int pid = (low >> 24) & 8191, d = ((low >> 37) & 1) ? 2 : 1;
        int a = topology.cid(x1, y1) * 4 + d, b = topology.cid(x2, y2) * 4 + d;
        if (a >= b) return;
        // Validate both ends before changing either. Never replace conflicting terrain.
        for (int key : {a, b}) {
            int type = topology.edgeTypeAt(key);
            if (type != E_UNKNOWN && (type != E_PORTAL || topology.portalIdAt(key) != pid)) return;
        }
        auto it = world.portalEnds.find(pid);
        if (it != world.portalEnds.end())
            for (int key : it->second)
                if (key != a && key != b) return;
        topology.setEdge(a / 4, d, E_PORTAL, pid);
        topology.setEdge(b / 4, d, E_PORTAL, pid);
        if (FEAT_PORTAL_EXPLORE && ((low >> 38) & 1)) world.exploredPortals.insert(pid);
    }
    uint32_t chooseLegacyMessage() {
        if (EscortPolicy::yieldsPearls(config, turn, world))
            if (uint64_t wide = escortPearlMessage()) return (uint32_t)wide;
        if (!turn.enemyHeads.empty()) {
            int best = 0;
            for (size_t i = 1; i < turn.enemyHeads.size(); ++i)
                if (turn.enemyHeadLen[i] > turn.enemyHeadLen[best]) best = (int)i;
            return pack(M_ENEMY, turn.enemyHeads[best],
                        (turn.round & 511) | (std::min(turn.enemyHeadLen[best], 31) << 9));
        }
        ++world.sonarCursor;
        int bestC = -1, bestDue = INF;
        for (int i = 0; i < turn.visibleCount; ++i) {
            int c = turn.visibleCells[i], due = world.pearlDue[c];
            if (due < 0 || due >= INF || due > 511) continue;
            if (due < turn.round && !occupancy.hasPearl(c)) continue;
            int key = due * 64 + ((c * 37 + world.sonarCursor * 11) & 63);
            if (key < bestDue) { bestDue = key; bestC = c; }
        }
        return bestC < 0 ? 0 : pack(M_PEARL, bestC, world.pearlDue[bestC]);
    }
    uint64_t chooseMessage() {
#if FEAT_PORTAL_EXPLORE
        if (world.urgentPortalReport >= 0) {
            int key = world.urgentPortalReport;
            world.urgentPortalReport = -1;
            if (uint64_t msg = portalMessage(key)) return msg;
        }
        // Reserve a periodic relay even during combat so new dragons can catch up.
        if (!world.portalKeys.empty() && ((turn.round + turn.myId) & 7) == 0) {
            int key = world.portalKeys[world.portalRelayCursor++ % world.portalKeys.size()];
            if (uint64_t msg = portalMessage(key)) return msg;
        }
#endif
        if (!turn.enemyHeads.empty()) {
            int best = 0;
            for (size_t i = 1; i < turn.enemyHeads.size(); ++i)
                if (turn.enemyHeadLen[i] > turn.enemyHeadLen[best]) best = (int)i;
            return wireMessage(pack(M_ENEMY, turn.enemyHeads[best],
                        (turn.round & 511) | (std::min(turn.enemyHeadLen[best], 31) << 9)));
        }
        // A periodic status report shares the ordinary beam with pearl and
        // portal reports; crossed pairs and active claims keep their beams.
        if (((turn.round + turn.myId) & 3) == 0)
            if (uint64_t msg = enemyHistoryMessage()) return msg;
        ++world.sonarCursor;
        if (!world.portalKeys.empty() && (world.sonarCursor & 1)) {
            int key = world.portalKeys[(world.sonarCursor / 2) % world.portalKeys.size()];
            if (uint64_t msg = portalMessage(key)) return msg;
        }
        // a pearl tile in view that will be ripe soon
        int bestC = -1, bestDue = INF;
        for (int i = 0; i < turn.visibleCount; ++i) {
            int c = turn.visibleCells[i];
            int due = world.pearlDue[c];
            if (due < 0 || due >= INF || due > 511) continue;
            if (due < turn.round && !occupancy.hasPearl(c)) continue;
            int key = due * 64 + ((c * 37 + world.sonarCursor * 11) & 63);
            if (key < bestDue) {
                bestDue = key;
                bestC = c;
            }
        }
        if (bestC >= 0) return wireMessage(pack(M_PEARL, bestC, world.pearlDue[bestC]));
        return enemyHistoryMessage();
    }
};

} // namespace bot
