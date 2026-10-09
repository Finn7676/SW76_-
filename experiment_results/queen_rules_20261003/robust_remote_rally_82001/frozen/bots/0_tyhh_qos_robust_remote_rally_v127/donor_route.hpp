#pragma once

struct DonorRoute {
    int direction = -1, receiver = -1, distance = INF;
};

DonorRoute donorRoute() {
    DonorRoute result;
    if (isQueen() || LEN < 2 || LEN > 24 || !consolidateQuiet() ||
        consolidateReceiver(MYID, HERE, LEN) >= 0) return result;
    for (auto const& p : parts) if (!p.mine) return result;

    std::vector<Part> receivers;
    for (auto const& p : parts) {
        if (!p.mine || !p.head || p.id == MYID || p.id == queenId() ||
            !visible(p.c) || !occupied(p.c) || !occMine[p.c] ||
            !occHead[p.c] || occId[p.c] != p.id) continue;
        int length = friendLength(p.id);
        if (length < LEN || (length == LEN && p.id > MYID)) continue;
        receivers.push_back(p);
    }
    if (receivers.empty()) return result;

    std::vector<int> queue{HERE}, depth(NC, -1), first(NC, -1);
    depth[HERE] = 0;
    int expanded = 0, bestLength = -1;
    for (size_t pos = 0; pos < queue.size() && expanded < 128; ++pos) {
        int c = queue[pos];
        if (depth[c] > result.distance) break;
        ++expanded;
        if (depth[c] > 0) {
            for (auto const& receiver : receivers) {
                int pickup = consolidatePickup(receiver.c, c, MYID);
                if (pickup > 2 || Constants::MAX_ROUNDS - ROUND <
                    depth[c] + pickup + LEN + 6) continue;
                int length = friendLength(receiver.id);
                if (depth[c] < result.distance || length > bestLength ||
                    (length == bestLength && receiver.id < result.receiver)) {
                    result = {first[c], receiver.id, depth[c]};
                    bestLength = length;
                }
            }
        }
        if (depth[c] >= 6 || depth[c] >= result.distance) continue;
        for (int d = 0; d < 4; ++d) {
            if (edgeT[c * 4 + d] != E_OPEN) continue;
            int q = geo(c, d);
            if (!visible(q) || occupied(q) || depth[q] >= 0) continue;
            if (c == HERE) {
                ChosenAction step{{d}, 0, false, false};
                auto projected = projectAction(step);
                auto risk = actionRisk(step);
                if (!projected.known || !projected.survives ||
                    directFriendCollision(step) || risk.strong || risk.soft) continue;
            }
            depth[q] = depth[c] + 1;
            first[q] = c == HERE ? d : first[c];
            queue.push_back(q);
        }
    }
    return result;
}

bool donorRouteAction(Controller& ct) {
    auto route = donorRoute();
    if (route.direction < 0) return false;
    doMove(route.direction);
    ct.output_log("DONOR_ROUTE", ROUND, MYID, "receiver", route.receiver,
                  "donor_len", LEN, "approach_distance", route.distance);
    return true;
}
