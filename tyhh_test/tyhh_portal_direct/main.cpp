// tyhh v59: portal/time-aware world model, safe split/sprint, champion endgame.
// Sonar and unrelated experimental tactics are intentionally disabled for this iteration.
#define FEAT_SONAR 1
#define FEAT_TRAP 0
#define FEAT_FEAST 0
#define FEAT_HUNT_SIGHTINGS 0
#define QOS_EARLY_PORTAL 1
// Derived from the locally validated xxqz strategy core.
//  * time-aware occupancy: a body segment k steps from its tail frees after k+1 moves,
//    so safety/space checks stop treating tails as permanent walls
//  * portal geometry: exits are inferred from portal ids as soon as both ends are seen,
//    used by every BFS, and shared over sonar
//  * sprint: 2-step head rams on valuable enemy heads, 2-pearl grabs on contested pearls
//  * sonar: one 32-bit message per turn carrying enemy sightings, portal ends and pearl timers
//  * own head trail: body segments left on the far side of a portal still count as walls
//  * endgame: short dragons feed a long friend, the champion stops trading
#include "helper.hpp"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifndef FEAT_SONAR
#define FEAT_SONAR 1
#endif
#ifndef FEAT_SPRINT_RAM
#define FEAT_SPRINT_RAM 1
#endif
#ifndef FEAT_SPRINT_EAT
#define FEAT_SPRINT_EAT 1
#endif
#ifndef FEAT_TIMED_OCC
#define FEAT_TIMED_OCC 1
#endif
#ifndef FEAT_RAM_FIRST
#define FEAT_RAM_FIRST 1
#endif
#ifndef FEAT_SACRIFICE
#define FEAT_SACRIFICE 1
#endif
#ifndef FEAT_CHAMPION
#define FEAT_CHAMPION 1
#endif
#ifndef FEAT_TRAP
#define FEAT_TRAP 1
#endif
#ifndef FEAT_FEAST
#define FEAT_FEAST 1
#endif
#ifndef FEAT_HUNT_SIGHTINGS
#define FEAT_HUNT_SIGHTINGS 1
#endif
#ifndef CH_START
#define CH_START 250
#endif
#ifndef CH_MOD
#define CH_MOD 8
#endif
#ifndef CH_LEN
#define CH_LEN 12
#endif
#ifndef SEED
#define SEED 0
#endif
#ifndef FEAT_REVERSE_SPLIT
#define FEAT_REVERSE_SPLIT 1
#endif

using namespace unswbc;

namespace {

constexpr int INF = 1 << 28;
constexpr int DX[4] = {0, 1, 0, -1};
constexpr int DY[4] = {-1, 0, 1, 0};
constexpr char DCH[4] = {'N', 'E', 'S', 'W'};
enum : uint8_t { E_UNKNOWN = 0, E_OPEN = 1, E_KELP = 2, E_PORTAL = 3 };

int W = 0, H = 0, NC = 0;
int ROUND = 0, LEN = 0, POP = 0, MYID = 0, HERE = 0, TARGET = 62;
Team::Value MYTEAM = Team::A;
Controller* CT = nullptr;

inline int cid(int x, int y) { return y * W + x; }
inline int cx(int c) { return c % W; }
inline int cy(int c) { return c / W; }
inline int geo(int c, int d) {
    int x = cx(c) + DX[d], y = cy(c) + DY[d];
    if (x < 0) x += W; else if (x >= W) x -= W;
    if (y < 0) y += H; else if (y >= H) y -= H;
    return cid(x, y);
}
inline int wrapd(int a, int m) { a = a < 0 ? -a : a; a %= m; return std::min(a, m - a); }
inline int manh(int a, int b) { return wrapd(cx(a) - cx(b), W) + wrapd(cy(a) - cy(b), H); }
inline int cheb(int a, int b) { return std::max(wrapd(cx(a) - cx(b), W), wrapd(cy(a) - cy(b), H)); }
inline int dirIndex(Direction d) {
    switch (d.value) {
    case Direction::NORTH: return 0;
    case Direction::EAST: return 1;
    case Direction::SOUTH: return 2;
    default: return 3;
    }
}

// ------------------------------------------------------------ persistent memory
std::vector<uint8_t> edgeT;       // NC*4
std::vector<int> edgePid;         // NC*4
std::vector<int> learnedExit;     // NC*4, observed teleport landing cell
std::vector<int> seenRound;       // NC, 0 = never seen
std::vector<int> pearlDue;        // NC, round a pearl is (or will be) there; -1 unknown, INF never
std::vector<int> pearlInfo;       // NC, round the pearlDue value was observed
std::map<int, std::vector<int>> portalEnds;  // portal id -> canonical edge keys
std::vector<int> portalKeys;      // every canonical portal key we know, for sonar relay
int pendingPortalKey = -1;
// Reserve a small portal room for the dragon already farming it.  Claims are
// relayed over sonar so teammates do not funnel into the same room.
int occupiedPortalId = -1;
std::map<int, int> portalClaimRound;
struct V3PortalClaim { int owner, round; };
std::map<int, V3PortalClaim> v3PortalClaims;
std::set<int> exploredPortals, crossedPortals;
int activeScoutClaim = -1;
size_t crossedRelayCursor = 0;
// our head's recent cells, newest last; the body lies on the last LEN of them (also beyond portals)
std::vector<int> trail, pendingPath;
int trailId = -1;

struct Sighting { int c, round, len; };
std::vector<Sighting> sightings;

// ------------------------------------------------------------ per-turn view
int STAMP = 0;
std::vector<int> visStamp, occStamp, occId, occFree, occPart;
std::vector<uint8_t> occMine, occHead, visPearl;

struct Part { int c, id, facing; bool mine, head; };
std::vector<Part> parts;
std::vector<int> enemyHeads, enemyHeadLen, friendParts, visPearls;
std::vector<std::pair<int, int>> idCount;  // dragon id -> visible segments

int countOf(int id) {
    for (auto const& p : idCount) if (p.first == id) return p.second;
    return 0;
}

inline int canon(int c, int d) { return (d == 1 || d == 2) ? c * 4 + d : geo(c, d) * 4 + (d ^ 2); }

void addPortal(int pid, int key) {
    auto& v = portalEnds[pid];
    if (std::find(v.begin(), v.end(), key) != v.end()) return;
    v.push_back(key);
    portalKeys.push_back(key);
}

void setEdge(int c, int d, uint8_t t, int pid) {
    int n = geo(c, d);
    edgeT[c * 4 + d] = t;
    edgeT[n * 4 + (d ^ 2)] = t;
    edgePid[c * 4 + d] = pid;
    edgePid[n * 4 + (d ^ 2)] = pid;
    if (t == E_PORTAL) addPortal(pid, canon(c, d));
}

// Cell the head lands on when leaving c through side d, -1 if unknown.
int portalExit(int c, int d) {
    int k = c * 4 + d;
    if (learnedExit[k] >= 0) return learnedExit[k];
    auto it = portalEnds.find(edgePid[k]);
    if (it == portalEnds.end()) return -1;
    int ck = canon(c, d);
    for (int key : it->second) {
        if (key == ck) continue;
        int b = key / 4, o = key % 4;
        if (o != d && o != (d ^ 2)) continue;
        return o == d ? geo(b, o) : b;
    }
    return -1;
}

// Destination of a single step, -1 for kelp or an unresolved portal.
inline int moveCell(int c, int d) {
    uint8_t e = edgeT[c * 4 + d];
    if (e == E_KELP) return -1;
    if (e == E_PORTAL) return portalExit(c, d);
    return geo(c, d);
}

inline bool visible(int c) { return visStamp[c] == STAMP; }
inline bool occupied(int c) { return occStamp[c] == STAMP; }
inline bool isEnemyHead(int c) { return occupied(c) && occHead[c] && !occMine[c]; }
inline bool hitsLongFriend(int c) {
    return occupied(c) && occMine[c] && occId[c] != MYID && countOf(occId[c]) > LEN;
}

// Blocked for the k-th move from now (k = 1 is this turn's move).
inline bool blockedAt(int c, int k, bool ate = false) {
    if (!occupied(c)) return false;
#if FEAT_TIMED_OCC
    int f = occFree[c];
    if (ate && occId[c] == MYID && occMine[c]) ++f;
    return k < f;
#else
    return true;
#endif
}

// ------------------------------------------------------------ sonar codec
constexpr uint32_t SALT = 0x5A17C3E1u;
uint32_t tagOf(uint32_t low) { return ((low ^ SALT) * 0x9E3779B1u) >> 28; }
uint32_t pack(int type, int c, int payload) {
    uint32_t low = (uint32_t)cx(c) | ((uint32_t)cy(c) << 6) | ((uint32_t)(payload & 0x3FFF) << 12) | ((uint32_t)type << 26);
    return low | (tagOf(low) << 28);
}
enum { M_PEARL = 0, M_ENEMY = 1, M_PORTAL = 2, M_PORTAL_OCCUPIED = 3 };

constexpr uint64_t V3_PAIR_MAGIC = 0xB7;
constexpr uint64_t V3_CLAIM_MAGIC = 0xC3;
uint16_t v3Tag(uint64_t low) {
    uint32_t hash = 2166136261u ^ SALT;
    for (int i = 0; i < 6; ++i) hash = (hash ^ uint8_t(low >> (8 * i))) * 16777619u;
    return uint16_t(hash ^ (hash >> 16));
}
bool receiveV3Portal(uint64_t raw) {
    uint64_t low = raw & ((1ull << 48) - 1);
    if ((raw >> 48) != v3Tag(low)) return false;
    uint64_t magic = (low >> 40) & 255;
    if (magic == V3_CLAIM_MAGIC) {
        if (((low >> 38) & 1) != uint64_t(MYTEAM == Team::B) || ((low >> 39) & 1)) return true;
        int pid = low & 8191, owner = (low >> 13) & 65535, r = (low >> 29) & 511;
        if (owner != MYID && r <= ROUND && ROUND - r <= 2 && !exploredPortals.count(pid)) {
            auto it = v3PortalClaims.find(pid);
            bool live = it != v3PortalClaims.end() && ROUND >= it->second.round && ROUND - it->second.round <= 2;
            if (!live || owner < it->second.owner || (owner == it->second.owner && r > it->second.round))
                v3PortalClaims[pid] = {owner, r};
        }
        return true;
    }
    if (magic != V3_PAIR_MAGIC) return false;
    if (((low >> 39) & 1) != uint64_t(MYTEAM == Team::B)) return true;
    int x1 = low & 63, y1 = (low >> 6) & 63, x2 = (low >> 12) & 63, y2 = (low >> 18) & 63;
    if (x1 >= W || x2 >= W || y1 >= H || y2 >= H) return true;
    int pid = (low >> 24) & 8191, d = ((low >> 37) & 1) ? 2 : 1;
    int a = cid(x1, y1) * 4 + d, b = cid(x2, y2) * 4 + d;
    if (a >= b) return true;
    for (int key : {a, b})
        if (edgeT[key] != E_UNKNOWN && (edgeT[key] != E_PORTAL || edgePid[key] != pid)) return true;
    auto it = portalEnds.find(pid);
    if (it != portalEnds.end()) for (int key : it->second) if (key != a && key != b) return true;
    setEdge(a / 4, d, E_PORTAL, pid);
    setEdge(b / 4, d, E_PORTAL, pid);
    if ((low >> 38) & 1) exploredPortals.insert(pid);
    return true;
}
uint64_t v3PairMessage(int key) {
    int pid = edgePid[key];
    auto it = portalEnds.find(pid);
    if (pid < 0 || pid >= 8192 || it == portalEnds.end() || it->second.size() != 2) return 0;
    int a = std::min(it->second[0], it->second[1]), b = std::max(it->second[0], it->second[1]);
    int d = a % 4;
    if (a == b || (d != 1 && d != 2) || b % 4 != d) return 0;
    int ax = cx(a / 4), ay = cy(a / 4), bx = cx(b / 4), by = cy(b / 4);
    if (ax >= 64 || ay >= 64 || bx >= 64 || by >= 64) return 0;
    uint64_t low = uint64_t(ax) | (uint64_t(ay) << 6) | (uint64_t(bx) << 12) | (uint64_t(by) << 18) |
                   (uint64_t(pid) << 24) | (uint64_t(d == 2) << 37) |
                   (uint64_t(exploredPortals.count(pid)) << 38) |
                   (uint64_t(MYTEAM == Team::B) << 39) | (V3_PAIR_MAGIC << 40);
    return low | (uint64_t(v3Tag(low)) << 48);
}
uint64_t v3ClaimMessage(int pid) {
    if (pid < 0 || pid >= 8192 || exploredPortals.count(pid) || MYID < 0 || MYID > 65535) return 0;
    uint64_t low = uint64_t(pid) | (uint64_t(MYID) << 13) | (uint64_t(ROUND & 511) << 29) |
                   (uint64_t(MYTEAM == Team::B) << 38) | (V3_CLAIM_MAGIC << 40);
    return low | (uint64_t(v3Tag(low)) << 48);
}
uint64_t v3CrossedPairMessage() {
    if (crossedPortals.empty()) return 0;
    size_t start = crossedRelayCursor++ % crossedPortals.size();
    auto it = crossedPortals.begin();
    std::advance(it, start);
    for (size_t n = 0; n < crossedPortals.size(); ++n) {
        if (it == crossedPortals.end()) it = crossedPortals.begin();
        int pid = *it++;
        auto ends = portalEnds.find(pid);
        if (ends != portalEnds.end() && !ends->second.empty())
            if (uint64_t msg = v3PairMessage(ends->second.front())) return msg;
    }
    return 0;
}

void receive(Controller const& ct) {
    for (auto raw64 : ct.get_sonar_messages()) {
        if (raw64 > 0xFFFFFFFFull && receiveV3Portal(raw64)) continue;
        if (raw64 > 0xFFFFFFFFull) continue;
        uint32_t raw = (uint32_t)raw64, low = raw & 0x0FFFFFFFu;
        if ((raw >> 28) != tagOf(low)) continue;
        int x = low & 63, y = (low >> 6) & 63, payload = (low >> 12) & 0x3FFF, type = (low >> 26) & 3;
        if (x >= W || y >= H) continue;
        int c = cid(x, y);
        if (type == M_PEARL) {
            int due = payload & 511;
            if (pearlInfo[c] < ROUND - 1) { pearlDue[c] = due; pearlInfo[c] = ROUND - 1; }
        } else if (type == M_ENEMY) {
            int r = payload & 511, l = payload >> 9;
            if (ROUND - r > 6 || r > ROUND) continue;
            sightings.push_back({c, r, l});
        } else if (type == M_PORTAL) {
            int d = (payload & 1) ? 2 : 1, pid = payload >> 1;
            if (edgeT[c * 4 + d] != E_PORTAL) setEdge(c, d, E_PORTAL, pid);
        } else if (type == M_PORTAL_OCCUPIED) {
            portalClaimRound[payload] = ROUND;
        }
    }
    sightings.erase(std::remove_if(sightings.begin(), sightings.end(),
                                   [](Sighting const& s) { return ROUND - s.round > 6; }),
                    sightings.end());
    for (auto it = portalClaimRound.begin(); it != portalClaimRound.end();) {
        if (ROUND - it->second > 6) it = portalClaimRound.erase(it);
        else ++it;
    }
    if (sightings.size() > 24) sightings.erase(sightings.begin(), sightings.end() - 24);
    for (auto it = v3PortalClaims.begin(); it != v3PortalClaims.end();) {
        if (ROUND - it->second.round > 2) it = v3PortalClaims.erase(it);
        else ++it;
    }
}

int sonarCursor = 0;
uint32_t chooseMessage() {
    if (occupiedPortalId >= 0 && occupiedPortalId < 8192)
        return pack(M_PORTAL_OCCUPIED, HERE, occupiedPortalId);
    if (!enemyHeads.empty()) {
        int best = 0;
        for (size_t i = 1; i < enemyHeads.size(); ++i) if (enemyHeadLen[i] > enemyHeadLen[best]) best = (int)i;
        return pack(M_ENEMY, enemyHeads[best], (ROUND & 511) | (std::min(enemyHeadLen[best], 31) << 9));
    }
    ++sonarCursor;
    if (!portalKeys.empty() && (sonarCursor & 1)) {
        int key = portalKeys[(sonarCursor / 2) % portalKeys.size()];
        int c = key / 4, d = key % 4;
        int pid = edgePid[c * 4 + d];
        if (pid >= 0 && pid < 8192) return pack(M_PORTAL, c, (pid << 1) | (d == 2 ? 1 : 0));
    }
    // a pearl tile in view that will be ripe soon
    int bestC = -1, bestDue = INF;
    for (auto const& t : CT->get_tiles()) {
        int c = cid(t.position.x, t.position.y);
        int due = pearlDue[c];
        if (due < 0 || due >= INF || due > 511) continue;
        if (due < ROUND && !visPearl[c]) continue;
        int key = due * 64 + ((c * 37 + sonarCursor * 11) & 63);
        if (key < bestDue) { bestDue = key; bestC = c; }
    }
    if (bestC >= 0) return pack(M_PEARL, bestC, pearlDue[bestC]);
    return 0;
}

bool portalRoomClaimed(int pid) {
    if (pid < 0) return false;
    if (occupiedPortalId == pid) return true;
    auto it = portalClaimRound.find(pid);
    return it != portalClaimRound.end() && ROUND - it->second <= 6;
}

// ------------------------------------------------------------ observation
void observe(Controller& ct) {
    ++STAMP;
    parts.clear(); enemyHeads.clear(); enemyHeadLen.clear(); friendParts.clear(); visPearls.clear(); idCount.clear();
    for (auto const& t : ct.get_tiles()) {
        int c = cid(t.position.x, t.position.y);
        visStamp[c] = STAMP;
        seenRound[c] = ROUND;
        visPearl[c] = t.has_pearl();
        int pt = t.get_pearl_time();
        if (t.has_pearl()) { pearlDue[c] = ROUND; visPearls.push_back(c); }
        else pearlDue[c] = pt >= 0 ? ROUND + pt : INF;
        pearlInfo[c] = ROUND;
        for (int d = 0; d < 4; ++d) {
            auto const& e = t.get_edge(Direction(DCH[d]));
            uint8_t ty = e.get_edge_type() == EdgeType::KELP ? E_KELP : e.is_portal() ? E_PORTAL : E_OPEN;
            if (edgeT[c * 4 + d] != ty || (ty == E_PORTAL && edgePid[c * 4 + d] != e.get_portal_id()))
                setEdge(c, d, ty, e.get_portal_id());
        }
        if (auto const* dr = t.get_dragon()) {
            Part p{c, dr->get_id(), dirIndex(dr->get_dir()), dr->get_team() == MYTEAM, dr->is_head()};
            occStamp[c] = STAMP;
            occId[c] = p.id;
            occMine[c] = p.mine;
            occHead[c] = p.head;
            occFree[c] = INF;
            occPart[c] = (int)parts.size();
            parts.push_back(p);
            bool found = false;
            for (auto& ic : idCount) if (ic.first == p.id) { ++ic.second; found = true; break; }
            if (!found) idCount.push_back({p.id, 1});
        }
    }
    for (auto const& p : parts) {
        if (p.mine) { if (p.c != HERE) friendParts.push_back(p.c); }
        else if (p.head) enemyHeads.push_back(p.c);
    }
    for (int c : enemyHeads) enemyHeadLen.push_back(countOf(occId[c]));

    // chain reconstruction: facing points to the next segment towards the head
    std::vector<int> prevOf(parts.size(), -1);
    for (size_t i = 0; i < parts.size(); ++i) {
        auto const& p = parts[i];
        if (p.head) continue;
        int n = moveCell(p.c, p.facing);
        if (n >= 0 && occupied(n) && occId[n] == p.id && occMine[n] == p.mine) prevOf[occPart[n]] = (int)i;
    }
    for (size_t hi = 0; hi < parts.size(); ++hi) {
        auto const& hp = parts[hi];
        if (!hp.head) continue;
        std::vector<int> chain;
        for (int cur = (int)hi; cur >= 0 && chain.size() < parts.size(); cur = prevOf[cur]) chain.push_back(cur);
        int k = (int)chain.size();
        int total = -1;
        if (hp.mine && hp.id == MYID) total = LEN;
        else {
            int tail = parts[chain.back()].c;
            bool confirmed = true;
            for (int d = 0; d < 4 && confirmed; ++d) {
                uint8_t e = edgeT[tail * 4 + d];
                if (e == E_KELP) continue;
                int n = moveCell(tail, d);
                if (n < 0 || !visible(n)) confirmed = false;
            }
            if (confirmed) total = k;
        }
        if (total < 0) continue;
        // another dragon may eat (tail stays) or split (it stands still) before we get there
        int slack = 0;
        if (!(hp.mine && hp.id == MYID)) {
            for (int d = 0; d < 4; ++d) { int n = moveCell(hp.c, d); if (n >= 0 && visible(n) && visPearl[n]) slack = 1; }
            if (total >= 4) slack = std::max(slack, 1);
        }
        for (int j = 0; j < k; ++j) {
            int idx = std::max(0, total - 1 - j);
            occFree[parts[chain[j]].c] = idx + 2 + slack + (slack && idx == 0 ? 1 : 0);
        }
    }

    // own segments out of view (typically on the far side of a portal)
    int n = (int)trail.size();
    for (int j = 1; j < LEN && j < n; ++j) {
        int c = trail[n - 1 - j];
        if (visible(c)) {
            if (occupied(c) && occMine[c] && occId[c] == MYID) continue;
            trail.erase(trail.begin(), trail.end() - j);
            break;
        }
        occStamp[c] = STAMP; occId[c] = MYID; occMine[c] = 1; occHead[c] = 0;
        occFree[c] = LEN - 1 - j + 2; occPart[c] = -1;
    }
}

// ------------------------------------------------------------ searches
std::vector<int> bfsMark, bfsDist, bfsFirst, bfsQ;
int bfsStamp = 0;

// Cells reachable from dest (reached on move 1) honouring time-aware occupancy.
int spaceFrom(int dest, bool ate, int cap, bool visionOnly) {
    ++bfsStamp;
    int qh = 0, qt = 0, n = 0;
    bfsQ[qt++] = dest; bfsMark[dest] = bfsStamp; bfsDist[dest] = 1;
    while (qh < qt && n < cap) {
        int p = bfsQ[qh++]; ++n;
        int k = bfsDist[p] + 1;
        for (int d = 0; d < 4; ++d) {
            int q = moveCell(p, d);
            if (q < 0 || bfsMark[q] == bfsStamp) continue;
            if (visionOnly && !visible(q)) continue;
            if (blockedAt(q, k, ate)) continue;
            bfsMark[q] = bfsStamp; bfsDist[q] = k; bfsQ[qt++] = q;
        }
    }
    return n;
}

int onwardSafe(int dest, bool ate) {
    int n = 0;
    for (int d = 0; d < 4; ++d) {
        int q = moveCell(dest, d);
        if (q < 0) { if (edgeT[dest * 4 + d] == E_PORTAL) ++n; continue; }
        if (!blockedAt(q, 2, ate)) ++n;
    }
    return n;
}

// Tron-style Voronoi: cells we reach no later than the nearest visible enemy head.
int controlledSpace(int dest, bool ate, int cap) {
    ++bfsStamp;
    int qh = 0, qt = 0, n = 0;
    bfsQ[qt++] = dest; bfsMark[dest] = bfsStamp; bfsDist[dest] = 0;
    while (qh < qt && n < cap) {
        int p = bfsQ[qh++];
        int dd = bfsDist[p];
        int em = INF;
        for (int eh : enemyHeads) em = std::min(em, manh(p, eh));
        if (dd > em) continue;
        ++n;
        for (int d = 0; d < 4; ++d) {
            int q = moveCell(p, d);
            if (q < 0 || bfsMark[q] == bfsStamp) continue;
            if (blockedAt(q, dd + 2, ate)) continue;
            bfsMark[q] = bfsStamp; bfsDist[q] = dd + 1; bfsQ[qt++] = q;
        }
    }
    return n;
}

// Count the unexplored frontier reachable from a candidate direction.  This
// uses only the current map memory and therefore adapts to any large map.
int unseenFrontier(int start, int cap) {
    ++bfsStamp;
    int qh = 0, qt = 0, n = 0, unseen = 0;
    bfsQ[qt++] = start; bfsMark[start] = bfsStamp; bfsDist[start] = 0;
    while (qh < qt && n < cap) {
        int p = bfsQ[qh++]; ++n;
        if (seenRound[p] == 0) ++unseen;
        if (bfsDist[p] >= 10) continue;
        for (int d = 0; d < 4; ++d) {
            int q = moveCell(p, d);
            if (q < 0 || bfsMark[q] == bfsStamp || blockedAt(q, bfsDist[p] + 1)) continue;
            bfsMark[q] = bfsStamp; bfsDist[q] = bfsDist[p] + 1; bfsQ[qt++] = q;
        }
    }
    return unseen;
}

int visPearlDist(int s) {
    if (visPearls.empty()) return 999;
    ++bfsStamp;
    int qh = 0, qt = 0;
    bfsQ[qt++] = s; bfsMark[s] = bfsStamp; bfsDist[s] = 0;
    while (qh < qt) {
        int p = bfsQ[qh++];
        if (visPearl[p] && visible(p)) return bfsDist[p];
        for (int d = 0; d < 4; ++d) {
            int q = moveCell(p, d);
            if (q < 0 || bfsMark[q] == bfsStamp || !visible(q)) continue;
            if (blockedAt(q, bfsDist[p] + 2)) continue;
            bfsMark[q] = bfsStamp; bfsDist[q] = bfsDist[p] + 1; bfsQ[qt++] = q;
        }
    }
    return 999;
}

// First direction of a shortest path to a goal cell (portal aware), -1 if none.
template <class Goal>
int bfsFirstDir(Goal isGoal, int maxDepth, int maxNodes) {
    ++bfsStamp;
    int qh = 0, qt = 0;
    bfsQ[qt++] = HERE; bfsMark[HERE] = bfsStamp; bfsDist[HERE] = 0; bfsFirst[HERE] = -1;
    while (qh < qt && qt < maxNodes) {
        int p = bfsQ[qh++];
        if (bfsDist[p] >= maxDepth) continue;
        for (int d = 0; d < 4; ++d) {
#if !QOS_EARLY_PORTAL
            if (LEN == 2 && edgeT[p * 4 + d] == E_PORTAL) continue;
#endif
            int q = moveCell(p, d);
            if (q < 0 || bfsMark[q] == bfsStamp) continue;
            int k = bfsDist[p] + 1;
            if (blockedAt(q, k)) continue;
            bfsMark[q] = bfsStamp; bfsDist[q] = k;
            bfsFirst[q] = p == HERE ? d : bfsFirst[p];
            if (isGoal(q)) return bfsFirst[q];
            bfsQ[qt++] = q;
        }
    }
    return -1;
}

bool scoutPortalRoute(int& firstDir, int& targetPid) {
    firstDir = -1;
    targetPid = -1;
    unsigned safeFirst = 0;
    for (int d = 0; d < 4; ++d) {
        int key = HERE * 4 + d;
        int q = moveCell(HERE, d);
        if (q < 0) {
            if (edgeT[key] == E_PORTAL && !exploredPortals.count(edgePid[key]) &&
                !portalRoomClaimed(edgePid[key])) safeFirst |= 1u << d;
            continue;
        }
        if (hitsLongFriend(q) || blockedAt(q, 1) || onwardSafe(q, visible(q) && visPearl[q]) <= 1 ||
            spaceFrom(q, visible(q) && visPearl[q], 12, false) < std::min(LEN + 1, 8)) continue;
        safeFirst |= 1u << d;
    }
    if (!safeFirst) return false;

    ++bfsStamp;
    int qh = 0, qt = 0, bestDistance = INF;
    bfsQ[qt++] = HERE;
    bfsMark[HERE] = bfsStamp;
    bfsDist[HERE] = 0;
    bfsFirst[HERE] = -1;
    for (; qh < qt && qt < 320; ++qh) {
        int p = bfsQ[qh];
        int distance = bfsDist[p];
        if (distance >= 6 || distance >= bestDistance) continue;
        for (int d = 0; d < 4; ++d) {
            if (p == HERE && !(safeFirst & (1u << d))) continue;
            int key = p * 4 + d;
            int type = edgeT[key];
            if (type != E_OPEN && type != E_PORTAL) continue;
            int next = moveCell(p, d);
            int nextDistance = distance + 1;
            if (nextDistance > bestDistance) continue;
            if (next >= 0 && blockedAt(next, nextDistance)) continue;
            if (type == E_PORTAL && !exploredPortals.count(edgePid[key])) {
                int pid = edgePid[key];
                bool claimed = portalRoomClaimed(pid);
                auto claim = v3PortalClaims.find(pid);
                if (claim != v3PortalClaims.end() && claim->second.owner < MYID &&
                    ROUND >= claim->second.round && ROUND - claim->second.round <= 2) claimed = true;
                int localDistance = INF;
                for (auto const& part : parts) {
                    if (!part.mine || !part.head || part.id == MYID || countOf(part.id) > 8) continue;
                    int approach = std::min(manh(part.c, p), manh(part.c, geo(p, d))) + 1;
                    if (approach <= 3 && (approach < nextDistance || (approach == nextDistance && part.id < MYID)))
                        localDistance = std::min(localDistance, approach);
                }
                if (!claimed && localDistance == INF) {
                    bestDistance = nextDistance;
                    firstDir = p == HERE ? d : bfsFirst[p];
                    targetPid = pid;
                }
            }
            if (next < 0 || nextDistance >= bestDistance || bfsMark[next] == bfsStamp ||
                qt >= 320) continue;
            bfsMark[next] = bfsStamp;
            bfsDist[next] = nextDistance;
            bfsFirst[next] = p == HERE ? d : bfsFirst[p];
            bfsQ[qt++] = next;
        }
    }
    return firstDir >= 0;
}

int enemyEscapes(int eh) {
    if (!visible(eh)) return 4;
    int n = 0;
    for (int d = 0; d < 4; ++d) {
        if (edgeT[eh * 4 + d] == E_KELP) continue;
        int q = moveCell(eh, d);
        if (q < 0) { ++n; continue; }
        if (!visible(q) || !blockedAt(q, 1)) ++n;
    }
    return n;
}

// Own segment by index from the tail (0), or -1 when out of view.
int ownSeg(int idx) {
    int j = LEN - 1 - idx, n = (int)trail.size();
    if (j >= 0 && j < n) return trail[n - 1 - j];
    for (auto const& p : parts)
        if (p.mine && p.id == MYID && occFree[p.c] == idx + 2) return p.c;
    return -1;
}
// Two consecutive segments joined through a portal rather than a plain edge.
bool bridged(int a, int b) {
    if (a < 0 || b < 0) return false;
    for (int d = 0; d < 4; ++d) if (geo(a, d) == b) return false;
    return true;
}
// The engine kills a length-2 dragon whose two segments straddle a portal.
bool splitSafe(int k) {
    if (LEN - k == 2 && bridged(HERE, ownSeg(LEN - 2))) return false;
    if (k == 2 && bridged(ownSeg(0), ownSeg(1))) return false;
    return true;
}

// A split-off child's head is our tail cell; it needs a free cell besides its own neck.
bool tailCanEscape() {
    int tail = -1;
    for (auto const& p : parts)
        if (p.mine && p.id == MYID && occFree[p.c] == 2) { tail = p.c; break; }
    if (tail < 0) return true;
    for (int d = 0; d < 4; ++d) {
        int q = moveCell(tail, d);
        if (q >= 0 && (!visible(q) || !occupied(q))) return true;
    }
    return false;
}

// ------------------------------------------------------------ actions
#ifdef DEBUG_PORTAL
int pendingPred = -1;
#endif
void doMove(int d) {
    if (edgeT[HERE * 4 + d] == E_PORTAL) {
        int pid = edgePid[HERE * 4 + d];
        // Crossing the same portal toggles room residency.  A resident dragon
        // may leave; teammates use its sonar claim to stay outside.
        if (pid >= 0) occupiedPortalId = (occupiedPortalId == pid) ? -1 : pid;
        pendingPortalKey = HERE * 4 + d;
#ifdef DEBUG_PORTAL
        learnedExit[pendingPortalKey] = -1;
        pendingPred = portalExit(HERE, d);
#endif
    }
    CT->make_move(Direction(DCH[d]));
}
void doSprint(std::initializer_list<int> ds) {
    std::string s = "MOVE ";
    int c = HERE;
    pendingPath.clear();
    for (int d : ds) {
        s += DCH[d];
        if (c >= 0 && c != HERE) pendingPath.push_back(c);
        c = c >= 0 ? moveCell(c, d) : -1;
    }
    std::cout << s << "\n";
}

// ------------------------------------------------------------ one turn
void takeTurn(Controller& ct) {
    activeScoutClaim = -1;
    bool attrition = POP >= 12;
    bool bigMap = W * H >= 600;
    // a head-on trade could wipe out the whole team
    bool fragile = POP <= 2;

    bool scout = false;
    int scoutEnd = W * H >= 1200 ? 300 : 220;
    bool unknownPortal = false;
    for (int key : portalKeys) {
        int pid = edgePid[key];
        if (edgeT[key] == E_PORTAL && pid >= 0 && !exploredPortals.count(pid)) {
            unknownPortal = true;
            break;
        }
    }
    bool championCandidate = false;
#if FEAT_CHAMPION
    championCandidate = ROUND >= CH_START && (MYID % CH_MOD == 0 || LEN >= CH_LEN);
#endif
    bool huntTarget = false;
    for (int lengthSeen : enemyHeadLen)
        if (lengthSeen >= LEN + 3) huntTarget = true;
#if FEAT_HUNT_SIGHTINGS
    for (auto const& sighting : sightings)
        if (sighting.len >= LEN + 3 && ROUND - sighting.round <= 3 && manh(HERE, sighting.c) <= 14)
            huntTarget = true;
#endif
    scout = ROUND < scoutEnd && POP >= 6 && LEN >= 2 && LEN <= 8 && !championCandidate &&
            !huntTarget && enemyHeads.empty() && unknownPortal;

#if QOS_EARLY_PORTAL
    // Early exploration is an explicit policy, not a small score bonus: once
    // an unclaimed portal is visible, enter it before swarm splitting or the
    // ordinary pearl/space scorer can choose another move.
    if (ROUND < 80 && !scout) {
        int earlyPortalDir = -1;
        for (int d = 0; d < 4; ++d) {
            if (edgeT[HERE * 4 + d] != E_PORTAL) continue;
            int pid = edgePid[HERE * 4 + d];
            if (portalRoomClaimed(pid)) continue;
            int dest = moveCell(HERE, d);
            if ((dest < 0 || !blockedAt(dest, 1)) && !hitsLongFriend(dest)) {
                earlyPortalDir = d;
                break;
            }
        }
        if (earlyPortalDir >= 0) { doMove(earlyPortalDir); return; }
    }
#endif

    // champion: probably our longest dragon, protect it late in the game
    // champion: a few dragons stop splitting late and play safe, for the longest-dragon tiebreak
    bool champion = false;
#if FEAT_CHAMPION
    champion = ROUND >= CH_START && (MYID % CH_MOD == 0 || LEN >= CH_LEN);
#endif

    // ---- adjacent enemy heads we could ram this move (portal aware)
    int ramDir = -1, ramSz = -1;
    for (int d = 0; d < 4; ++d) {
        int q = moveCell(HERE, d);
        if (q < 0 || !isEnemyHead(q)) continue;
        int sz = countOf(occId[q]);
        if (sz > ramSz) { ramSz = sz; ramDir = d; }
    }
    bool ramGood = ramDir >= 0 && ramSz >= LEN - 1 && ramSz >= 3 && !champion && POP > 1;

#if FEAT_RAM_FIRST
    if (ramGood && POP >= 4) { doMove(ramDir); return; }
#endif

    // ---- 1) keep the swarm at the cap
    bool wantExpand = (ROUND < 470 && !champion) || POP < 8;
    if (wantExpand && !scout && POP < TARGET && LEN >= 4 && (!bigMap || LEN <= 22) && ct.can_split(2) && splitSafe(2)) {
        ct.do_split(2);
        return;
    }

    // ---- 2) ram an adjacent head (trade-up / even)
    if (ramGood) { doMove(ramDir); return; }

#if FEAT_SPRINT_RAM
    // ---- 2b) sprint two tiles into a valuable enemy head
    if (LEN >= 3 && !champion && POP >= 4) {
        int bestSz = -1, b1 = -1, b2 = -1;
        for (int d1 = 0; d1 < 4; ++d1) {
            if (edgeT[HERE * 4 + d1] == E_PORTAL) continue;
            int m = moveCell(HERE, d1);
            if (m < 0 || blockedAt(m, 1)) continue;
            for (int d2 = 0; d2 < 4; ++d2) {
                if (edgeT[m * 4 + d2] == E_PORTAL) continue;
                int q = moveCell(m, d2);
                if (q < 0 || q == HERE || !isEnemyHead(q)) continue;
                int sz = countOf(occId[q]);
                if (sz > bestSz) { bestSz = sz; b1 = d1; b2 = d2; }
            }
        }
        bool worth = bestSz >= LEN + 2 && bestSz >= 5;
        if (attrition && bestSz >= LEN + 1 && bestSz >= 4) worth = true;
        if (b1 >= 0 && worth) { doSprint({b1, b2}); return; }
    }
#endif

#if FEAT_SPRINT_EAT
    // ---- 2c) grab two pearls in one turn when an enemy head is contesting them
    if (LEN >= 3 && !enemyHeads.empty()) {
        for (int d1 = 0; d1 < 4; ++d1) {
            if (LEN <= 3 && edgeT[HERE * 4 + d1] == E_PORTAL) continue;
            int m = moveCell(HERE, d1);
            if (m < 0 || !visible(m) || !visPearl[m] || blockedAt(m, 1)) continue;
            for (int d2 = 0; d2 < 4; ++d2) {
                if (edgeT[m * 4 + d2] == E_PORTAL) continue;
                int q = moveCell(m, d2);
                if (q < 0 || q == HERE || !visible(q) || !visPearl[q] || occupied(q)) continue;
                bool contested = false;
                for (int eh : enemyHeads) if (manh(eh, q) <= 2 || manh(eh, m) <= 2) contested = true;
                if (!contested) continue;
                bool nearHead = false;
                for (int eh : enemyHeads) if (cheb(eh, q) <= 1) nearHead = true;
                if (nearHead && !attrition) continue;
                if (spaceFrom(q, true, 12, false) < std::min(LEN + 2, 8)) continue;
                doSprint({d1, d2});
                return;
            }
        }
    }
#endif

    if (scout) {
        int scoutDir = -1, scoutPid = -1;
        if (scoutPortalRoute(scoutDir, scoutPid)) {
            activeScoutClaim = scoutPid;
            doMove(scoutDir);
            return;
        }
    }

    // ---- 3) route: soon-ripe pearls, then unexplored cells
    int planDir = -1;
    {
        bool any = !visPearls.empty();
        static std::vector<uint8_t> isGoal;
        isGoal.assign(NC, 0);
        for (int c : visPearls) isGoal[c] = 1;
        for (int c = 0; c < NC; ++c) {
            int due = pearlDue[c];
            if (due >= 0 && due < INF && due - ROUND <= 8) { isGoal[c] = 1; any = true; }
        }
        int depth = 24, nodes = 320;
        if (any) planDir = bfsFirstDir([&](int q) { return isGoal[q] != 0; }, depth, nodes);
        if (planDir < 0) planDir = bfsFirstDir([&](int q) { return seenRound[q] == 0; }, depth, nodes);
    }
    bool pearlInView = !visPearls.empty();

    // ---- 4) score safe moves
    int const flr = std::min(LEN + 1, 8);
    int best = -1;
    long bestScore = LONG_MIN;
#ifdef DEBUG_PORTAL
    long dbgScore[4] = {LONG_MIN, LONG_MIN, LONG_MIN, LONG_MIN};
    struct DbgOut {
        long* s; int* b; ~DbgOut() {
            if (*b >= 0 && (edgeT[HERE * 4 + *b] == E_PORTAL || (cx(HERE) == 16 && cy(HERE) == 7)))
                std::clog << "PCHOICE r" << ROUND << " id" << MYID << " at " << cx(HERE) << "," << cy(HERE) << " len" << LEN
                          << " d" << *b << " exit" << portalExit(HERE, *b) << " scores " << s[0] << " " << s[1] << " " << s[2] << " " << s[3] << std::endl;
        }
    } dbgOut{dbgScore, &best};
#endif
    for (int d = 0; d < 4; ++d) {
        uint8_t e = edgeT[HERE * 4 + d];
        if (e == E_KELP) continue;
        int dest = moveCell(HERE, d);
        bool isPortal = e == E_PORTAL;
#if !QOS_EARLY_PORTAL
        if (isPortal && LEN == 2) continue;
#endif
        if (isPortal && portalRoomClaimed(edgePid[HERE * 4 + d]) &&
            occupiedPortalId != edgePid[HERE * 4 + d]) continue;
        if (dest < 0) {
            if (!isPortal) continue;
            // unresolved portal: a mild exploration option
            long ps = 30;
#if QOS_EARLY_PORTAL
            if (ROUND < 80) ps += 260;
#endif
            auto it = portalEnds.find(edgePid[HERE * 4 + d]);
            if (it == portalEnds.end() || it->second.size() < 2) ps += 50;
#ifdef DEBUG_PORTAL
            dbgScore[d] = ps;
#endif
            if (best < 0 || ps > bestScore) { bestScore = ps; best = d; }
            continue;
        }
        // Never let a short dragon move into any segment of a longer friendly dragon.
        if (hitsLongFriend(dest)) continue;
        if (blockedAt(dest, 1)) continue;
        bool eats = visible(dest) && visPearl[dest];
        int space = spaceFrom(dest, eats, 90, false);
        int opens = onwardSafe(dest, eats);
        int frontier = (W * H >= 1200 && ROUND < 220) ? unseenFrontier(dest, 180) : 0;
        int crowd = 0;
        int nearestFriendHead = 99;
        for (auto const& p : parts) if (p.mine && p.head && p.id != MYID)
            nearestFriendHead = std::min(nearestFriendHead, cheb(dest, p.c));
        for (int f : friendParts) if (cheb(dest, f) <= 3) ++crowd;

        long score = (long)controlledSpace(dest, eats, 80) * 11;

        if (champion || fragile)
            for (int eh : enemyHeads) {
                int md = manh(dest, eh);
                if (md <= 1) score -= 1000;
                else if (md == 2) score -= 150;
            }
        for (size_t i = 0; i < enemyHeads.size() && !champion && !fragile; ++i) {
            int eh = enemyHeads[i];
            if (cheb(dest, eh) > 1) continue;
            int esz = enemyHeadLen[i];
            if (esz > LEN) score += std::min((esz - LEN) * 30L, 260L);
            else if (LEN > esz) score -= std::min((LEN - esz) * (attrition ? 5L : 12L), attrition ? 40L : 120L);
            else score += attrition ? 25 : -15;
#if FEAT_FEAST
            {
                int fN = 0, eN = 0;
                for (int f : friendParts) if (cheb(f, eh) <= 2) ++fN;
                for (int e2 : enemyHeads) if (cheb(e2, eh) <= 2) ++eN;
                if (fN > eN) score += std::min((long)esz * 8L + 25L, 150L);
            }
#endif
        }
#if FEAT_TRAP
        for (int eh : enemyHeads)
            if (cheb(dest, eh) == 1 && manh(dest, eh) == 1) {
                int esc = enemyEscapes(eh);
                if (esc <= 2) score += (long)(3 - esc) * 40L;
            }
#endif
        if (LEN <= 5 && POP > 6 && !champion) {
            int nearD = 99;
            for (size_t i = 0; i < enemyHeads.size(); ++i)
                if (enemyHeadLen[i] > LEN + 2) nearD = std::min(nearD, manh(dest, enemyHeads[i]));
#if FEAT_HUNT_SIGHTINGS
            if (nearD == 99)
                for (auto const& s : sightings)
                    if (s.len > LEN + 2 && ROUND - s.round <= 3 && manh(HERE, s.c) <= 14)
                        nearD = std::min(nearD, manh(dest, s.c) + 4);
#endif
            if (nearD < 99) score -= (long)nearD * 14;
        }
        if (champion)
            for (auto const& s : sightings)
                if (ROUND - s.round <= 2 && manh(dest, s.c) <= 3) score -= (4 - manh(dest, s.c)) * 60L;

        if (pearlInView) { score -= (long)std::min(visPearlDist(dest), 14) * 25; if (eats) score += 500; }
        if (d == planDir) score += 30;
        if (isPortal) score += 25;
#if QOS_EARLY_PORTAL
        if (ROUND < 80 && isPortal) score += 360;
#endif
        if (seenRound[dest] == 0) score += (W * H >= 1200 && ROUND < 220) ? 360 : 130;

#if QOS_EARLY_PORTAL
        if (W * H >= 1200 && ROUND < 220 && nearestFriendHead < 99) {
            score += (long)std::min(nearestFriendHead, 10) * 48;
            if (nearestFriendHead < 4) score -= (long)(4 - nearestFriendHead) * 110;
        }
        if (W * H >= 1200 && ROUND < 220) score += (long)frontier * 9;
#endif
        score -= (long)crowd * ((W * H >= 1200 && ROUND < 220) ? 65 : 35);
        if (opens <= 1 && !eats) score -= 80;
        if (space < flr) score -= (long)(flr - space) * 120;
#ifdef DEBUG_PORTAL
        dbgScore[d] = score;
#endif
#if SEED
        score += ((uint32_t)(SEED * 2654435761u) ^ (uint32_t)(MYID * 40503u) ^ (uint32_t)(ROUND * 9973u) ^ (uint32_t)(d * 7919u)) * 2246822519u >> 29;
#endif

        if (best < 0 || score > bestScore) { bestScore = score; best = d; }
    }

    if (best < 0) {
#if FEAT_REVERSE_SPLIT
        // trapped: cut everything but the front two segments loose; the child faces the way we came
        if (LEN >= 5 && ct.can_split(LEN - 2) && tailCanEscape() && splitSafe(LEN - 2)) { ct.do_split(LEN - 2); return; }
#endif
        if (ct.can_split(2) && splitSafe(2)) { ct.do_split(2); return; }
    }
    if (best < 0) {
        // nothing is safe: prefer a head we can at least trade with, else any open edge
        if (ramDir >= 0) { doMove(ramDir); return; }
        int bs = -1;
        for (int d = 0; d < 4; ++d) {
            int q = moveCell(HERE, d);
            if (q < 0 || hitsLongFriend(q)) continue;
            int s = occupied(q) ? 0 : 1;
            if (s > bs) { bs = s; best = d; }
        }
        if (best < 0) best = dirIndex(ct.get_dir());
    }
    doMove(best);
}

}  // namespace

int main() {
    auto [ct, game] = init();
    CT = &ct;
    W = game.width; H = game.height; NC = W * H;
    TARGET = std::min(62, game.get_unit_limit() - 2);
    edgeT.assign(NC * 4, E_UNKNOWN);
    edgePid.assign(NC * 4, -1);
    learnedExit.assign(NC * 4, -1);
    seenRound.assign(NC, 0);
    pearlDue.assign(NC, -1);
    pearlInfo.assign(NC, -1000);
    visStamp.assign(NC, 0); occStamp.assign(NC, 0); occId.assign(NC, -1); occFree.assign(NC, INF);
    occPart.assign(NC, -1); occMine.assign(NC, 0); occHead.assign(NC, 0); visPearl.assign(NC, 0);
    bfsMark.assign(NC, 0); bfsDist.assign(NC, 0); bfsFirst.assign(NC, -1); bfsQ.assign(NC + 8, 0);

    while (update(ct, game)) {
        ROUND = game.get_round_num();
        LEN = ct.get_length();
        POP = ct.get_unit_count();
        MYID = ct.get_id();
        MYTEAM = ct.get_team().value;
        HERE = cid(ct.get_position().x, ct.get_position().y);
        if (pendingPortalKey >= 0) {
            int crossedPid = edgePid[pendingPortalKey];
            if (crossedPid >= 0) {
                crossedPortals.insert(crossedPid);
                exploredPortals.insert(crossedPid);
            }
#ifdef DEBUG_PORTAL
            std::clog << "PORTAL r" << ROUND << " id" << MYID << " from " << cx(pendingPortalKey / 4) << "," << cy(pendingPortalKey / 4)
                      << " d" << pendingPortalKey % 4 << " pred " << (pendingPred < 0 ? -1 : cx(pendingPred)) << ","
                      << (pendingPred < 0 ? -1 : cy(pendingPred)) << " got " << cx(HERE) << "," << cy(HERE)
                      << (pendingPred == HERE ? " OK" : pendingPred < 0 ? " UNK" : " MISMATCH") << std::endl;
#endif
            learnedExit[pendingPortalKey] = HERE;
            pendingPortalKey = -1;
        }
        if (trailId != MYID) { trail.clear(); pendingPath.clear(); trailId = MYID; }
        for (int c : pendingPath) trail.push_back(c);
        pendingPath.clear();
        if (trail.empty() || trail.back() != HERE) trail.push_back(HERE);
        if ((int)trail.size() > 2 * LEN + 64) trail.erase(trail.begin(), trail.end() - (LEN + 8));
        receive(ct);
        observe(ct);
#ifdef DEBUG_STATS
        std::clog << "ST " << ROUND << " " << (char)MYTEAM << " " << LEN << " " << MYID << " " << cx(HERE) << " " << cy(HERE) << std::endl;
#endif
#ifdef DEBUG_PORTAL
        bool watch = true;
        std::streambuf* saved = nullptr;
        std::ostringstream cap;
        if (watch) saved = std::cout.rdbuf(cap.rdbuf());
#endif
        takeTurn(ct);
#ifdef DEBUG_PORTAL
        if (watch) {
            std::cout.rdbuf(saved);
            std::cout << cap.str();
            std::string act = cap.str();
            if (!act.empty() && act.back() == '\n') act.pop_back();
            std::cout << "LOG ACT r" << ROUND << " id" << MYID << " at " << cx(HERE) << "," << cy(HERE) << " len" << LEN << " -> " << act;
            for (int d = 0; d < 4; ++d) {
                int q = moveCell(HERE, d);
                std::cout << " | " << DCH[d] << " e" << (int)edgeT[HERE * 4 + d] << " q" << q;
                if (q >= 0) std::cout << "(" << cx(q) << "," << cy(q) << ") occ" << occupied(q) << " mine" << (int)occMine[q] << " id" << occId[q] << " free" << occFree[q];
            }
            std::cout << "\n";
        }
#endif
#if FEAT_SONAR
        uint64_t ordinary = chooseMessage();
        uint64_t crossed = v3CrossedPairMessage();
        uint64_t claim = v3ClaimMessage(activeScoutClaim);
        int base = (ROUND + MYID) & 3;
        for (int d = 0; d < 4; ++d) {
            uint64_t message = ordinary;
            if (crossed && (d == base || (!claim && d == ((base + 2) & 3)))) message = crossed;
            if (claim && (d == ((base + 1) & 3) || d == ((base + 3) & 3) ||
                          (!crossed && d == ((base + 2) & 3)))) message = claim;
            if (message) ct.send_sonar(Direction(DCH[d]), message);
        }
#endif
        end_turn();
    }
}
