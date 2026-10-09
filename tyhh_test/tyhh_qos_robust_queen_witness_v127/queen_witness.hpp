struct QueenVisibleBody {
    int id = -1;
    std::vector<int> cells;
};

// Part facing points toward the preceding segment; the rear can bend.
// Enemy tails require every possible predecessor cell to be visible.
bool queenWitnessBody(int head, QueenVisibleBody& result) {
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

bool queenWitnessOrdinary(ChosenAction const& action) {
    if (action.split || action.donation || action.dirs.empty() || action.dirs.size() > 4) return false;
    int c = HERE;
    for (int d : action.dirs) {
        if (!visible(c) || edgeT[c * 4 + d] != E_OPEN) return false;
        c = geo(c, d);
        if (!visible(c)) return false;
    }
    return true;
}

// A confirmed witness is a possible first free step, not a prediction that
// this enemy acts immediately next or that absent witnesses mean safety.
unsigned queenWitnessMask(QueenVisibleBody const& enemy, ActionProjection const& post) {
    unsigned result = 0;
    int head = enemy.cells.front();
    for (int d = 0; d < 4; ++d) {
        if (edgeT[head * 4 + d] != E_OPEN) continue;
        int q = geo(head, d);
        if (q != post.head || !visible(q)) continue;
        if (std::find(enemy.cells.begin(), enemy.cells.end(), q) != enemy.cells.end()) continue;
        result |= 1u << d;
    }
    return result;
}

int queenWitnessCount(unsigned mask) {
    int n = 0;
    while (mask) { n += int(mask & 1u); mask >>= 1; }
    return n;
}

bool queenWitnessContinuation(ActionProjection const& post) {
    for (int d = 0; d < 4; ++d) {
        if (!visible(post.head) || edgeT[post.head * 4 + d] != E_OPEN) continue;
        int q = geo(post.head, d);
        if (!visible(q) || std::find(post.body.begin(), post.body.end(), q) != post.body.end()) continue;
        if (occupied(q) && !(occMine[q] && occId[q] == MYID)) continue;
        return true;
    }
    return false;
}

void reviewQueenFirstStepWitness(Controller& ct) {
    if (!isQueen() || enemyHeads.empty() || !queenWitnessOrdinary(chosenAction)) return;
    QueenVisibleBody own;
    if (!queenWitnessBody(HERE, own)) return;
    for (int i = 0; i < LEN; ++i)
        if (ownSeg(LEN - 1 - i) != own.cells[i]) return;
    ActionProjection original = projectAction(chosenAction);
    if (!original.known || !original.survives) return;

    std::vector<QueenVisibleBody> enemies;
    std::vector<unsigned> originalMasks;
    int originalCount = 0;
    for (int head : enemyHeads) {
        QueenVisibleBody enemy;
        if (!queenWitnessBody(head, enemy)) continue;
        unsigned mask = queenWitnessMask(enemy, original);
        originalCount += queenWitnessCount(mask);
        enemies.push_back(enemy);
        originalMasks.push_back(mask);
    }
    if (!originalCount) return;

    constexpr int nodeLimit = 256;
    const int maxSteps = std::min(4, (LEN + 3) / 4);
    int nodes = 0, bestCount = originalCount, bestPrefix = -1;
    bool exhausted = false;
    ChosenAction path, best;
    auto search = [&](auto&& self, int c) -> void {
        if (int(path.dirs.size()) >= maxSteps) return;
        for (int d = 0; d < 4; ++d) {
            if (!visible(c) || edgeT[c * 4 + d] != E_OPEN) continue;
            int q = geo(c, d);
            if (!visible(q)) continue;
            if (++nodes > nodeLimit) { exhausted = true; return; }
            path.dirs.push_back(d);
            auto post = projectAction(path);
            if (post.known && post.survives) {
                if (post.len == original.len && !directFriendCollision(path) &&
                    queenWitnessContinuation(post)) {
                    auto risk = actionRisk(path);
                    if (!risk.strong && !risk.soft) {
                        int count = 0;
                        bool added = false;
                        for (size_t i = 0; i < enemies.size(); ++i) {
                            unsigned mask = queenWitnessMask(enemies[i], post);
                            added |= (mask & ~originalMasks[i]) != 0;
                            count += queenWitnessCount(mask);
                        }
                        int prefix = 0;
                        while (prefix < int(path.dirs.size()) && prefix < int(chosenAction.dirs.size()) &&
                               path.dirs[prefix] == chosenAction.dirs[prefix]) ++prefix;
                        if (!added && count < originalCount &&
                            (count < bestCount || (count == bestCount &&
                             (prefix > bestPrefix || (prefix == bestPrefix &&
                              path.dirs.size() < best.dirs.size()))))) {
                            bestCount = count; bestPrefix = prefix; best = path;
                        }
                    }
                }
                self(self, q);
            }
            path.dirs.pop_back();
            if (exhausted) return;
        }
    };
    search(search, HERE);
    if (exhausted || best.dirs.empty()) return;
    chosenAction = best;
    ct.output_log("QUEEN_FIRST_STEP_WITNESS", ROUND, MYID,
                  "confirmed_before", originalCount, "confirmed_after", bestCount,
                  "complete_heads", enemies.size(), "nodes", nodes);
}
