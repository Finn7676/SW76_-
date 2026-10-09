struct TradeVisibleBody {
    int id = -1;
    std::vector<int> cells;
};

// Each rear part faces its predecessor; all possible tail extensions must
// be visible before a shown enemy chain is treated as an exact body.
bool tradeVisibleBody(int head, TradeVisibleBody& result) {
    if (!visible(head) || !occupied(head) || !occHead[head]) return false;
    const int id = occId[head];
    const bool mine = occMine[head];
    std::vector<int> members, previous(parts.size(), -1);
    int headIndex = -1;
    for (size_t i = 0; i < parts.size(); ++i) {
        auto const& part = parts[i];
        if (part.id != id || part.mine != mine) continue;
        if (!visible(part.c) || !occupied(part.c) || occPart[part.c] != int(i)) return false;
        members.push_back(int(i));
        if (part.head) {
            if (headIndex >= 0 || part.c != head) return false;
            headIndex = int(i);
            continue;
        }
        if (edgeT[part.c * 4 + part.facing] != E_OPEN) return false;
        int front = geo(part.c, part.facing);
        if (!visible(front) || !occupied(front) || occId[front] != id || bool(occMine[front]) != mine) return false;
        int frontIndex = occPart[front];
        if (frontIndex < 0 || frontIndex >= int(parts.size()) || previous[frontIndex] >= 0) return false;
        previous[frontIndex] = int(i);
    }
    if (headIndex < 0) return false;
    std::vector<int> cells;
    for (int i = headIndex; i >= 0; i = previous[i]) {
        if (std::find(cells.begin(), cells.end(), parts[i].c) != cells.end()) return false;
        cells.push_back(parts[i].c);
    }
    if (cells.size() < 2 || cells.size() != members.size() || int(cells.size()) != countOf(id)) return false;
    if (mine && id == MYID) {
        if (int(cells.size()) != LEN) return false;
    } else {
        int tail = cells.back();
        for (int d = 0; d < 4; ++d) {
            if (edgeT[tail * 4 + d] == E_KELP) continue;
            if (edgeT[tail * 4 + d] != E_OPEN || !visible(geo(tail, d))) return false;
        }
    }
    result = {id, cells};
    return true;
}

bool tradeOrdinary(ChosenAction const& action) {
    if (action.split || action.donation || action.dirs.empty() || action.dirs.size() > 4) return false;
    int c = HERE;
    for (int d : action.dirs) {
        if (d < 0 || d >= 4 || !visible(c) || edgeT[c * 4 + d] != E_OPEN) return false;
        c = geo(c, d);
        if (!visible(c)) return false;
    }
    return true;
}

unsigned tradeHeadExposure(int head, int target) {
    unsigned mask = 0;
    for (int d = 0; d < 4; ++d)
        if (edgeT[head * 4 + d] == E_OPEN && geo(head, d) == target) mask |= 1u << d;
    return mask;
}

bool tradeShorterWitness(TradeVisibleBody const& enemy, ActionProjection const& post) {
    if (enemy.id == enemyQueenId() || int(enemy.cells.size()) >= post.len) return false;
    if (std::find(enemy.cells.begin(), enemy.cells.end(), post.head) != enemy.cells.end()) return false;
    return tradeHeadExposure(enemy.cells.front(), post.head) != 0;
}

bool tradeContinuation(ActionProjection const& post) {
    for (int d = 0; d < 4; ++d) {
        if (!visible(post.head) || edgeT[post.head * 4 + d] != E_OPEN) continue;
        int q = geo(post.head, d);
        if (!visible(q) || std::find(post.body.begin(), post.body.end(), q) != post.body.end()) continue;
        if (occupied(q) && !(occMine[q] && occId[q] == MYID)) continue;
        return true;
    }
    return false;
}

void reviewShorterHeadTrade(Controller& ct) {
    if (isQueen() || enemyHeads.empty() || !tradeOrdinary(chosenAction)) return;
    TradeVisibleBody own;
    if (!tradeVisibleBody(HERE, own)) return;
    for (int i = 0; i < LEN; ++i)
        if (ownSeg(LEN - 1 - i) != own.cells[i]) return;
    const ChosenAction proposed = chosenAction;
    const ActionProjection original = projectAction(proposed);
    if (!original.known || !original.survives) return;
    std::vector<TradeVisibleBody> enemies;
    std::vector<unsigned> originalExposure;
    for (int head : enemyHeads) {
        TradeVisibleBody enemy;
        if (tradeVisibleBody(head, enemy)) enemies.push_back(enemy);
        originalExposure.push_back(tradeHeadExposure(head, original.head));
    }
    int before = 0;
    for (auto const& enemy : enemies) before += tradeShorterWitness(enemy, original);
    if (!before) return;

    // Compute only the inherited score ordering; its temporary proposal is
    // not committed, and all final inherited reviews have already run.
    chooseScoredAction(ct, championPolicy(), POP <= 2, POP >= 12, -1);
    chosenAction = proposed;
    int best = -1;
    long bestScore = LONG_MIN;
    for (int d = 0; d < 4; ++d) {
        if (moveScores[d] == LONG_MIN) continue;
        ChosenAction alternative{{d}, 0, true, false};
        if (!tradeOrdinary(alternative)) continue;
        auto post = projectAction(alternative);
        if (!post.known || !post.survives || !tradeContinuation(post) || directFriendCollision(alternative)) continue;
        auto risk = actionRisk(alternative);
        if (risk.strong || risk.soft) continue;
        bool rejected = false;
        for (size_t i = 0; i < enemyHeads.size(); ++i)
            if (tradeHeadExposure(enemyHeads[i], post.head) & ~originalExposure[i]) rejected = true;
        for (auto const& enemy : enemies)
            if (tradeShorterWitness(enemy, post)) rejected = true;
        if (rejected) continue;
        if (best < 0 || moveScores[d] > bestScore) { best = d; bestScore = moveScores[d]; }
    }
    if (best < 0) return;
    chosenAction = {{best}, 0, true, false};
    ct.output_log("SHORTER_HEAD_TRADE", ROUND, MYID, "before", before,
                  "after", 0, "complete_heads", enemies.size(), "to", DCH[best]);
}
