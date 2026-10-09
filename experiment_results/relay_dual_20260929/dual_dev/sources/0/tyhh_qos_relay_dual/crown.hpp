#pragma once

// H2: relay-native endgame consolidation. Stages / recipient-relative donation
// and death-pearl collection are informed by local xxqztest; no planner copied.
// Included inside relay's unnamed namespace after the protection primitives.
struct CrownPolicy { int reserve, maxDonor, quiet; };
struct CrownClue { int cell, round; };
struct CrownRecipient { int id, head, length; };
int crownFirstRound = -1, crownLastEnemy = -1000, crownMemoryId = -1;
int crownTarget = -1, crownProgressRound = -1, crownBestDistance = INF;
int crownIgnoreId = -1, crownIgnoreUntil = -1;
std::vector<Part> crownPreviousFriends;
std::vector<CrownClue> crownClues;

CrownPolicy crownPolicy() {
    if (ROUND >= 460) return {2, 32, 6};
    if (ROUND >= 430) return {4, 20, 10};
    return {6, 10, 12};
}
void crownObserve() {
    if (crownMemoryId != MYID) {
        crownMemoryId = MYID; crownFirstRound = ROUND; crownLastEnemy = -1000;
        crownPreviousFriends.clear(); crownClues.clear();
        crownTarget = crownIgnoreId = -1; crownProgressRound = -1;
        crownBestDistance = INF; crownIgnoreUntil = -1;
    }
    for (auto const& p : parts) if (!p.mine) crownLastEnemy = ROUND;
    for (auto const& s : sightings) crownLastEnemy = std::max(crownLastEnemy, s.round);
    // Require an observed pearl on a disappeared friendly HEAD, and no
    // remaining visible segment of that ID, before interpreting its old body.
    for (auto const& h : crownPreviousFriends) {
        if (!h.head || !visible(h.c) || !visPearl[h.c] || countOf(h.id) != 0) continue;
        for (auto const& p : crownPreviousFriends)
            if (p.id == h.id && visible(p.c) && visPearl[p.c] && !occupied(p.c)) {
                auto it = std::find_if(crownClues.begin(), crownClues.end(),
                    [&](CrownClue const& c) { return c.cell == p.c; });
                if (it == crownClues.end()) crownClues.push_back({p.c, ROUND});
                else it->round = ROUND;
            }
    }
    crownClues.erase(std::remove_if(crownClues.begin(), crownClues.end(),
        [](CrownClue const& c) { return ROUND - c.round > 12 ||
            (visible(c.cell) && (!visPearl[c.cell] || occupied(c.cell))); }), crownClues.end());
    crownPreviousFriends.clear();
    for (auto const& p : parts) if (p.mine && p.id != MYID) crownPreviousFriends.push_back(p);
}
bool crownQuiet() {
#if FEAT_SACRIFICE
    int q = crownPolicy().quiet;
    return ROUND >= 400 && crownFirstRound >= 0 && ROUND - crownFirstRound >= q &&
           ROUND - crownLastEnemy >= q;
#else
    return false;
#endif
}
bool crownPreserve() { return crownQuiet() && LEN >= 8; }
bool crownAllowExpansion() { return !crownQuiet() || POP < crownPolicy().reserve; }
bool crownHorizon(int approach) {
    // Death leaves ceil(length/2) pearls, with a gap between successive pearls.
    return Constants::MAX_ROUNDS - ROUND >= approach + 2 * ((LEN + 1) / 2) + 3;
}
int crownKnownStep(int c, int d) {
    if (c < 0 || c >= NC || edgeT[c * 4 + d] == E_UNKNOWN) return -1;
    int q = moveCell(c, d);
    return q >= 0 && visible(q) ? q : -1;
}
bool crownVacantAfterDeath(int c) {
    return !occupied(c) || (occMine[c] && occId[c] == MYID);
}
bool crownCanCollect(CrownRecipient const& r) {
    if (!visible(r.head) || !occupied(r.head) || !occMine[r.head] ||
        !occHead[r.head] || occId[r.head] != r.id || r.length < 8 || r.length <= LEN) return false;
    std::vector<int> path{r.head};
    auto walk = [&](auto&& self, int c, int depth) -> bool {
        if (depth == 2) return false;
        for (int d = 0; d < 4; ++d) {
            int q = crownKnownStep(c, d);
            if (q < 0 || !crownVacantAfterDeath(q) ||
                std::find(path.begin(), path.end(), q) != path.end()) continue;
            if (q == HERE) {
                // The recipient's new neck and route must not be its only way out.
                for (int e = 0; e < 4; ++e) {
                    int next = crownKnownStep(q, e);
                    if (next >= 0 && next != HERE && crownVacantAfterDeath(next) &&
                        std::find(path.begin(), path.end(), next) == path.end()) return true;
                }
            } else {
                path.push_back(q);
                bool found = self(self, q, depth + 1);
                path.pop_back();
                if (found) return true;
            }
        }
        return false;
    };
    return walk(walk, r.head, 0);
}
bool crownSafeFirst(int d) {
    int q = crownKnownStep(HERE, d);
    if (q < 0 || occupied(q)) return false;
    bool eats = visPearl[q];
    if (onwardSafe(q, eats) == 0 || spaceFrom(q, eats, 32, true) < std::min(LEN + 1, 8)) return false;
    ChosenAction a{{d}, 0, false, false};
    return !directFriendCollision(a) && actionRisk(a).strong == 0;
}
template <class Goal>
int crownRoute(Goal goal, int maxDepth, int& distance) {
    std::vector<int> queue{HERE}, depth(NC, -1), first(NC, -1);
    depth[HERE] = 0; distance = INF;
    bool safe[4];
    for (int d = 0; d < 4; ++d) safe[d] = crownSafeFirst(d);
    for (size_t pos = 0; pos < queue.size() && pos < 192; ++pos) {
        int p = queue[pos];
        if (depth[p] >= maxDepth) continue;
        for (int d = 0; d < 4; ++d) {
            if (p == HERE && !safe[d]) continue;
            int q = crownKnownStep(p, d);
            if (q < 0 || occupied(q) || depth[q] >= 0) continue;
            depth[q] = depth[p] + 1; first[q] = p == HERE ? d : first[p];
            if (goal(q)) { distance = depth[q]; return first[q]; }
            queue.push_back(q);
        }
    }
    return -1;
}
bool crownAction(Controller& ct) {
    if (!crownQuiet()) { crownTarget = -1; crownBestDistance = INF; return false; }
    // Never split the freshly recovered length just because population fell.
    if (LEN >= 8 && !crownClues.empty()) {
        int distance;
        int d = crownRoute([](int c) {
            return visible(c) && visPearl[c] && std::any_of(crownClues.begin(), crownClues.end(),
                [&](CrownClue const& clue) { return clue.cell == c; });
        }, 8, distance);
        if (d >= 0 && distance < Constants::MAX_ROUNDS - ROUND) {
            doMove(d); ct.output_log("CROWN_COLLECT", ROUND, MYID, "distance", distance); return true;
        }
    }
    auto policy = crownPolicy();
    if (POP <= policy.reserve || LEN > policy.maxDonor || !crownHorizon(0)) return false;
    std::vector<CrownRecipient> recipients;
    for (auto const& h : parts) {
        if (!h.mine || !h.head || h.id == MYID) continue;
        int length = friendLength(h.id);
        if (length >= std::max(8, LEN + (ROUND < 430 ? 2 : 1)) &&
            !(h.id == crownIgnoreId && ROUND < crownIgnoreUntil))
            recipients.push_back({h.id, h.c, length});
    }
    std::sort(recipients.begin(), recipients.end(), [](CrownRecipient const& a, CrownRecipient const& b) {
        if ((a.id == crownTarget) != (b.id == crownTarget)) return a.id == crownTarget;
        return a.length != b.length ? a.length > b.length : a.id < b.id;
    });
    for (auto const& r : recipients) {
        if (crownCanCollect(r)) {
            chosenAction = {{}, 0, false, true};
            ct.output_log("CROWN_DONATE", ROUND, MYID, "len", LEN, "recipient", r.id, "length", r.length);
            return true;
        }
        int distance;
        int d = crownRoute([&](int c) {
            for (int side = 0; side < 4; ++side)
                if (crownKnownStep(r.head, side) == c) return true;
            return false;
        }, 6, distance);
        if (d < 0 || !crownHorizon(distance)) continue;
        if (crownTarget != r.id || distance < crownBestDistance) {
            crownTarget = r.id; crownBestDistance = distance; crownProgressRound = ROUND;
        } else if (ROUND - crownProgressRound >= 4) {
            crownIgnoreId = r.id; crownIgnoreUntil = ROUND + 6;
            crownTarget = -1; crownBestDistance = INF; continue;
        }
        doMove(d);
        ct.output_log("CROWN_APPROACH", ROUND, MYID, "recipient", r.id, "distance", distance);
        return true;
    }
    return false;
}
