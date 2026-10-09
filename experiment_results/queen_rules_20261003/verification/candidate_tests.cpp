#define main bot_program_main
#include "../../../tyhh_test/tyhh_qos_counter_queen_v127/main.cpp"
#undef main
#include <cassert>
#include <iostream>

struct RulesFixture {
    Controller ct{0, Team::A, Direction::EAST, Vision{}, 64};

    void put(int c, int id, bool mine, bool head, int freeAt) {
        occStamp[c] = STAMP; occId[c] = id; occMine[c] = mine; occHead[c] = head;
        occFree[c] = freeAt; occPart[c] = int(parts.size());
        parts.push_back({c, id, 1, mine, head});
    }

    void setup(int length, int id = 0, Team::Value team = Team::A) {
        W = H = 32; NC = W * H; HERE = cid(16, 8); LEN = length; POP = 10;
        MYID = id; MYTEAM = team; ROUND = 0; STAMP = 1; TARGET = 62;
        ct.length = LEN; ct.unit_count = POP; ct.head.dragon_id = MYID;
        ct.head.team = Team(team); ct.head.dir = Direction::EAST; CT = &ct;
        queenSeededScout = false;
        edgeT.assign(NC * 4, E_OPEN); edgePid.assign(NC * 4, -1);
        learnedExit.assign(NC * 4, -1); seenRound.assign(NC, 1);
        pearlDue.assign(NC, INF); pearlInfo.assign(NC, ROUND);
        visStamp.assign(NC, STAMP); occStamp.assign(NC, 0); occId.assign(NC, -1);
        occFree.assign(NC, INF); occPart.assign(NC, -1);
        occMine.assign(NC, 0); occHead.assign(NC, 0); visPearl.assign(NC, 0);
        bfsMark.assign(NC, 0); bfsDist.assign(NC, 0); bfsFirst.assign(NC, -1);
        bfsQ.assign(NC + 8, 0); bfsStamp = 0;
        parts.clear(); idCount.clear(); enemyHeads.clear(); enemyHeadLen.clear();
        friendParts.clear(); visPearls.clear(); friendGuards.clear(); sightings.clear();
        friendStatuses.clear(); portalEnds.clear(); portalKeys.clear(); portalClaimRound.clear();
        occupiedPortalId = -1; trail.clear(); trailId = MYID;
        for (int i = LEN - 1; i >= 0; --i) trail.push_back(cid((16 - i + W) % W, 8));
        for (int i = 0; i < LEN; ++i) put(trail[LEN - 1 - i], MYID, true, i == 0, LEN + 1 - i);
        idCount.push_back({MYID, LEN});
        v41Target = -1; v41TargetPhase = -1; v41ExplorationMode = true;
        v41LowFrontierRounds = 0; teamFrontier = 999; chosenAction = {};
    }

    ActionProjection east(int n) { return projectAction({std::vector<int>(n, 1), 0, false, false}); }
};

int main() {
    RulesFixture f;
    for (int length : {4, 5, 8, 9, 12}) {
        const int free = (length + 3) / 4;
        f.setup(length);
        auto p = f.east(free);
        assert(p.survives && p.known && p.len == length && int(p.body.size()) == length);
        p = f.east(free + 2);
        assert(p.survives && p.len == length - 2 && int(p.body.size()) == length - 2);
        p = f.east(free + length - 2);
        assert(p.survives && p.len == 2 && int(p.body.size()) == 2);
        p = f.east(free + length - 1);
        assert(!p.survives && p.len == 2);
    }
    f.setup(4);
    visPearl[geo(HERE, 1)] = 1;
    auto p = f.east(2);
    assert(p.survives && p.len == 4); // Growth to five does not add a second free step.
    f.setup(8);
    visPearl[geo(HERE, 1)] = 1;
    p = f.east(3);
    assert(p.survives && p.len == 8); // Growth to nine does not add a third free step.
    f.setup(2);
    visPearl[geo(geo(HERE, 1), 1)] = 1;
    p = f.east(2);
    assert(!p.survives && p.head == geo(HERE, 1) && p.len == 2);
    f.setup(2);
    visPearl[geo(HERE, 1)] = 1;
    visPearl[geo(geo(HERE, 1), 1)] = 1;
    p = f.east(2);
    assert(p.survives && p.len == 3 && int(p.body.size()) == 3);
    f.setup(4);
    p = projectAction({});
    assert(!p.survives); // No action is engine suicide, never a surviving projection.

    f.setup(4);
    std::fill(occStamp.begin(), occStamp.end(), 0);
    parts.clear();
    const std::vector<int> bent{cid(16, 8), cid(16, 9), cid(17, 9), cid(18, 9)};
    trail.assign(bent.rbegin(), bent.rend());
    for (int i = 0; i < 4; ++i) f.put(bent[i], MYID, true, i == 0, 5 - i);
    p = projectAction({{1, 2}, 0, false, false});
    assert(!p.survives && p.head == cid(17, 8) && p.len == 4);

    for (auto team : {Team::A, Team::B}) {
        const int id = team == Team::A ? 0 : 1;
        f.setup(2, id, team);
        assert(isQueen() && protectedDragon() && championPolicy());
        assert(!profitableLen2Ram(1, 6));
        f.setup(2, id + 2, team);
        assert(!isQueen() && !protectedDragon());
        assert(profitableLen2Ram(1, 6));
    }

    f.setup(6);
    POP = f.ct.unit_count = 1;
    takeTurn(f.ct);
    assert(chosenAction.split == 2 && queenSeededScout);
    chosenAction = {};
    takeTurn(f.ct);
    assert(chosenAction.split == 0); // Even if population later returns to one, do not bootstrap twice.
    f.setup(6);
    takeTurn(f.ct);
    assert(chosenAction.split == 0); // Queen does not participate in routine swarm splitting.

    f.setup(3);
    setEdge(HERE, 1, E_PORTAL, 7);
    assert(!portalAllowed(HERE, 1));
    const int dest = cid(24, 24);
    learnedExit[HERE * 4 + 1] = dest;
    assert(portalAllowed(HERE, 1));
    visStamp[dest] = 0;
    assert(!portalAllowed(HERE, 1));
    visStamp[dest] = STAMP;
    f.put(dest, 3, false, true, INF);
    assert(!portalAllowed(HERE, 1));
    occStamp[dest] = 0;
    for (int d = 0; d < 4; ++d) edgeT[dest * 4 + d] = E_KELP;
    assert(!portalAllowed(HERE, 1));
    for (int d = 0; d < 4; ++d) edgeT[dest * 4 + d] = E_OPEN;
    std::fill(visStamp.begin(), visStamp.end(), 0);
    visStamp[dest] = visStamp[geo(dest, 1)] = STAMP;
    assert(!portalAllowed(HERE, 1)); // A visible landing plus one continuation is insufficient space.
    f.setup(8, 7);
    setEdge(HERE, 1, E_PORTAL, 7);
    assert(!portalAllowed(HERE, 1)); // The same unknown-portal guard applies to long scouts.
    f.setup(3, 7);
    setEdge(HERE, 1, E_PORTAL, 7);
    assert(portalAllowed(HERE, 1)); // Short scouts retain early exploration.

    std::cout << "candidate projection, queen identity/bootstrap and portal guards passed\n";
}
