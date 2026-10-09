#pragma once

// Feeding is decided from current visible geometry, never stale queen position.
int queenFeedObserver = -1;
int queenFeedFirstRound = -1;
int queenFeedLastEnemy = -1;

void queenFeedObserve() {
    if (queenFeedObserver != MYID) {
        queenFeedObserver = MYID;
        queenFeedFirstRound = queenFeedLastEnemy = ROUND;
    }
    for (auto const& p : parts) if (!p.mine) queenFeedLastEnemy = ROUND;
    for (auto const& s : sightings) queenFeedLastEnemy = std::max(queenFeedLastEnemy, s.round);
}

int queenFeedKnownStep(int c, int d) {
    uint8_t edge = edgeT[c * 4 + d];
    if (edge != E_OPEN && edge != E_PORTAL) return -1;
    int q = moveCell(c, d);
    return q >= 0 && visible(q) ? q : -1;
}

bool queenFeedVacantAfterDeath(int c) {
    return !occupied(c) || (occMine[c] && occId[c] == MYID);
}

// The queen can collect the first guaranteed pearl on the donor's old head.
// A distinct continuation prevents approving a two-cell cul-de-sac.
int queenFeedCollectionDistance(int queenHead) {
    std::vector<int> path{queenHead};
    auto search = [&](auto&& self, int c, int depth) -> int {
        if (depth >= 2) return INF;
        int result = INF;
        for (int d = 0; d < 4; ++d) {
            int q = queenFeedKnownStep(c, d);
            if (q < 0 || !queenFeedVacantAfterDeath(q) ||
                std::find(path.begin(), path.end(), q) != path.end()) continue;
            path.push_back(q);
            if (q == HERE) {
                for (int next = 0; next < 4; ++next) {
                    int r = queenFeedKnownStep(q, next);
                    if (r >= 0 && queenFeedVacantAfterDeath(r) &&
                        std::find(path.begin(), path.end(), r) == path.end())
                        result = std::min(result, depth + 1);
                }
            } else {
                result = std::min(result, self(self, q, depth + 1));
            }
            path.pop_back();
        }
        return result;
    };
    return search(search, queenHead, 0);
}

bool queenFeedCompetitorReaches(int start, int distance) {
    std::vector<int> queue{start}, depth(NC, -1);
    depth[start] = 0;
    for (size_t pos = 0; pos < queue.size(); ++pos) {
        int c = queue[pos];
        if (c == HERE) return true;
        if (depth[c] >= distance) continue;
        for (int d = 0; d < 4; ++d) {
            int q = queenFeedKnownStep(c, d);
            if (q < 0 || depth[q] >= 0 || !queenFeedVacantAfterDeath(q)) continue;
            depth[q] = depth[c] + 1;
            queue.push_back(q);
        }
    }
    return false;
}

bool queenFeedAction(Controller& ct) {
    // Population four leaves at least three survivors. Newborns need their
    // own eight quiet observations, rather than inheriting a guessed history.
    if (isQueen() || ROUND < 280 || POP < 4 || LEN > 24 ||
        queenFeedFirstRound < 0 || ROUND - queenFeedFirstRound < 8 ||
        ROUND - queenFeedLastEnemy < 8) return false;
    auto queen = std::find_if(parts.begin(), parts.end(), [](Part const& p) {
        return p.mine && p.head && p.id == queenId();
    });
    if (queen == parts.end() || !visible(queen->c) || !occupied(queen->c) ||
        !occMine[queen->c] || !occHead[queen->c] || occId[queen->c] != queenId()) return false;
    int distance = queenFeedCollectionDistance(queen->c);
    // A short queen may collect one pearl per turn with gaps between pearls.
    if (distance > 2 || Constants::MAX_ROUNDS - ROUND < distance + LEN + 6) return false;
    for (auto const& p : parts) {
        if (!p.head || p.id == MYID || (p.mine && p.id == queenId())) continue;
        if (queenFeedCompetitorReaches(p.c, distance)) return false;
    }
    chosenAction = {{}, 0, false, true};
    ct.output_log("QUEEN_FEED", ROUND, MYID, "donor_len", LEN,
                  "queen", queenId(), "queen_len_lower_bound", friendLength(queenId()),
                  "collection_distance", distance);
    return true;
}
