// White-box protection regression tests. Include the production translation unit
// with its protocol entry point renamed; the fixtures exercise the same helpers
// used by the engine-facing bot rather than a second model of the policy.
#define main protected_bot_protocol_entry
#include "../../tyhh_test/tyhh_qos_protect_route/main.cpp"
#undef main

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace protection_tests {

std::unique_ptr<Game> fixture_game;
std::unique_ptr<Controller> fixture_controller;
int passed = 0;

void require(bool condition, char const* description) {
    if (!condition) throw std::runtime_error(description);
}

void reset_world(int length = 2, int my_id = 10) {
    W = 12; H = 12; NC = W * H;
    ROUND = 40; LEN = length; POP = 12; MYID = my_id;
    MYTEAM = Team::A; HERE = cid(5, 5); TARGET = 62;
    STAMP = 1; bfsStamp = 0;
    edgeT.assign(NC * 4, E_OPEN);
    edgePid.assign(NC * 4, -1);
    learnedExit.assign(NC * 4, -1);
    seenRound.assign(NC, ROUND);
    pearlDue.assign(NC, INF);
    pearlInfo.assign(NC, -1000);
    visStamp.assign(NC, STAMP);
    occStamp.assign(NC, 0);
    occId.assign(NC, -1);
    occFree.assign(NC, INF);
    occPart.assign(NC, -1);
    occMine.assign(NC, 0);
    occHead.assign(NC, 0);
    visPearl.assign(NC, 0);
    bfsMark.assign(NC, 0);
    bfsDist.assign(NC, 0);
    bfsFirst.assign(NC, -1);
    bfsQ.assign(NC + 8, 0);
    portalEnds.clear(); portalKeys.clear(); portalClaimRound.clear();
    occupiedPortalId = -1; pendingPortalKey = -1;
    trail.clear(); pendingPath.clear(); trailId = MYID;
    sightings.clear(); friendStatuses.clear(); parts.clear(); idCount.clear();
    statusAccepted = statusRejected = 0;
    enemyHeads.clear(); enemyHeadLen.clear(); friendParts.clear();
    visPearls.clear(); sonarCursor = 0;
    friendGuards.clear(); chosenAction = {}; yieldStreak = 0;
    std::fill(safeMoves, safeMoves + 4, false);
    std::fill(moveScores, moveScores + 4, LONG_MIN);
    teamFrontier = 0; teamFrontierRound = -1;
    v41Target = -1; v41TargetPhase = -1;
    v41LowFrontierRounds = 0; v41ExplorationMode = true;
    fixture_game = std::make_unique<Game>(W, H, 64);
    fixture_game->round_num = ROUND;
    unswbc::game = fixture_game.get();
    fixture_controller = std::make_unique<Controller>(
        MYID, Team(MYTEAM), Direction('E'), Vision(), 64);
    fixture_controller->length = LEN;
    fixture_controller->unit_count = POP;
    fixture_controller->head.position = Position(cx(HERE), cy(HERE));
    CT = fixture_controller.get();
    unswbc::ct = fixture_controller.get();
}

void put_part(int cell, int id, bool mine, bool head, int direction, int free_after = INF) {
    occStamp[cell] = STAMP;
    occId[cell] = id; occMine[cell] = mine; occHead[cell] = head;
    occFree[cell] = free_after; occPart[cell] = static_cast<int>(parts.size());
    parts.push_back({cell, id, direction, mine, head});
    bool counted = false;
    for (auto& entry : idCount) if (entry.first == id) { ++entry.second; counted = true; }
    if (!counted) idCount.push_back({id, 1});
    if (mine && id != MYID) friendParts.push_back(cell);
}

void set_visible_length(int id, int length) {
    for (auto& entry : idCount) if (entry.first == id) { entry.second = length; return; }
    idCount.push_back({id, length});
}

void put_own_body() {
    for (int i = LEN - 1; i >= 0; --i) {
        int cell = cid((5 - i + W * 4) % W, 5);
        trail.push_back(cell);
        put_part(cell, MYID, true, i == 0, 1, LEN - i + 1);
    }
}

int put_long_friend(int id = 11, int visible_length = 8, bool unique_exit = true) {
    int head = cid(6, 4);
    put_part(head, id, true, true, 2);
    set_visible_length(id, visible_length);
    if (unique_exit) {
        setEdge(head, 0, E_KELP, -1);
        setEdge(head, 1, E_KELP, -1);
        setEdge(head, 3, E_KELP, -1);
    }
    return head;
}

void run(char const* name, std::function<void()> test) {
    test();
    ++passed;
    std::cout << "PASS " << name << '\n';
}

uint64_t status_from(int id, int round, Team::Value team, int head, int length) {
    int saved_id = MYID, saved_round = ROUND;
    Team::Value saved_team = MYTEAM;
    MYID = id; ROUND = round; MYTEAM = team;
    uint64_t result = packStatus(head, length);
    MYID = saved_id; ROUND = saved_round; MYTEAM = saved_team;
    return result;
}

void status_tests() {
    run("invalid cell sentinels never index occupancy", [] {
        reset_world();
        require(!visible(-1) && !occupied(-1) && !hitsLongFriend(-1), "negative cell must be rejected");
        require(!visible(NC) && !occupied(NC) && !hitsLongFriend(NC), "past-end cell must be rejected");
        FriendStatus invalid{11, -1, 8, ROUND - 1};
        require(!verifiedStatus(invalid), "invalid status head must be rejected");
    });
    run("status obeys round and dragon-ID ordering", [] {
        reset_world();
        FriendStatus decoded;
        int head = cid(6, 4);
        require(decodeStatus(status_from(9, ROUND, Team::A, head, 8), decoded), "smaller ID current round should decode");
        require(decoded.id == 9 && decoded.head == head && decoded.len == 8, "status fields must roundtrip");
        require(decodeStatus(status_from(11, ROUND - 1, Team::A, head, 8), decoded), "larger ID preceding round should decode");
        require(!decodeStatus(status_from(9, ROUND - 1, Team::A, head, 8), decoded), "smaller ID old report is stale");
        require(!decodeStatus(status_from(11, ROUND, Team::A, head, 8), decoded), "larger ID has not acted current round");
        require(!decodeStatus(status_from(11, ROUND - 2, Team::A, head, 8), decoded), "two-round report is stale");
        require(!decodeStatus(status_from(9, ROUND + 1, Team::A, head, 8), decoded), "future report cannot be accepted");
    });
    run("status rejects own enemy and malformed reports", [] {
        reset_world();
        FriendStatus decoded;
        int head = cid(6, 4);
        require(!decodeStatus(status_from(MYID, ROUND, Team::A, head, 8), decoded), "own report must not reserve a route");
        require(!decodeStatus(status_from(11, ROUND - 1, Team::B, head, 8), decoded), "enemy team report must be rejected");
        require(!decodeStatus(status_from(11, ROUND - 1, Team::A, head, 1), decoded), "invalid dragon length must be rejected");
        uint64_t correct = status_from(11, ROUND - 1, Team::A, head, 8);
        require(!decodeStatus(correct ^ (1ull << 63), decoded), "wrong protocol magic must be rejected");
        require(!decodeStatus((correct & ~63ull) | 63ull, decoded), "out-of-map x must be rejected");
        require(packStatus(-1, 8) == 0, "unknown post-action head must not be announced");
    });
    run("reported length requires matching visible friendly head", [] {
        reset_world();
        int head = put_long_friend(11, 1, false);
        friendStatuses.push_back({11, head, 17, ROUND - 1});
        require(friendLength(11) == 17, "verified true length should supersede visible lower bound");
        occMine[head] = 0;
        require(friendLength(11) == 1, "enemy occupancy cannot verify friendly report");
        occMine[head] = 1; occHead[head] = 0;
        require(friendLength(11) == 1, "body cannot verify head report");
        occHead[head] = 1; occId[head] = 12;
        require(friendLength(11) == 1, "different dragon cannot verify report");
        occId[head] = 11; visStamp[head] = 0;
        require(friendLength(11) == 1, "out-of-vision report must not increase confidence");
        visStamp[head] = STAMP; set_visible_length(11, 20);
        require(friendLength(11) == 20, "reported length cannot undercut seen segments");
        require(friendLength(MYID) == LEN, "own true length is authoritative");
    });
    run("status receive coexists with every legacy message type", [] {
        reset_world();
        int pearl = cid(3, 3), enemy = cid(8, 8), portal = cid(9, 4);
        uint32_t pearl_msg = pack(M_PEARL, pearl, 44);
        uint32_t enemy_msg = pack(M_ENEMY, enemy, ROUND | (7 << 9));
        uint32_t portal_msg = pack(M_PORTAL, portal, 23 << 1);
        uint32_t occupied_msg = pack(M_PORTAL_OCCUPIED, HERE, 42);
        uint32_t frontier_msg = pack(M_FRONTIER, HERE, 120);
        fixture_controller->sonar_messages = {pearl_msg, enemy_msg, portal_msg, occupied_msg, frontier_msg,
            status_from(11, ROUND - 1, Team::A, cid(6, 4), 8), pack(5, cid(6, 5), 4095)};
        receive(*fixture_controller);
        require(pearlDue[pearl] == 44, "legacy pearl report must be decoded");
        require(sightings.size() == 1 && sightings[0].c == enemy, "legacy enemy report must be decoded");
        require(edgeT[portal * 4 + 1] == E_PORTAL && edgePid[portal * 4 + 1] == 23, "legacy portal report must be decoded");
        require(portalClaimRound[42] == ROUND && teamFrontier == 120, "legacy claim and frontier reports must be decoded");
        require(friendStatuses.size() == 1, "retired intent packets must not become friend status");
        fixture_controller->sonar_messages.clear();
        receive(*fixture_controller);
        require(friendStatuses.empty(), "status lifetime is one receive cycle");
    });
    run("long dragons keep the legacy broadcaster active", [] {
        reset_world(12);
        enemyHeads.push_back(cid(8, 8)); enemyHeadLen.push_back(7);
        require(((chooseMessage() >> 25) & 7) == M_ENEMY, "long-dragon status must not preempt enemy report");
        enemyHeads.clear(); enemyHeadLen.clear();
        occupiedPortalId = 7;
        require(((chooseMessage() >> 25) & 7) == M_PORTAL_OCCUPIED, "room claim must keep its priority");
        occupiedPortalId = -1;
        require(((chooseMessage() >> 25) & 7) == M_FRONTIER, "frontier report must remain reachable");
    });
}


void action_tests() {
    run("unique exit is strong while alternate exits are soft", [] {
        reset_world(); put_own_body(); put_long_friend(); prepareFriendProtection();
        require(actionRisk({{1},0,true,false}).strong == 1, "must protect the sole exit");
        require(actionRisk({{2},0,true,false}).strong == 0, "unrelated direction remains available");
        reset_world(); put_own_body(); put_long_friend(11,8,false); prepareFriendProtection();
        auto risk=actionRisk({{1},0,true,false});
        require(risk.strong == 0 && risk.soft == 48, "multiple exits imply bounded preference only");
    });
    run("unknown portal counts as an alternative without a fictional cell", [] {
        reset_world(); put_own_body(); int head=put_long_friend();
        setEdge(head,0,E_PORTAL,77); prepareFriendProtection();
        require(friendGuards.size()==1 && friendGuards[0].unknown==1, "unknown portal must remain possible");
        require(actionRisk({{1},0,true,false}).strong==0, "known exit is not unique when portal unresolved");
        setEdge(HERE,1,E_PORTAL,78); chosenAction={{1},0,false,false};
        reviewProtection(*CT);
        require(chosenAction.dirs==std::vector<int>{1}, "unknown portal exploration stays intact");
        require(!projectAction(chosenAction).known, "unknown landing cannot generate exact status");
    });
    run("guard thresholds preserve early short-dragon expansion", [] {
        reset_world(2); put_own_body(); put_long_friend(11,4); prepareFriendProtection();
        require(friendGuards.empty(), "ordinary length-4 teammate should not suppress exploration");
        reset_world(5); put_own_body(); put_long_friend(11,8); prepareFriendProtection();
        require(friendGuards.size()==1, "medium dragon protects clearly longer friend");
        reset_world(7); put_own_body(); put_long_friend(11,8); prepareFriendProtection();
        require(friendGuards.empty(), "one-segment gap should not impose route yielding");
    });
    run("sprint intermediate body blocks a friend's exit", [] {
        reset_world(3); put_own_body(); put_long_friend(); prepareFriendProtection();
        ChosenAction sprint{{1,2},0,false,false}; auto p=projectAction(sprint);
        require(p.survives && p.len==2 && p.head==cid(6,6), "two-step projection should survive with paid length");
        require(actionRisk(sprint).strong==1, "must inspect intermediate body cell, not only endpoint");
    });
    run("split extends tail occupancy while moving can release it", [] {
        reset_world(4); put_own_body(); int head=cid(2,4);
        put_part(head,11,true,true,2); set_visible_length(11,8);
        for(int d:{0,1,3}) setEdge(head,d,E_KELP,-1);
        prepareFriendProtection();
        require(friendGuards.size()==1, "own tail must be considered an exit we can release");
        require(actionRisk({{},2,false,false}).strong==1, "split holds the exit occupied");
        require(actionRisk({{1},0,false,false}).strong==0, "non-eating move releases that exit");
    });
    run("known portal checks its actual remote landing", [] {
        reset_world(3); put_own_body(); setEdge(HERE,1,E_PORTAL,5); setEdge(cid(9,8),1,E_PORTAL,5);
        int head=cid(10,7); put_part(head,11,true,true,2); set_visible_length(11,8);
        for(int d:{0,1,3}) setEdge(head,d,E_KELP,-1);
        prepareFriendProtection();
        require(moveCell(HERE,1)==cid(10,8), "portal fixture must use remote exit");
        require(actionRisk({{1},0,false,false}).strong==1, "protect real portal landing");
    });
    run("projection accounts for growth sprint cost and split length", [] {
        reset_world(3); put_own_body();
        require(projectAction({{1,1},0,false,false}).len==2, "sprint consumes one segment");
        visPearl[cid(6,5)]=1;
        require(projectAction({{1},0,false,false}).len==4, "ordinary pearl grows one");
        require(projectAction({{1,1},0,false,false}).len==3, "sprint plus pearl nets zero growth");
        reset_world(2); put_own_body(); visPearl[cid(6,5)]=1;
        require(projectAction({{1,1},0,false,false}).survives, "first pearl can pay later sprint");
        visPearl[cid(6,5)]=0; visPearl[cid(7,5)]=1;
        require(!projectAction({{1,1},0,false,false}).survives, "later pearl cannot prepay sprint");
        reset_world(4); put_own_body();
        require(projectAction({{},2,false,false}).len==2, "status reports parent length after split");
    });
    run("sprint collision precedes paid-tail release", [] {
        reset_world(4); HERE=cid(2,2);
        std::vector<int> body={cid(2,2),cid(2,3),cid(3,3),cid(4,3)};
        for(int i=3;i>=0;--i) { trail.push_back(body[i]); put_part(body[i],MYID,true,i==0,1,5-i); }
        require(!projectAction({{1,2},0,false,false}).survives, "actual engine reports self collision at second step");
    });
    run("strong exit conflict yields to a safe scored alternative", [] {
        reset_world(); put_own_body(); put_long_friend(); prepareFriendProtection();
        chosenAction={{1},0,true,false}; moveScores[1]=500; moveScores[2]=100; safeMoves[2]=true;
        reviewProtection(*CT);
        require(chosenAction.dirs==std::vector<int>{2} && yieldStreak==1, "must select viable unblocked direction");
    });
    run("no safe alternative retains a physically valid proposed move", [] {
        reset_world(); put_own_body(); put_long_friend(); prepareFriendProtection();
        chosenAction={{1},0,true,false}; moveScores[1]=500;
        reviewProtection(*CT);
        require(chosenAction.dirs==std::vector<int>{1}, "must not force death for a route conflict");
    });
    run("weak yielding is bounded and releases after two consecutive yields", [] {
        reset_world(); put_own_body(); put_long_friend(11,8,false); prepareFriendProtection();
        chosenAction={{1},0,true,false}; moveScores[1]=500; moveScores[2]=100; safeMoves[2]=true;
        reviewProtection(*CT); require(chosenAction.dirs==std::vector<int>{1}, "large goal loss outweighs weak hint");
        chosenAction={{1},0,true,false}; moveScores[2]=480;
        reviewProtection(*CT); require(chosenAction.dirs==std::vector<int>{2}, "small detour can respect weak hint");
        yieldStreak=2; chosenAction={{1},0,true,false};
        reviewProtection(*CT); require(chosenAction.dirs==std::vector<int>{1}, "third weak yield must not form a loop");
    });
    run("direct friendly head collision retains emergency legal split", [] {
        reset_world(4); put_own_body();
        put_part(cid(6,5),11,true,true,2);set_visible_length(11,8);
        setEdge(HERE,0,E_KELP,-1);setEdge(HERE,2,E_KELP,-1);
        prepareFriendProtection(); chosenAction={{1},0,false,false};
        reviewProtection(*CT);
        require(chosenAction.split==2, "legal escape split must not be discarded");
    });
    run("all sprint steps check direct friendly collisions", [] {
        reset_world(3); put_own_body();
        put_part(cid(7,5),11,true,true,2);set_visible_length(11,8);
        require(directFriendCollision({{1,1},0,false,false}), "second sprint step can hit friendly head");
        occHead[cid(7,5)]=0;
        require(directFriendCollision({{1,1},0,false,false}), "long friend's body must also be checked");
        require(!directFriendCollision({{1,1},0,false,true}), "intentional body donation is distinct from head collision");
    });
}

} // namespace protection_tests

int main() {
    using namespace protection_tests;
    try {
        status_tests();
        action_tests();
        std::cout << "PASS " << passed << " protection regression cases\n";
        return 0;
    } catch (std::exception const& error) {
        std::cerr << "FAIL after " << passed << " cases: " << error.what() << '\n';
        return 1;
    }
}
