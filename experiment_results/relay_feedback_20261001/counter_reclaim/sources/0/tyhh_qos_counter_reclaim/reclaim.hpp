// Included inside main.cpp's anonymous namespace after action helpers.
// H3: conservative, observation-only corpse races and actual pearl recovery.
struct ReclaimOldPart { int c, id; bool mine, hadPearl; int nextDue; };
std::vector<ReclaimOldPart> reclaimOldParts;
std::vector<int> reclaimUntil;
int reclaimObserver = -1, reclaimRound = -1;
int reclaimRefused = 0, reclaimDetected = 0, reclaimCollected = 0;

// Unknown map edges and unresolved portal exits never establish reachability.
int reclaimKnownStep(int c, int d) {
    uint8_t t = edgeT[c * 4 + d];
    if (t != E_OPEN && t != E_PORTAL) return -1;
    return moveCell(c, d);
}

std::vector<int> reclaimRaceDistances(int start, int victimId) {
    std::vector<int> distance(NC, INF), queue;
    distance[start] = 0; queue.push_back(start);
    for (size_t i = 0; i < queue.size(); ++i) {
        int p = queue[i];
        if (distance[p] >= 6) continue;
        for (int d = 0; d < 4; ++d) {
            int q = reclaimKnownStep(p, d);
            if (q < 0 || distance[q] < INF) continue;
            // Both trading dragons disappear. Other bodies do not.
            if (occupied(q) && occId[q] != MYID && occId[q] != victimId) continue;
            distance[q] = distance[p] + 1;
            queue.push_back(q);
        }
    }
    return distance;
}

struct ReclaimRace { int friendly = 0, enemy = 0; bool allowed = true; };
ReclaimRace reclaimTradeRace(int enemyHead, int firstSprintCell = -1) {
    ReclaimRace race;
    if (!isEnemyHead(enemyHead)) return race;
    int victimId = occId[enemyHead];
    std::vector<int> friendly(NC, INF), enemy(NC, INF);
    bool hasThird = false;
    for (auto const& p : parts) {
        if (!p.head || p.id == MYID || p.id == victimId) continue;
        hasThird = true;
        auto distance = reclaimRaceDistances(p.c, victimId);
        auto& best = p.mine ? friendly : enemy;
        for (int c = 0; c < NC; ++c) best[c] = std::min(best[c], distance[c]);
    }
    if (!hasThird) return race;

    // Exact own trail where known; the enemy chain is reconstructed backwards
    // from the visible head. Missing chain links are not invented.
    std::vector<int> ownBody;
    for (int j = 0; j < LEN; ++j) ownBody.push_back(ownSeg(LEN - 1 - j));
    if (!ownBody.empty()) ownBody[0] = HERE;
    if (firstSprintCell >= 0) {
        ownBody.insert(ownBody.begin(), firstSprintCell);
        if (!(visible(firstSprintCell) && visPearl[firstSprintCell])) ownBody.pop_back();
    }
    std::vector<int> pearlCells;
    for (size_t j = 0; j < ownBody.size(); j += 2)
        if (ownBody[j] >= 0) pearlCells.push_back(ownBody[j]);
    int cell = enemyHead;
    std::vector<int> chain;
    for (size_t j = 0; j < parts.size(); ++j) {
        if (cell < 0 || std::find(chain.begin(), chain.end(), cell) != chain.end()) break;
        chain.push_back(cell);
        if (!(j & 1)) pearlCells.push_back(cell);
        int back = -1;
        for (auto const& p : parts)
            if (p.id == victimId && !p.head && reclaimKnownStep(p.c, p.facing) == cell) {
                back = p.c; break;
            }
        cell = back;
    }
    for (int c : pearlCells) {
        int fd = friendly[c], ed = enemy[c];
        if (fd >= INF && ed >= INF) continue;
        if (fd < ed) race.friendly += 2;
        else if (ed < fd) race.enemy += 2;
        else { ++race.friendly; ++race.enemy; }
    }
    race.allowed = race.enemy <= race.friendly;
    return race;
}

bool reclaimAllowsTrade(int enemyHead, int firstSprintCell = -1) {
    auto race = reclaimTradeRace(enemyHead, firstSprintCell);
    if (!race.allowed) {
        ++reclaimRefused;
        if (CT) CT->output_log("RECLAIM_REJECT", ROUND, MYID, "target", occId[enemyHead],
                               "friend", race.friendly, "enemy", race.enemy);
    }
    return race.allowed;
}

void reclaimObserve() {
    if (reclaimObserver != MYID || (int)reclaimUntil.size() != NC) {
        reclaimOldParts.clear(); reclaimUntil.assign(NC, -1);
        reclaimObserver = MYID; reclaimRound = -1;
    }
    for (int c = 0; c < NC; ++c)
        if (reclaimUntil[c] < ROUND || (visible(c) && !visPearl[c])) reclaimUntil[c] = -1;
    if (reclaimRound == ROUND - 1) {
        for (auto const& old : reclaimOldParts) {
            // A split/move/out-of-view event alone is not a corpse signal.
            if (old.hadPearl || old.nextDue <= ROUND || !visible(old.c) || !visPearl[old.c] || occupied(old.c)) continue;
            bool stillSeen = false;
            for (auto const& p : parts)
                if (p.id == old.id && p.mine == old.mine) { stillSeen = true; break; }
            if (stillSeen) continue;
            reclaimUntil[old.c] = ROUND + 8;
            ++reclaimDetected;
            if (CT) CT->output_log("RECLAIM_PEARL", ROUND, MYID, "cell", old.c, "old_id", old.id);
        }
    }
    reclaimOldParts.clear();
    for (auto const& p : parts)
        if (visible(p.c)) reclaimOldParts.push_back({p.c, p.id, p.mine, bool(visPearl[p.c]), pearlDue[p.c]});
    reclaimRound = ROUND;
}

// A recovery route may not cross a cell a visible enemy can hit within this
// turn (one move, or the existing two-move sprint with sufficient length).
std::vector<uint8_t> reclaimThreats() {
    std::vector<uint8_t> threat(NC, 0);
    for (size_t i = 0; i < enemyHeads.size(); ++i) {
        int h = enemyHeads[i]; threat[h] = 1;
        for (int d = 0; d < 4; ++d) {
            int m = reclaimKnownStep(h, d);
            if (m < 0 || occupied(m)) continue;
            threat[m] = 1;
            // Visible segment counts are only lower bounds: even a head with
            // one/two observed segments might be long enough to sprint.
            for (int d2 = 0; d2 < 4; ++d2) {
                int q = reclaimKnownStep(m, d2);
                if (q >= 0 && !occupied(q)) threat[q] = 1;
            }
        }
    }
    return threat;
}

bool reclaimCollect() {
    if ((int)reclaimUntil.size() != NC) return false;
    bool any = false;
    for (int c : visPearls)
        if (reclaimUntil[c] >= ROUND && !occupied(c)) { any = true; break; }
    if (!any) return false;
    auto threat = reclaimThreats();
    std::vector<int> distance(NC, INF), first(NC, -1), queue;
    distance[HERE] = 0; queue.push_back(HERE);
    for (size_t i = 0; i < queue.size(); ++i) {
        int p = queue[i];
        if (distance[p] >= 6) continue;
        for (int d = 0; d < 4; ++d) {
            int q = reclaimKnownStep(p, d);
            if (q < 0 || !visible(q) || occupied(q) || threat[q] || distance[q] < INF) continue;
            int k = distance[p] + 1;
            // A goal reached at k cannot rely on revisiting its new body
            // immediately. Require an observed free onward edge.
            bool onward = false;
            for (int nd = 0; nd < 4; ++nd) {
                int r = reclaimKnownStep(q, nd);
                if (r >= 0 && r != p && visible(r) && !occupied(r) && !threat[r]) onward = true;
            }
            if (!onward) continue;
            distance[q] = k; first[q] = p == HERE ? d : first[p];
            queue.push_back(q);
            if (reclaimUntil[q] < ROUND || !visPearl[q]) continue;
            int direction = first[q], dest = moveCell(HERE, direction);
            if (dest < 0 || occupied(dest) || threat[dest]) continue;
            ChosenAction proposal{{direction}, 0, false, false};
            auto projection = projectAction(proposal);
            if (!projection.known || !projection.survives || directFriendCollision(proposal)) continue;
            if (actionRisk(proposal).strong) continue;
            if (spaceFrom(dest, visPearl[dest], 90, true) < std::min(LEN + 1, 8)) continue;
            doMove(direction);
            ++reclaimCollected;
            if (CT) CT->output_log("RECLAIM_COLLECT", ROUND, MYID, "goal", q, "distance", k);
            return true;
        }
    }
    return false;
}
