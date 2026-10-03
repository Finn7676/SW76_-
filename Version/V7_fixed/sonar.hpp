#pragma once

#include "topology.hpp"
#include "occupancy.hpp"
#include "donation.hpp"
#include "body.hpp"
#include "coordination.hpp"
#include <iterator>

namespace bot {

class Sonar {
    GameConfig const &config;
    TurnState const &turn;
    WorldModel &world;
    Topology &topology;
    OccupancyModel const &occupancy;
    int postHead = -1, postLength = 0, postFacing = -1, nextDir = -1;
    int preparedRound = -1;
    bool postAlive = false, postLengthExact = true;
    int pearlTarget = -1, pearlEta = 0, cancelPearlUntil = -1, enteringPortal = -1;
    std::vector<int> postBody, transferredBody, eaten;
    std::vector<std::pair<int, int>> consumedReports, releasedPortals; // cell/portal, event round
    size_t busyCursor = 0, consumedCursor = 0;

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
        world.lastSonarRound = turn.round;
        world.lastSonarBeams = 0;
        world.lastEchoProbePortal = world.lastEchoProbeDirection = -1;
        if (!ct.sonar_protocol3) {
            // Both output syntaxes are accepted, but legacy input carries no
            // 64-bit coordination packets or echo counts until negotiation.
            if (uint32_t legacy = chooseLegacyMessage()) {
                ct.send_sonar(legacy);
                world.lastSonarBeams = 1;
            }
            return;
        }
        auto emit = [&](int d, uint64_t message) {
            if (!message) return;
            ct.send_sonar(Direction(DCH[d]), message);
            ++world.lastSonarBeams;
        };
        std::array<uint64_t, 4> holds{};
        bool hasHold = false;
        for (int d = 0; d < 4; ++d) { holds[d] = portalHoldMessage(d); hasHold |= holds[d] != 0; }
        uint64_t busy = portalOccupancyMessage();
        uint64_t length = allyLengthMessage();
        // Messages are cast from the surviving post-action head. A hold must
        // travel through its own portal edge, irrespective of the normal ray rotation.
        auto emitBatch = [&](std::array<uint64_t, 4> messages) {
            int lengthDir = (turn.round + turn.myId) & 3;
            if (length) {
                for (int d = 0; d < 4; ++d) if (messages[d] == length) lengthDir = d;
                int oldLengthDir = lengthDir;
                for (int n = 0; n < 4; ++n) {
                    int d = (oldLengthDir + n) & 3;
                    if (!holds[d]) { lengthDir = d; break; }
                }
                if (lengthDir != oldLengthDir && messages[oldLengthDir] == length)
                    messages[oldLengthDir] = messages[lengthDir];
            }
            for (int d = 0; d < 4; ++d) if (holds[d]) messages[d] = holds[d];
            // Four portal edges: rotate the omitted hold so length still gets one ray.
            if (length) messages[lengthDir] = length;
            for (int d = 0; d < 4; ++d) emit(d, messages[d]);
        };
        // Crossing reports share the four-ray budget with directed holds and length.
        if (enteringPortal >= 0 && busy) {
            std::array<uint64_t, 4> messages{busy, busy, busy, busy};
            emitBatch(messages);
            return;
        }
        uint64_t pearl = pearlIntentMessage();
        uint64_t consumed = consumedPearlMessage();
        uint64_t intent = rallyIntentMessage();
        uint64_t rally = allyRallyMessage();
#if FEAT_V3_PORTAL_CLAIM
        uint64_t claim = claimMessage(world.activeClaimPortal);
#else
        uint64_t claim = 0;
#endif
        // A probe uses exactly ONE ray. Four-ray aggregate echoes cannot be
        // assigned to directions, and an ally hit is not a delivery ACK.
        if (!hasHold && !busy && !pearl && !consumed && !rally && turn.enemyHeads.empty() &&
            ((turn.round + turn.myId) & 15) == 0 && postHead >= 0) {
            for (int d = 0; d < 4; ++d) {
                int key = postHead * 4 + d;
                if (topology.edgeTypeAt(key) != E_PORTAL || topology.portalExit(postHead, d) >= 0) continue;
                uint64_t message = length ? length : (claim ? claim : chooseMessage());
                if (!message) break;
                emit(d, message);
                world.lastEchoProbePortal = topology.portalIdAt(key);
                world.lastEchoProbeDirection = d;
                return;
            }
        }
#if FEAT_V3_RELAY
        uint64_t crossed = chooseCrossedPortalMessage();
#else
        uint64_t crossed = 0;
#endif
        bool urgentPair = world.urgentPortalReport >= 0;
        uint64_t ordinary = chooseMessage();
        int base = (turn.round + turn.myId) & 3;
        std::array<uint64_t, 4> messages{};
        messages[0] = busy ? busy : (urgentPair && ordinary ? ordinary :
                        (claim ? claim : (crossed ? crossed : ordinary)));
        messages[1] = consumed && (!pearl || ((turn.round + turn.myId) & 1)) ? consumed : pearl;
        messages[2] = queenSightingMessage();
        if (!messages[2]) messages[2] = intent;
        messages[3] = length ? length : ordinary;
        if (!messages[1]) messages[1] = consumed ? consumed : (busy ? busy : (crossed ? crossed : ordinary));
        if (busy && (urgentPair || (turn.round + turn.myId) % 3 == 0)) {
            // Renew occupancy while still relaying terrain during long leases.
            uint64_t relay = urgentPair && ordinary ? ordinary : (crossed ? crossed : ordinary);
            if (relay) messages[2] = relay;
        }
        std::array<uint64_t, 4> directed{};
        for (int i = 0; i < 4; ++i) directed[(base + i) & 3] = messages[i];
        emitBatch(directed);
    }
    void receive(Controller const &ct) {
        world.lastSonarEchoes = ct.get_sonar_echoes();
        if (ct.sonar_protocol3 && world.lastSonarRound == turn.round - 1 &&
            world.lastSonarBeams == 1 && world.lastEchoProbePortal >= 0) {
            auto const &echo = world.lastSonarEchoes;
            world.portalEchoHints[world.lastEchoProbePortal] =
                {turn.round, echo.ally + echo.ally_head, echo.enemy + echo.enemy_head, echo.kelp};
        }
        std::set<uint64_t> received;
        for (auto raw64 : ct.get_sonar_messages()) {
            if (!received.insert(raw64).second) continue;
            uint64_t kind = (raw64 >> 40) & 254;
            if (kind == BUSY_MAGIC) { receivePortalOccupancy(raw64); continue; }
            if (kind == PORTAL_HOLD_MAGIC) { receivePortalHold(raw64); continue; }
            if (kind == PEARL_INTENT_MAGIC) { receivePearlIntent(raw64); continue; }
            if (kind == PEARL_STATUS_MAGIC) { receivePearlStatus(raw64); continue; }
            if (kind == RALLY_INTENT_MAGIC) { receiveRallyIntent(raw64); continue; }
            if (kind == ALLY_LENGTH_MAGIC) { receiveAllyLength(raw64); continue; }
            if (kind == QUEEN_SIGHTING_MAGIC) { receiveQueenSighting(raw64); continue; }
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
                if (payload >> 9) continue;
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
    // New packet headers encode team in their low bit, leaving all 40 payload
    // bits available. IDs are full 16-bit values, never silently truncated.
    static constexpr uint64_t BUSY_MAGIC = 0xF0;
    static constexpr uint64_t PEARL_INTENT_MAGIC = 0xF2;
    static constexpr uint64_t PEARL_STATUS_MAGIC = 0xF4;
    static constexpr uint64_t RALLY_INTENT_MAGIC = 0xF6;
    static constexpr uint64_t ALLY_LENGTH_MAGIC = 0xF8;
    static constexpr uint64_t QUEEN_SIGHTING_MAGIC = 0xA0;
    static constexpr uint64_t PORTAL_HOLD_MAGIC = 0xFC;
    static uint16_t pairTag(uint64_t low) {
        uint32_t hash = 2166136261u ^ SALT;
        for (int i = 0; i < 6; ++i) {
            hash = (hash ^ uint8_t(low >> (8 * i))) * 16777619u;
        }
        return uint16_t(hash ^ (hash >> 16));
    }
    uint64_t coordinationMessage(uint64_t kind, uint64_t payload) const {
        uint64_t low = payload | ((kind | uint64_t(turn.myTeam == Team::B)) << 40);
        return low | (uint64_t(pairTag(low)) << 48);
    }
    bool validCoordination(uint64_t raw, uint64_t kind) const {
        uint64_t low = raw & ((1ull << 48) - 1);
        return (raw >> 48) == pairTag(low) &&
               ((low >> 40) & 255) == (kind | uint64_t(turn.myTeam == Team::B));
    }
    bool freshReport(int round, int age = COORDINATION_MAX_AGE) const {
        return round >= 0 && round <= turn.round && turn.round - round <= age;
    }
    bool validSender() const { return turn.myId >= 0 && turn.myId <= 65535 && turn.round >= 0 && turn.round <= 511; }
    // sender (16), round (9), length (14), exact/lower-bound (1).
    // Length is the surviving sender's length AFTER its final action.
    uint64_t allyLengthMessage() const {
        if (!validSender()) return 0;
        bool prepared = preparedRound == turn.round;
        if (prepared && !postAlive) return 0;
        int length = prepared ? postLength : turn.length;
        if (length < Constants::MIN_SIZE || length > 16383) return 0;
        bool exact = !prepared || postLengthExact;
        return coordinationMessage(ALLY_LENGTH_MAGIC, uint64_t(turn.myId) |
            (uint64_t(turn.round) << 16) | (uint64_t(length) << 25) | (uint64_t(exact) << 39));
    }
    void receiveAllyLength(uint64_t raw) {
        if (!validCoordination(raw, ALLY_LENGTH_MAGIC)) return;
        int id = raw & 65535, round = (raw >> 16) & 511, length = (raw >> 25) & 16383;
        bool exact = (raw >> 39) & 1;
        if (id == turn.myId || length < Constants::MIN_SIZE || !freshReport(round, ALLY_LENGTH_MAX_AGE)) return;
        auto &report = world.allyLengths[id];
        if (round > report.round || (round == report.round &&
            ((!report.exact && exact) || (!report.exact && !exact && length > report.length))))
            report = {round, length, exact};
    }
    // x/y (6 each), observation round (9), length (14), exact (1), reserved (4).
    // The opposing Queen is unique. Relays retain the original observation round.
    uint64_t queenSightingMessage() const {
        auto const &q = world.enemyQueen;
        if (!encodableCell(q.c) || !freshReport(q.round, 3) || q.length < 2 || q.length > 16383) return 0;
        return coordinationMessage(QUEEN_SIGHTING_MAGIC, uint64_t(topology.cx(q.c)) |
            (uint64_t(topology.cy(q.c)) << 6) | (uint64_t(q.round) << 12) |
            (uint64_t(q.length) << 21) | (uint64_t(q.exact) << 35));
    }
    void receiveQueenSighting(uint64_t raw) {
        if (!validCoordination(raw, QUEEN_SIGHTING_MAGIC) || ((raw >> 36) & 15)) return;
        int x = raw & 63, y = (raw >> 6) & 63, round = (raw >> 12) & 511;
        int length = (raw >> 21) & 16383; bool exact = (raw >> 35) & 1;
        if (x >= config.width || y >= config.height || length < 2 || !freshReport(round, 3)) return;
        auto &q = world.enemyQueen;
        if (round > q.round || (round == q.round && !q.exact && exact))
            q = {topology.cid(x,y), round, length, exact};
        world.lastEnemyIntelRound = std::max(world.lastEnemyIntelRound, round);
    }
    bool encodableCell(int c) const {
        return c >= 0 && c < config.cells && topology.cx(c) < 64 && topology.cy(c) < 64;
    }
    bool wasEaten(int c) const { return std::find(eaten.begin(), eaten.end(), c) != eaten.end(); }
    bool postOwnBodyAt(int c) const { return std::find(postBody.begin(), postBody.end(), c) != postBody.end(); }
    bool postBlocked(int c) const {
        if (postOwnBodyAt(c) || std::find(transferredBody.begin(), transferredBody.end(), c) != transferredBody.end()) return true;
        if (!occupancy.occupied(c)) return false;
        // Release only the own segments that the simulated action actually
        // removed. Unknown cells and other dragons keep conservative occupancy.
        return !occupancy.isMine(c) || occupancy.dragonId(c) != turn.myId;
    }

    // Prepare messages from the final action, before printing it. The engine
    // casts these packets only if that action survives. Unknown landings stay
    // unknown; no post-action visibility is invented.
    void prepare(Action const &action, Controller const &ct, BodyMemory const &body,
                 int preferredPearl = -1, bool collecting = false) {
        preparedRound = turn.round;
        postAlive = action.type == ActionType::MOVE || action.type == ActionType::SPRINT || action.type == ActionType::SPLIT;
        postLengthExact = true;
        postHead = turn.here;
        postLength = turn.length;
        postFacing = dirIndex(ct.get_dir());
        enteringPortal = -1;
        eaten.clear();
        postBody.clear();
        transferredBody.clear();
        for (int i = 0; i < turn.length; ++i) postBody.push_back(body.cellAtTailOffset(i));
        if (!postBody.empty()) postBody.back() = turn.here;
        if (action.type == ActionType::SPLIT) {
            postLength -= action.splitSize;
            if (action.splitSize >= 0 && action.splitSize <= int(postBody.size())) {
                transferredBody.assign(postBody.begin(), postBody.begin() + action.splitSize);
                postBody.erase(postBody.begin(), postBody.begin() + action.splitSize);
            }
        } else if (action.type == ActionType::MOVE || action.type == ActionType::SPRINT) {
            // Pay the complete sprint cost even if a portal hides later steps.
            postLength -= MovementRules::cost(turn.length, action.steps);
            for (int i = 0; i < action.steps; ++i) {
                int d = action.dirs[i];
                if (postHead < 0) break;
                int key = postHead * 4 + d;
                if (topology.edgeTypeAt(key) == E_PORTAL) {
                    int pid = topology.portalIdAt(key);
                    if (enteringPortal < 0) enteringPortal = pid;
                    world.ownPortalOccupancy[pid] = {topology.step(postHead, d), turn.round};
                }
                postHead = topology.step(postHead, d);
                postFacing = d;
                if (postHead < 0) { postLengthExact = false; postBody.clear(); break; }
                if (!occupancy.visible(postHead)) postLengthExact = false;
                bool consumes = occupancy.visible(postHead) && occupancy.hasPearl(postHead) && !wasEaten(postHead);
                if (consumes) {
                    eaten.push_back(postHead);
                    consumedReports.push_back({postHead, turn.round});
                    ++postLength;
                } else if (occupancy.visible(postHead)) {
                    if (!postBody.empty()) postBody.erase(postBody.begin());
                }
                // On an unseen cell a pearl might retain the tail. Keep that
                // segment for prediction; advertised length remains a lower bound.
                postBody.push_back(postHead);
                if (MovementRules::paidStep(turn.length, i)) {
                    if (!postBody.empty()) postBody.erase(postBody.begin());
                }
            }
        } else {
            postHead = -1;
            postBody.clear();
        }
        // Any packet that is actually cast comes from a surviving dragon.
        if (!postLengthExact) postLength = std::max(Constants::MIN_SIZE, postLength);
        refreshOwnPortalOccupancy();
        choosePostActionPearl(preferredPearl, collecting);
        if (pearlTarget < 0 && world.ownPearlTarget >= 0) cancelPearlUntil = turn.round + 1;
        world.ownPearlTarget = pearlTarget;
        world.nextRallyCell = postHead;
        world.nextRallyLength = postLength;
        world.suppressRally = postHead < 0;
        if (nextDir < 0 && postHead >= 0 && action.type != ActionType::SPLIT) {
            // Continuation is an intention, never a promise. New vision may
            // change it next turn; receivers retain their local safety checks.
            int best = -1, bestScore = INT_MIN;
            for (int d = 0; d < 4; ++d) {
                uint8_t e = topology.edgeTypeAt(postHead * 4 + d);
                int q = topology.step(postHead, d);
                if ((e != E_OPEN && e != E_PORTAL) || q < 0 || postBlocked(q)) continue;
                int score = int(d == postFacing) * 2;
                for (int nd = 0; nd < 4; ++nd) {
                    int n = topology.step(q, nd);
                    if (n >= 0 && !postBlocked(n)) ++score;
                }
                if (score > bestScore) { best = d; bestScore = score; }
            }
            nextDir = best;
        }
    }
    void refreshOwnPortalOccupancy() {
        for (auto &[pid, lease] : world.ownPortalOccupancy)
            if (lease.landing < 0 && world.lastExploredPortalRound == turn.round &&
                world.lastExploredPortalOwner == turn.myId && world.lastExploredPortalKey >= 0 &&
                topology.portalIdAt(world.lastExploredPortalKey) == pid)
                lease.landing = world.lastExploredPortalLanding;
        std::map<int, int> occupiedPortals;
        auto noteChain = [&](std::vector<int> const &chain) {
            for (size_t i = 1; i < chain.size(); ++i) {
                int tailward = chain[i - 1], headward = chain[i];
                if (tailward < 0 || headward < 0) continue;
                for (int d = 0; d < 4; ++d)
                    if (topology.edgeTypeAt(tailward * 4 + d) == E_PORTAL &&
                        topology.step(tailward, d) == headward)
                        occupiedPortals[topology.portalIdAt(tailward * 4 + d)] = headward;
            }
        };
        noteChain(postBody);
        // The split child occupies its segments immediately, before its first
        // turn. Transferred segments must not produce a false release report.
        noteChain(transferredBody);
        for (auto const &[pid, landing] : occupiedPortals)
            world.ownPortalOccupancy.try_emplace(pid, LocalPortalOccupancy{landing, turn.round});
        for (auto it = world.ownPortalOccupancy.begin(); it != world.ownPortalOccupancy.end();) {
            auto const &lease = it->second;
            bool nearLanding = lease.landing >= 0 && postHead >= 0 && topology.cheb(postHead, lease.landing) <= 2;
            bool busy = it->first == enteringPortal || occupiedPortals.count(it->first) || nearLanding;
            // A lost/unknown landing gets only the immediate short lease, not
            // an indefinitely renewed claim of occupancy.
            if (busy) ++it;
            else {
                releasedPortals.push_back({it->first, turn.round});
                it = world.ownPortalOccupancy.erase(it);
            }
        }
        auto expire = [&](auto &reports) {
            reports.erase(std::remove_if(reports.begin(), reports.end(), [&](auto const &r) {
                return !freshReport(r.second, 1);
            }), reports.end());
        };
        expire(consumedReports);
        expire(releasedPortals);
    }
    void choosePostActionPearl(int preferred, bool collecting) {
        pearlTarget = -1; pearlEta = 0; nextDir = -1;
        if (postHead < 0 || (!collecting && preferred < 0)) return;
        if (wasEaten(preferred)) preferred = -1;
        Coordination coordination(turn, world, topology, occupancy);
        std::vector<int> distance(config.cells, -1), first(config.cells, -1), queue{postHead};
        distance[postHead] = 0;
        int fallback = -1;
        for (size_t i = 0; i < queue.size() && i < 96; ++i) {
            int c = queue[i], depth = distance[c];
            bool pearl = c != postHead && occupancy.visible(c) && occupancy.hasPearl(c) &&
                         !wasEaten(c) && coordination.pearlAvailable(c, depth + 1);
            if (pearl) {
                if (fallback < 0) fallback = c;
                if (preferred < 0 || c == preferred) { fallback = c; break; }
                continue; // a nearer pearl is the next meal on this path
            }
            if (depth >= PEARL_INTENT_HORIZON) continue;
            for (int d = 0; d < 4; ++d) {
                uint8_t edge = topology.edgeTypeAt(c * 4 + d);
                int q = topology.step(c, d);
                if ((edge != E_OPEN && edge != E_PORTAL) || q < 0 || distance[q] >= 0 || postBlocked(q)) continue;
                if (edge == E_PORTAL && (coordination.portalBusy(topology.portalIdAt(c * 4 + d)) ||
                    coordination.portalHeld(c, d))) continue;
                distance[q] = depth + 1;
                first[q] = c == postHead ? d : first[c];
                queue.push_back(q);
            }
        }
        if (fallback >= 0) {
            pearlTarget = fallback; pearlEta = distance[fallback]; nextDir = first[fallback];
        }
    }
    // portal (13), sender (16), round modulo 64 (6), direction half (1),
    // surviving length capped at 15 (4). The portal edge supplies its axis.
    // This single packet preserves length evidence on engines that replace
    // earlier SONAR commands in the same direction. Ordinary length rays still
    // carry the full 14-bit length separately.
    uint64_t portalHoldMessage(int d) const {
        if (!validSender() || preparedRound != turn.round || !postAlive || postHead < 0 || d < 0 || d >= 4 ||
            postLength < Constants::MIN_SIZE || topology.edgeTypeAt(postHead * 4 + d) != E_PORTAL) return 0;
        int pid = topology.portalIdAt(postHead * 4 + d);
        if (pid < 0 || pid >= 8192) return 0;
        return coordinationMessage(PORTAL_HOLD_MAGIC, uint64_t(pid) | (uint64_t(turn.myId) << 13) |
            (uint64_t(turn.round & 63) << 29) | (uint64_t(d >> 1) << 35) |
            (uint64_t(std::min(postLength, 15)) << 36));
    }
    void receivePortalHold(uint64_t raw) {
        if (!validCoordination(raw, PORTAL_HOLD_MAGIC)) return;
        int pid = raw & 8191, id = (raw >> 13) & 65535, wireRound = (raw >> 29) & 63;
        int side = (raw >> 35) & 1, length = (raw >> 36) & 15;
        if (id == turn.myId || length < Constants::MIN_SIZE || turn.round < 0) return;
        int age = (turn.round + 64 - wireRound) % 64, round = turn.round - age;
        // Holds are never relayed. Only this or the previous engine round is
        // meaningful; compressed timestamps are reconstructed against that window.
        if (!freshReport(round, PORTAL_HOLD_MAX_AGE)) return;
        auto &report = world.portalHolds[{pid, id}];
        if (round > report.rounds[side]) {
            report.rounds[side] = round; report.lengths[side] = length;
        }
    }
    uint64_t portalOccupancyMessage() {
        if (!validSender()) return 0;
        int pid = enteringPortal, reportRound = turn.round;
        bool busy = true;
        if (pid < 0 && !releasedPortals.empty()) {
            pid = releasedPortals.back().first;
            reportRound = releasedPortals.back().second;
            busy = false;
        } else if (pid < 0 && !world.ownPortalOccupancy.empty()) {
            auto it = world.ownPortalOccupancy.begin();
            std::advance(it, busyCursor++ % world.ownPortalOccupancy.size());
            pid = it->first;
        }
        if (pid < 0 || pid >= 8192) return 0;
        return coordinationMessage(BUSY_MAGIC, uint64_t(pid) | (uint64_t(turn.myId) << 13) |
            (uint64_t(reportRound) << 29) | (uint64_t(busy) << 38));
    }
    void receivePortalOccupancy(uint64_t raw) {
        if (!validCoordination(raw, BUSY_MAGIC) || ((raw >> 39) & 1)) return;
        int pid = raw & 8191, id = (raw >> 13) & 65535, round = (raw >> 29) & 511;
        if (id == turn.myId || !freshReport(round)) return;
        auto &report = world.portalOccupancy[{pid, id}];
        if (round > report.round || (round == report.round && !((raw >> 38) & 1)))
            report = {round, bool((raw >> 38) & 1)};
    }
    uint64_t pearlIntentMessage() const {
        if (!validSender()) return 0;
        if (pearlTarget < 0 && turn.round > cancelPearlUntil) return 0;
        int c = pearlTarget >= 0 ? pearlTarget : 0;
        if (!encodableCell(c) || pearlEta < 0 || pearlEta > PEARL_INTENT_HORIZON) return 0;
        return coordinationMessage(PEARL_INTENT_MAGIC, uint64_t(topology.cx(c)) |
            (uint64_t(topology.cy(c)) << 6) | (uint64_t(turn.myId) << 12) |
            (uint64_t(turn.round) << 28) | (uint64_t(pearlTarget >= 0 ? pearlEta : 0) << 37));
    }
    void receivePearlIntent(uint64_t raw) {
        if (!validCoordination(raw, PEARL_INTENT_MAGIC)) return;
        int x = raw & 63, y = (raw >> 6) & 63, id = (raw >> 12) & 65535;
        int round = (raw >> 28) & 511, eta = (raw >> 37) & 7;
        if (id == turn.myId || x >= config.width || y >= config.height ||
            eta > PEARL_INTENT_HORIZON || !freshReport(round)) return;
        auto &claim = world.pearlClaims[id];
        if (round > claim.round || (round == claim.round && eta == 0))
            claim = {eta ? topology.cid(x, y) : -1, round, round + eta};
    }
    uint64_t pearlStatusMessage(int c, int due, int observedRound, int state) const {
        if (!encodableCell(c) || observedRound < 0 || observedRound > 511 || state < 0 || state > 2) return 0;
        if (due < 0 || due >= INF || due > 510) due = 511; // unknown next respawn
        return coordinationMessage(PEARL_STATUS_MAGIC, uint64_t(topology.cx(c)) |
            (uint64_t(topology.cy(c)) << 6) | (uint64_t(due) << 12) |
            (uint64_t(observedRound) << 21) | (uint64_t(state) << 30));
    }
    uint64_t consumedPearlMessage() {
        if (consumedReports.empty()) return 0;
        auto const &report = consumedReports[consumedCursor++ % consumedReports.size()];
        return pearlStatusMessage(report.first, INF, report.second, 2);
    }
    void receivePearlStatus(uint64_t raw) {
        if (!validCoordination(raw, PEARL_STATUS_MAGIC) || ((raw >> 32) & 255)) return;
        int x = raw & 63, y = (raw >> 6) & 63, due = (raw >> 12) & 511;
        int observed = (raw >> 21) & 511, state = (raw >> 30) & 3;
        if (x >= config.width || y >= config.height || state > 2 || !freshReport(observed, 3) ||
            (state == 1 && due != observed) || (state == 0 && due != 511 && due < observed) ||
            (state == 2 && due != 511)) return;
        int c = topology.cid(x, y);
        // Consumption wins ties over an earlier observation from this round.
        // Direct vision is applied afterwards and remains authoritative.
        if (world.pearlInfo[c] > observed ||
            (world.pearlInfo[c] == observed && world.pearlDue[c] == INF && state != 2)) return;
        world.pearlInfo[c] = observed;
        world.pearlDue[c] = state == 2 || due == 511 ? INF : due;
        if (state == 2)
            for (auto &[id, claim] : world.pearlClaims)
                if (claim.c == c && claim.round <= observed) claim.c = -1;
    }
    uint64_t rallyIntentMessage() const {
        if (!validSender() || !encodableCell(postHead)) return 0;
        return coordinationMessage(RALLY_INTENT_MAGIC, uint64_t(topology.cx(postHead)) |
            (uint64_t(topology.cy(postHead)) << 6) | (uint64_t(turn.myId) << 12) |
            (uint64_t(turn.round) << 28) | (uint64_t(nextDir >= 0 ? nextDir : 4) << 37));
    }
    void receiveRallyIntent(uint64_t raw) {
        if (!validCoordination(raw, RALLY_INTENT_MAGIC)) return;
        int x = raw & 63, y = (raw >> 6) & 63, id = (raw >> 12) & 65535;
        int round = (raw >> 28) & 511, d = (raw >> 37) & 7;
        if (x >= config.width || y >= config.height || id == turn.myId || d > 4 || !freshReport(round, 1)) return;
        auto &plan = world.allyPlans[id];
        if (round >= plan.round) plan = {topology.cid(x, y), round, d < 4 ? d : -1};
    }
    // Called after direct observation, before planning. A disproved position
    // or empty pearl is removed immediately, regardless of its checksum.
    void reconcile() {
        for (auto it = world.allyLengths.begin(); it != world.allyLengths.end();) {
            if (!freshReport(it->second.round, ALLY_LENGTH_MAX_AGE)) it = world.allyLengths.erase(it);
            else ++it;
        }
        for (auto it = world.allyPlans.begin(); it != world.allyPlans.end();) {
            int c = it->second.c;
            bool contradicted = c < 0 || (occupancy.visible(c) &&
                (!occupancy.teammateHeadAt(c) || occupancy.dragonId(c) != it->first));
            if (contradicted || !freshReport(it->second.round, 1)) it = world.allyPlans.erase(it);
            else ++it;
        }
        for (auto it = world.pearlClaims.begin(); it != world.pearlClaims.end();) {
            auto &claim = it->second;
            if (!freshReport(claim.round)) it = world.pearlClaims.erase(it);
            else {
                if (claim.c >= 0 && occupancy.visible(claim.c) && !occupancy.hasPearl(claim.c)) claim.c = -1;
                ++it;
            }
        }
        for (auto it = world.portalHolds.begin(); it != world.portalHolds.end();) {
            bool active = false;
            for (int round : it->second.rounds) active |= freshReport(round, PORTAL_HOLD_MAX_AGE);
            if (!active) it = world.portalHolds.erase(it);
            else ++it;
        }
        for (auto it = world.portalOccupancy.begin(); it != world.portalOccupancy.end();) {
            if (!freshReport(it->second.round)) it = world.portalOccupancy.erase(it);
            else ++it;
        }
        for (auto it = world.portalEchoHints.begin(); it != world.portalEchoHints.end();) {
            if (!freshReport(it->second.round, 1)) it = world.portalEchoHints.erase(it);
            else ++it;
        }
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
    uint64_t allyRallyMessage() const {
        int senderMinimum = DonationPolicy::feedingActive(turn)
            ? Constants::MIN_SIZE + V5_FEED_MIN_LENGTH_GAIN
            : DonationPolicy::at(config, turn).minRecipientLength;
        // Reuse the existing packet to refresh a locally visible collector's
        // length, even when distant enemies prevent the old quiet gate.
        if (world.suppressRally ||
            !(DonationPolicy::feedingActive(turn) || DonationPolicy::quiet(config, turn, world)) ||
            (world.nextRallyCell >= 0 ? world.nextRallyLength : turn.length) < senderMinimum || turn.myId < 0 || turn.myId > 4095 ||
            turn.round > 511 || !encodableCell(world.nextRallyCell >= 0 ? world.nextRallyCell : turn.here)) return 0;
        uint64_t low = uint64_t(topology.cx(world.nextRallyCell >= 0 ? world.nextRallyCell : turn.here)) |
                       (uint64_t(topology.cy(world.nextRallyCell >= 0 ? world.nextRallyCell : turn.here)) << 6) |
                       (uint64_t(turn.myId) << 12) | (uint64_t(std::min(world.nextRallyCell >= 0 ? world.nextRallyLength : turn.length, 63)) << 24) |
                       (uint64_t(turn.round) << 30) | (uint64_t(turn.myTeam == Team::B) << 39) |
                       (RALLY_MAGIC << 40);
        return low | (uint64_t(pairTag(low)) << 48);
    }
    void receiveAllyRally(uint64_t raw) {
        uint64_t low = raw & ((1ull << 48) - 1);
        if (!DonationPolicy::active(turn) || (raw >> 48) != pairTag(low) ||
            ((low >> 40) & 255) != RALLY_MAGIC ||
            ((low >> 39) & 1) != uint64_t(turn.myTeam == Team::B)) return;
        int x = low & 63, y = (low >> 6) & 63, id = (low >> 12) & 4095;
        int length = (low >> 24) & 63, round = (low >> 30) & 511;
        if (x >= config.width || y >= config.height || id == turn.myId ||
            length < DonationPolicy::at(config, turn).minRecipientLength || round > turn.round ||
            turn.round - round > V4_DONATION_REPORT_AGE) return;
        auto it = world.allyRallies.find(id);
        if (it == world.allyRallies.end() || it->second.round <= round)
            world.allyRallies[id] = {topology.cid(x, y), round, length, round};
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
            if (due < 0 || due >= INF || due > 511 || wasEaten(c)) continue;
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
            if (uint64_t msg = portalMessage(key)) { world.urgentPortalReport = -1; return msg; }
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
            if (due < 0 || due >= INF || due > 511 || wasEaten(c)) continue;
            if (due < turn.round && !occupancy.hasPearl(c)) continue;
            int key = due * 64 + ((c * 37 + world.sonarCursor * 11) & 63);
            if (key < bestDue) {
                bestDue = key;
                bestC = c;
            }
        }
        if (bestC >= 0) return pearlStatusMessage(bestC, world.pearlDue[bestC], turn.round, occupancy.hasPearl(bestC) ? 1 : 0);
        return enemyHistoryMessage();
    }
};

} // namespace bot
