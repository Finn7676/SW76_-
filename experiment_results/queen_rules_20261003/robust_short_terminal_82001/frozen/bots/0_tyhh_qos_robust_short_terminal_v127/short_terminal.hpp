// R25: avoid only proved quiet, ordinary LEN2 pearl terminals.
bool shortTerminalOwnBody() {
    if (LEN != 2 || countOf(MYID) != LEN) return false;
    int head = -1, tail = -1, ownCount = 0;
    for (auto const& p : parts) {
        if (!p.mine) return false;
        if (p.id != MYID) continue;
        ++ownCount;
        if (!visible(p.c) || !occupied(p.c) || occId[p.c] != MYID || !occMine[p.c]) return false;
        if (p.head) {
            if (head >= 0) return false;
            head = p.c;
        } else {
            if (tail >= 0 || edgeT[p.c * 4 + p.facing] != E_OPEN ||
                geo(p.c, p.facing) != HERE) return false;
            tail = p.c;
        }
    }
    return ownCount == 2 && head == HERE && tail >= 0 && tail != head &&
           ownSeg(1) == head && ownSeg(0) == tail;
}

bool shortTerminalProof(ActionProjection const& p) {
    if (!p.known || !p.survives || p.head < 0 || p.len != (int)p.body.size()) return false;
    for (int d = 0; d < 4; ++d) {
        uint8_t e = edgeT[p.head * 4 + d];
        if (e == E_KELP) continue;
        if (e != E_OPEN) return false;
        int q = geo(p.head, d);
        if (!visible(q) || std::find(p.body.begin(), p.body.end(), q) == p.body.end()) return false;
    }
    return true;
}

bool shortTerminalContinuation(ActionProjection const& p) {
    for (int d = 0; d < 4; ++d) {
        if (edgeT[p.head * 4 + d] != E_OPEN) continue;
        int q = geo(p.head, d);
        if (!visible(q) || std::find(p.body.begin(), p.body.end(), q) != p.body.end()) continue;
        if (occupied(q) && !(occMine[q] && occId[q] == MYID)) continue;
        ActionProjection next = p;
        next.body.insert(next.body.begin(), q);
        // A pearl consumed by the first move cannot grow the continuation twice.
        bool eats = visPearl[q] && q != p.head;
        if (eats) ++next.len; else next.body.pop_back();
        next.head = q; next.heading = d;
        if (!shortTerminalProof(next)) return true;
    }
    return false;
}

void reviewShortTerminal(Controller& ct) {
    if (isQueen() || LEN != 2 || chosenAction.split || chosenAction.donation ||
        chosenAction.dirs.size() != 1 || !shortTerminalOwnBody()) return;
    int originalDir = chosenAction.dirs[0];
    if (edgeT[HERE * 4 + originalDir] != E_OPEN) return;
    int dest = geo(HERE, originalDir);
    if (!visible(dest) || occupied(dest) || !visPearl[dest]) return;
    ActionProjection original = projectAction(chosenAction);
    if (original.len != 3 || !shortTerminalProof(original)) return;

    // Populate the unchanged scorer if a non-scored planner proposed the pearl.
    // Never admit its fallback directly: every replacement is reviewed below.
    ChosenAction proposed = chosenAction;
    chooseScoredAction(ct, championPolicy(), POP <= 2, POP >= 12, -1);
    chosenAction = proposed;
    int best = -1;
    long bestScore = LONG_MIN;
    for (int d = 0; d < 4; ++d) {
        if (edgeT[HERE * 4 + d] != E_OPEN || moveScores[d] == LONG_MIN) continue;
        int q = geo(HERE, d);
        if (!visible(q) || occupied(q)) continue;
        ChosenAction alt{{d}, 0, true, false};
        ActionProjection p = projectAction(alt);
        ProtectionRisk risk = actionRisk(alt);
        if (!p.known || !p.survives || directFriendCollision(alt) || risk.strong || risk.soft ||
            shortTerminalProof(p) || !shortTerminalContinuation(p)) continue;
        if (best < 0 || moveScores[d] > bestScore) { best = d; bestScore = moveScores[d]; }
    }
    if (best >= 0) chosenAction = {{best}, 0, true, false};
}
