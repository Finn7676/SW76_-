#pragma once

std::vector<int> reservedPearls;

bool pearlReserveQuiet() {
    return !isQueen() && ROUND >= 350 && LEN >= 2 && LEN <= 6 && POP >= 4 &&
           queenFeedFirstRound >= 0 && ROUND - queenFeedFirstRound >= 8 &&
           ROUND - queenFeedLastEnemy >= 8 &&
           std::none_of(parts.begin(), parts.end(), [](Part const& p) { return !p.mine; });
}

bool pearlReserveRecipientReaches(int start, int target) {
    std::vector<int> path{start};
    auto walk = [&](auto&& self, int c, int depth) -> bool {
        if (depth >= 2) return false;
        for (int d = 0; d < 4; ++d) {
            int q = queenFeedKnownStep(c, d);
            if (q < 0 || occupied(q) ||
                std::find(path.begin(), path.end(), q) != path.end()) continue;
            path.push_back(q);
            bool reaches = false;
            if (q == target) {
                for (int e = 0; e < 4; ++e) {
                    int next = queenFeedKnownStep(q, e);
                    if (next >= 0 && !occupied(next) &&
                        std::find(path.begin(), path.end(), next) == path.end()) reaches = true;
                }
            } else reaches = self(self, q, depth + 1);
            path.pop_back();
            if (reaches) return true;
        }
        return false;
    };
    return walk(walk, start, 0);
}

bool pearlReserveNonPearlEscape() {
    for (int d = 0; d < 4; ++d) {
        int q = queenFeedKnownStep(HERE, d);
        if (q < 0 || visPearl[q] || occupied(q) || !portalAllowed(HERE, d)) continue;
        ChosenAction action{{d}, 0, false, false};
        auto post = projectAction(action);
        if (post.known && post.survives && projectedEscape(post) &&
            !directFriendCollision(action) && !actionRisk(action).strong) return true;
    }
    return false;
}

void pearlReservePrepare() {
    reservedPearls.clear();
    if (!pearlReserveQuiet() || !pearlReserveNonPearlEscape()) return;
    for (int c : visPearls) {
        if (!visible(c) || !visPearl[c] || occupied(c)) continue;
        for (auto const& receiver : parts) {
            if (!receiver.mine || !receiver.head || receiver.id == MYID ||
                !visible(receiver.c) || !occupied(receiver.c) || !occMine[receiver.c] ||
                !occHead[receiver.c] || occId[receiver.c] != receiver.id) continue;
            int length = friendLength(receiver.id);
            if (receiver.id != queenId() && length < std::max(8, LEN + 2)) continue;
            if (pearlReserveRecipientReaches(receiver.c, c)) {
                reservedPearls.push_back(c);
                break;
            }
        }
    }
}

bool pearlReserved(int c) {
    return std::find(reservedPearls.begin(), reservedPearls.end(), c) != reservedPearls.end();
}

bool pearlReserveTakes(ChosenAction const& a) {
    int c = HERE;
    for (int d : a.dirs) {
        c = moveCell(c, d);
        if (c < 0) return false;
        if (pearlReserved(c)) return true;
    }
    return false;
}
