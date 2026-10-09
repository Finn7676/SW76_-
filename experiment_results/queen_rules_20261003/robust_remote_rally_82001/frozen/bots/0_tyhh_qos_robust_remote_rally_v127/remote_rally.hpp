#pragma once

// These reports are navigation cues, never current occupancy or donation proof.
std::map<int, FriendStatus> remoteRallies;
std::map<int, int> remoteRallyIgnoreUntil;
int remoteRallyObserver = -1, remoteRallyTarget = -1;
int remoteRallyBestDistance = INF, remoteRallyProgressRound = -1;

bool remoteRallyHeadMatches(FriendStatus const& s) {
    return !visible(s.head) || (occupied(s.head) && occMine[s.head] &&
           occHead[s.head] && occId[s.head] == s.id);
}

void remoteRallyObserve() {
    if (remoteRallyObserver != MYID) {
        remoteRallyObserver = MYID;
        remoteRallies.clear(); remoteRallyIgnoreUntil.clear();
        remoteRallyTarget = -1; remoteRallyBestDistance = INF;
        remoteRallyProgressRound = -1;
    }
    for (auto const& s : friendStatuses) {
        if (s.id < 0 || s.id == MYID || s.id == queenId() || s.head < 0 ||
            s.head >= NC || s.len < 2 ||
            s.round != ROUND - (s.id > MYID ? 1 : 0) || !remoteRallyHeadMatches(s)) continue;
        auto previous = remoteRallies.find(s.id);
        if (previous == remoteRallies.end() || previous->second.round <= s.round)
            remoteRallies[s.id] = s;
    }
    for (auto it = remoteRallies.begin(); it != remoteRallies.end();) {
        if (it->second.round > ROUND || ROUND - it->second.round > 6 ||
            !remoteRallyHeadMatches(it->second)) it = remoteRallies.erase(it);
        else ++it;
    }
    for (auto it = remoteRallyIgnoreUntil.begin(); it != remoteRallyIgnoreUntil.end();) {
        if (ROUND >= it->second) it = remoteRallyIgnoreUntil.erase(it);
        else ++it;
    }
}

struct RemoteRallyRoute {
    int direction = -1, receiver = -1, distance = INF, length = -1;
};

RemoteRallyRoute remoteRallyRoute() {
    RemoteRallyRoute result;
    if (isQueen() || LEN < 2 || LEN > 24 || !consolidateQuiet() ||
        consolidateReceiver(MYID, HERE, LEN) >= 0) return result;
    for (auto const& p : parts) if (!p.mine) return result;
    for (int i = 0; i < LEN; ++i) if (ownSeg(i) < 0) return result;

    // A refreshed report cannot postpone a stalled pursuit indefinitely.
    if (remoteRallyTarget >= 0 && remoteRallyProgressRound >= 0 &&
        ROUND - remoteRallyProgressRound >= 6) {
        remoteRallyIgnoreUntil[remoteRallyTarget] = ROUND + 12;
        remoteRallyTarget = -1; remoteRallyBestDistance = INF;
        remoteRallyProgressRound = -1;
    }

    std::vector<FriendStatus> candidates;
    for (auto const& [id, s] : remoteRallies) {
        if (id >= MYID || id == queenId() || s.head < 0 || s.head >= NC ||
            s.round > ROUND || ROUND - s.round > 6 ||
            !remoteRallyHeadMatches(s) || remoteRallyIgnoreUntil.count(id)) continue;
        int length = visible(s.head) ? friendLength(id) : s.len;
        if (length < 8 || length < LEN + 2) continue;
        candidates.push_back({id, s.head, length, s.round});
    }
    if (candidates.empty()) return result;

    std::vector<int> queue{HERE}, depth(NC, -1), first(NC, -1);
    depth[HERE] = 0;
    int expanded = 0;
    for (size_t pos = 0; pos < queue.size() && expanded < 256; ++pos) {
        int c = queue[pos];
        ++expanded;
        if (depth[c] > 0) {
            for (auto const& s : candidates) {
                bool goal = c == s.head;
                for (int d = 0; d < 4 && !goal; ++d) {
                    uint8_t edge = edgeT[s.head * 4 + d];
                    if ((edge == E_OPEN || edge == E_PORTAL) && moveCell(s.head, d) == c)
                        goal = true;
                }
                if (!goal || Constants::MAX_ROUNDS - ROUND < depth[c] + LEN + 8) continue;
                if (s.len > result.length || (s.len == result.length &&
                    (depth[c] < result.distance || (depth[c] == result.distance && s.id < result.receiver))))
                    result = {first[c], s.id, depth[c], s.len};
            }
        }
        if (depth[c] >= 12) continue;
        for (int d = 0; d < 4; ++d) {
            uint8_t edge = edgeT[c * 4 + d];
            if ((edge != E_OPEN && edge != E_PORTAL) || !portalAllowed(c, d)) continue;
            int q = moveCell(c, d);
            if (q < 0 || depth[q] >= 0 || (visible(q) && occupied(q))) continue;
            if (c == HERE) {
                if (!visible(q)) continue;
                ChosenAction step{{d}, 0, false, false};
                auto projected = projectAction(step);
                auto risk = actionRisk(step);
                if (!projected.known || !projected.survives || directFriendCollision(step) ||
                    risk.strong || risk.soft || !projectedEscape(projected)) continue;
            }
            depth[q] = depth[c] + 1;
            first[q] = c == HERE ? d : first[c];
            queue.push_back(q);
        }
    }
    if (result.direction >= 0) {
        if (remoteRallyTarget != result.receiver || result.distance < remoteRallyBestDistance) {
            remoteRallyProgressRound = ROUND;
            remoteRallyBestDistance = result.distance;
        }
        remoteRallyTarget = result.receiver;
    }
    return result;
}

bool remoteRallyAction(Controller& ct) {
    auto route = remoteRallyRoute();
    if (route.direction < 0) return false;
    doMove(route.direction);
    ct.output_log("REMOTE_RALLY", ROUND, MYID, "receiver", route.receiver,
                  "donor_len", LEN, "approach_distance", route.distance);
    return true;
}
