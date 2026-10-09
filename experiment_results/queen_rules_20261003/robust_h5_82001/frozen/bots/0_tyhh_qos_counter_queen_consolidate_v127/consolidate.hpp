#pragma once

int consolidateObserver = -1, consolidateHoldUntil = -1;

bool consolidateQueenVisible() {
    for (auto const& p : parts)
        if (p.mine && p.id == queenId() && visible(p.c)) return true;
    return false;
}

bool consolidateQuiet() {
    return ROUND >= 350 && POP >= 4 && !consolidateQueenVisible() &&
           queenFeedFirstRound >= 0 && ROUND - queenFeedFirstRound >= 8 &&
           ROUND - queenFeedLastEnemy >= 8;
}

bool consolidateVacant(int c, int donor) {
    return !occupied(c) || (occMine[c] && occId[c] == donor);
}

int consolidatePickup(int receiverHead, int donorHead, int donor) {
    std::vector<int> path{receiverHead};
    auto search = [&](auto&& self, int c, int depth) -> int {
        if (depth >= 2) return INF;
        int result = INF;
        for (int d = 0; d < 4; ++d) {
            int q = queenFeedKnownStep(c, d);
            if (q < 0 || !consolidateVacant(q, donor) ||
                std::find(path.begin(), path.end(), q) != path.end()) continue;
            path.push_back(q);
            if (q == donorHead) {
                for (int next = 0; next < 4; ++next) {
                    int r = queenFeedKnownStep(q, next);
                    if (r >= 0 && consolidateVacant(r, donor) &&
                        std::find(path.begin(), path.end(), r) == path.end())
                        result = std::min(result, depth + 1);
                }
            } else result = std::min(result, self(self, q, depth + 1));
            path.pop_back();
        }
        return result;
    };
    return search(search, receiverHead, 0);
}

bool consolidateCompetitor(int start, int target, int donor, int limit) {
    std::vector<int> queue{start}, depth(NC, -1);
    depth[start] = 0;
    for (size_t pos = 0; pos < queue.size(); ++pos) {
        int c = queue[pos];
        if (c == target) return true;
        if (depth[c] >= limit) continue;
        for (int d = 0; d < 4; ++d) {
            int q = queenFeedKnownStep(c, d);
            if (q < 0 || depth[q] >= 0 || !consolidateVacant(q, donor)) continue;
            depth[q] = depth[c] + 1; queue.push_back(q);
        }
    }
    return false;
}

int consolidateReceiver(int donor, int donorHead, int donorLength) {
    int best = -1, bestLength = -1;
    for (auto const& p : parts) {
        if (!p.mine || !p.head || p.id == donor || p.id == queenId()) continue;
        int length = friendLength(p.id);
        if (length < donorLength || (length == donorLength && p.id > donor)) continue;
        int distance = consolidatePickup(p.c, donorHead, donor);
        if (distance > 2 || Constants::MAX_ROUNDS - ROUND < distance + donorLength + 6) continue;
        bool raced = false;
        for (auto const& rival : parts) {
            if (!rival.head || rival.id == donor || rival.id == p.id) continue;
            if (consolidateCompetitor(rival.c, donorHead, donor, distance)) raced = true;
        }
        if (raced) continue;
        if (length > bestLength || (length == bestLength && (best < 0 || p.id < best))) {
            best = p.id; bestLength = length;
        }
    }
    return best;
}

void consolidateObserve() {
    if (consolidateObserver != MYID) {
        consolidateObserver = MYID; consolidateHoldUntil = -1;
    }
    if (isQueen() || !consolidateQuiet()) return;
    for (auto const& donor : parts) {
        if (!donor.mine || !donor.head || donor.id == MYID || donor.id == queenId()) continue;
        int lowerLength = friendLength(donor.id);
        if (lowerLength < 2 || lowerLength > 24) continue;
        if (consolidateReceiver(donor.id, donor.c, lowerLength) == MYID)
            consolidateHoldUntil = ROUND + 12;
    }
}

bool consolidateHolding() {
    return !isQueen() && ROUND >= 350 && ROUND <= consolidateHoldUntil;
}

bool consolidateAction(Controller& ct) {
    if (isQueen() || LEN > 24 || !consolidateQuiet()) return false;
    int receiver = consolidateReceiver(MYID, HERE, LEN);
    if (receiver < 0) return false;
    chosenAction = {{}, 0, false, true};
    ct.output_log("CONSOLIDATE_FEED", ROUND, MYID, "receiver", receiver, "donor_len", LEN);
    return true;
}
