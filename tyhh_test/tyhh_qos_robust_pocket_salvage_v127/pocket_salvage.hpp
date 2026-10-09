#pragma once

bool pocketSalvageCompleteBody() {
    std::vector<int> cells;
    for (int j = 0; j < LEN; ++j) {
        int c = ownSeg(j);
        if (c < 0 || !visible(c) || !occupied(c) || !occMine[c] || occId[c] != MYID ||
            std::find(cells.begin(), cells.end(), c) != cells.end()) return false;
        cells.push_back(c);
    }
    return cells.back() == HERE;
}

// Three kelp boundaries and the entry edge prove a terminal, not an unknown room.
bool pocketSalvageTerminal(int target, int entry) {
    int open = 0;
    for (int d = 0; d < 4; ++d) {
        uint8_t edge = edgeT[target * 4 + d];
        if (edge == E_KELP) continue;
        if (edge != E_OPEN || moveCell(target, d) != entry) return false;
        ++open;
    }
    return open == 1;
}

bool pocketSalvageChildEscape(ActionProjection const& post) {
    // Reverse-split keeps the front two; the reversed rear starts at the old tail.
    if (post.body.size() != 5 || bridged(post.body[0], post.body[1])) return false;
    std::vector<int> child(post.body.rbegin(), post.body.rend() - 2);
    if (child.size() != 3) return false;
    const int head = child[0];
    for (int d = 0; d < 4; ++d) {
        if (edgeT[head * 4 + d] != E_OPEN) continue;
        int q = moveCell(head, d);
        if (q < 0 || !visible(q) || occupied(q) ||
            std::find(post.body.begin(), post.body.end(), q) != post.body.end()) continue;
        for (int next = 0; next < 4; ++next) {
            if (edgeT[q * 4 + next] != E_OPEN) continue;
            int r = moveCell(q, next);
            if (r >= 0 && r != q && visible(r) && !occupied(r) &&
                std::find(post.body.begin(), post.body.end(), r) == post.body.end()) return true;
        }
    }
    return false;
}

bool pocketSalvageAction(Controller& ct) {
    if (isQueen() || (LEN != 4 && LEN != 5) || !ct.can_split(2) || !splitSafe(2) ||
        !pocketSalvageCompleteBody() ||
        std::any_of(parts.begin(), parts.end(), [](Part const& p) { return !p.mine; })) return false;
    for (auto const& sighting : sightings)
        if (ROUND - sighting.round <= 6) return false;

    if (LEN == 5) {
        ChosenAction split{{}, 3, false, false};
        auto post = projectAction(split);
        if (!ct.can_split(3) || !splitSafe(3) || post.len != 2 ||
            !pocketSalvageTerminal(HERE, post.body[1]) ||
            !pocketSalvageChildEscape(post) || actionRisk(split).strong ||
            actionRisk(split).soft) return false;
        doSplit(3);
        ct.output_log("POCKET_SALVAGE", ROUND, MYID, "reverse", LEN);
        return true;
    }

    int targetDir = -1;
    for (int d = 0; d < 4; ++d) {
        uint8_t edge = edgeT[HERE * 4 + d];
        if (edge == E_KELP) continue;
        if (edge == E_UNKNOWN) return false;
        int q = moveCell(HERE, d);
        if (q < 0 || !visible(q)) return false;
        ChosenAction move{{d}, 0, false, false};
        auto post = projectAction(move);
        if (!post.known) return false;
        if (!post.survives) continue;
        if (targetDir >= 0 || edge != E_OPEN || !visPearl[q] ||
            !pocketSalvageTerminal(q, HERE) || directFriendCollision(move) ||
            actionRisk(move).strong || actionRisk(move).soft ||
            post.len != 5 || !pocketSalvageChildEscape(post)) return false;
        targetDir = d;
    }
    if (targetDir < 0) return false;
    doMove(targetDir);
    ct.output_log("POCKET_SALVAGE", ROUND, MYID, "pickup", LEN);
    return true;
}
