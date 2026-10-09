#pragma once

struct QueenRoute {
    int direction = -1, distance = INF;
};

QueenRoute queenRoute() {
    QueenRoute result;
    if (isQueen() || ROUND < 280 || POP < 4 || LEN > 24 ||
        queenFeedFirstRound < 0 || ROUND - queenFeedFirstRound < 8 ||
        ROUND - queenFeedLastEnemy < 8) return result;
    for (auto const& p : parts) if (!p.mine) return result;
    auto queen = std::find_if(parts.begin(), parts.end(), [](Part const& p) {
        return p.mine && p.head && p.id == queenId();
    });
    if (queen == parts.end() || !visible(queen->c) || !occupied(queen->c) ||
        !occMine[queen->c] || !occHead[queen->c] || occId[queen->c] != queenId() ||
        queenFeedCollectionDistance(queen->c) <= 2) return result;

    std::vector<int> queue{HERE}, depth(NC, -1), first(NC, -1);
    depth[HERE] = 0;
    int expanded = 0;
    for (size_t pos = 0; pos < queue.size() && expanded < 128; ++pos) {
        int c = queue[pos];
        ++expanded;
        if (depth[c] > 0) {
            int pickup = consolidatePickup(queen->c, c, MYID);
            if (pickup <= 2 && Constants::MAX_ROUNDS - ROUND >=
                depth[c] + pickup + LEN + 6) return {first[c], depth[c]};
        }
        if (depth[c] >= 6) continue;
        for (int d = 0; d < 4; ++d) {
            if (edgeT[c * 4 + d] != E_OPEN) continue;
            int q = geo(c, d);
            if (!visible(q) || occupied(q) || depth[q] >= 0) continue;
            if (c == HERE) {
                ChosenAction step{{d}, 0, false, false};
                auto projected = projectAction(step);
                auto risk = actionRisk(step);
                if (!projected.known || !projected.survives ||
                    directFriendCollision(step) || risk.strong || risk.soft ||
                    !projectedEscape(projected)) continue;
            }
            depth[q] = depth[c] + 1;
            first[q] = c == HERE ? d : first[c];
            queue.push_back(q);
        }
    }
    return result;
}

bool queenRouteAction(Controller& ct) {
    auto route = queenRoute();
    if (route.direction < 0) return false;
    doMove(route.direction);
    ct.output_log("QUEEN_ROUTE", ROUND, MYID, "queen", queenId(),
                  "donor_len", LEN, "approach_distance", route.distance);
    return true;
}
