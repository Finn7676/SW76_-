#pragma once

#include "body.hpp"

namespace bot {

class Observer {
    TurnState &turn;
    WorldModel &world;
    Topology &topology;
    OccupancyModel &occupancy;
    BodyMemory &body;

  public:
    Observer(TurnState &turn, WorldModel &world, Topology &topology, OccupancyModel &occupancy, BodyMemory &body)
        : turn(turn), world(world), topology(topology), occupancy(occupancy), body(body) {}

    void observe(Controller &ct) {
        if (world.firstObservedRound < 0) world.firstObservedRound = turn.round;
        occupancy.beginTurn();
        turn.visibleCount = 0;
        turn.parts.clear();
        turn.enemyHeads.clear();
        turn.enemyHeadLen.clear();
        turn.enemyHeadExactLen.clear();
        turn.friendHeads.clear();
        turn.friendHeadLen.clear();
        turn.friendHeadExactLen.clear();
        turn.friendParts.clear();
        turn.visPearls.clear();
        turn.idCount.clear();
        for (auto const &t : ct.get_tiles()) {
            int c = topology.cid(t.position.x, t.position.y);
            turn.visibleCells[turn.visibleCount++] = c;
            occupancy.visStamp[c] = occupancy.STAMP;
            world.seenRound[c] = turn.round;
            occupancy.visPearl[c] = t.has_pearl();
            int pt = t.get_pearl_time();
            if (t.has_pearl()) {
                world.pearlDue[c] = turn.round;
                turn.visPearls.push_back(c);
            } else
                world.pearlDue[c] = pt >= 0 ? turn.round + pt : INF;
            world.pearlInfo[c] = turn.round;
            for (int d = 0; d < 4; ++d) {
                auto const &e = t.get_edge(Direction(DCH[d]));
                uint8_t ty = e.get_edge_type() == EdgeType::KELP ? E_KELP : e.is_portal() ? E_PORTAL : E_OPEN;
                if (world.edgeT[c * 4 + d] != ty || (ty == E_PORTAL && world.edgePid[c * 4 + d] != e.get_portal_id()))
                    topology.setEdge(c, d, ty, e.get_portal_id());
            }
            if (auto const *dr = t.get_dragon()) {
                Part p{c, dr->get_id(), dirIndex(dr->get_dir()), dr->get_team() == turn.myTeam, dr->is_head()};
                occupancy.occStamp[c] = occupancy.STAMP;
                occupancy.occId[c] = p.id;
                occupancy.occMine[c] = p.mine;
                occupancy.occHead[c] = p.head;
                occupancy.occFree[c] = INF;
                occupancy.occPart[c] = (int)turn.parts.size();
                turn.parts.push_back(p);
                bool found = false;
                for (auto &ic : turn.idCount)
                    if (ic.first == p.id) {
                        ++ic.second;
                        found = true;
                        break;
                    }
                if (!found) turn.idCount.push_back({p.id, 1});
            }
        }
        for (auto const &p : turn.parts) {
            if (p.mine) {
                if (p.c != turn.here) turn.friendParts.push_back(p.c);
                if (p.id != turn.myId && p.head) turn.friendHeads.push_back(p.c);
                // A teammate's body segment points toward its head. If that
                // direction crosses a portal, its chain proves a crossing;
                // merely standing beside the portal does not.
                if (p.id != turn.myId && !p.head)
                    topology.noteTeammateBodyAcrossPortal(p.c, p.facing, p.id, turn.round);
            } else if (p.head)
                turn.enemyHeads.push_back(p.c);
            if (!p.mine) world.lastEnemyIntelRound = turn.round;
        }
        // Keep expired records only while the same spanning body is still
        // visible, so it cannot renew the eight-round timer every turn.
        for(auto it=world.observedPortalPassages.begin();it!=world.observedPortalPassages.end();) {
            auto const &p=it->second;
            if(p.enteredRound>=0 && turn.round-p.enteredRound>=8 && p.lastBodyRound<turn.round-1)
                it=world.observedPortalPassages.erase(it);
            else ++it;
        }
        // Follow the visible same-ID chain from the landing to its head.
        // A glimpse of an unrelated head does not confirm emergence.
        for (auto &[key, passage] : world.observedPortalPassages) {
            if (passage.emerged || passage.source < 0) continue;
            int c = topology.step(passage.source,passage.dir);
            // Once the spanning segment is gone, a fresh same-ID head sighting
            // also confirms emergence; the landing chain need not remain visible.
            if(passage.lastBodyRound < turn.round) {
                for(auto const &p:turn.parts)if(p.mine && p.head && p.id==key.second)passage.emerged=true;
            }
            for (size_t n=0;n<turn.parts.size() && c>=0;++n) {
                if (!occupancy.occupied(c) || !occupancy.isMine(c) || occupancy.dragonId(c)!=key.second) break;
                int index=occupancy.occPart[c];if(index<0)break;
                auto const &part=turn.parts[index];
                if(part.head){passage.emerged=true;break;}
                c=topology.step(c,part.facing);
            }
        }
        noteNearbyDeaths();
        for (int c : turn.enemyHeads)
            turn.enemyHeadLen.push_back(turn.countOf(occupancy.occId[c]));
        turn.enemyHeadExactLen.assign(turn.enemyHeads.size(), -1);
        for (int c : turn.friendHeads)
            turn.friendHeadLen.push_back(turn.countOf(occupancy.occId[c]));
        turn.friendHeadExactLen.assign(turn.friendHeads.size(), -1);

        // chain reconstruction: facing points to the next segment towards the head
        std::vector<int> prevOf(turn.parts.size(), -1);
        for (size_t i = 0; i < turn.parts.size(); ++i) {
            auto const &p = turn.parts[i];
            if (p.head) continue;
            int n = topology.step(p.c, p.facing);
            if (n >= 0 && occupancy.occupied(n) && occupancy.occId[n] == p.id && occupancy.occMine[n] == p.mine)
                prevOf[occupancy.occPart[n]] = (int)i;
        }
        for (size_t hi = 0; hi < turn.parts.size(); ++hi) {
            auto const &hp = turn.parts[hi];
            if (!hp.head) continue;
            std::vector<int> chain;
            for (int cur = (int)hi; cur >= 0 && chain.size() < turn.parts.size(); cur = prevOf[cur])
                chain.push_back(cur);
            int k = (int)chain.size();
            int total = -1;
            if (hp.mine && hp.id == turn.myId)
                total = turn.length;
            else {
                int tail = turn.parts[chain.back()].c;
                bool confirmed = true;
                for (int d = 0; d < 4 && confirmed; ++d) {
                    uint8_t e = world.edgeT[tail * 4 + d];
                    if (e == E_KELP) continue;
                    int n = topology.step(tail, d);
                    if (n < 0 || !occupancy.visible(n)) confirmed = false;
                }
                if (confirmed) total = k;
            }
            if (total < 0) continue;
            if (!hp.mine) {
                auto it = std::find(turn.enemyHeads.begin(), turn.enemyHeads.end(), hp.c);
                if (it != turn.enemyHeads.end())
                    turn.enemyHeadExactLen[it - turn.enemyHeads.begin()] = total;
            } else if (hp.id != turn.myId) {
                auto it = std::find(turn.friendHeads.begin(), turn.friendHeads.end(), hp.c);
                if (it != turn.friendHeads.end())
                    turn.friendHeadExactLen[it - turn.friendHeads.begin()] = total;
            }
            // another dragon may eat (tail stays) or split (it stands still) before we get there
            int slack = 0;
            if (!(hp.mine && hp.id == turn.myId)) {
                for (int d = 0; d < 4; ++d) {
                    int n = topology.step(hp.c, d);
                    if (n >= 0 && occupancy.visible(n) && occupancy.visPearl[n]) slack = 1;
                }
                if (total >= 4) slack = std::max(slack, 1);
            }
            for (int j = 0; j < k; ++j) {
                int idx = std::max(0, total - 1 - j);
                occupancy.occFree[turn.parts[chain[j]].c] = idx + 2 + slack + (slack && idx == 0 ? 1 : 0);
            }
        }

        for (size_t i = 0; i < turn.friendHeads.size(); ++i) {
            int c = turn.friendHeads[i], id = occupancy.dragonId(c);
            int length = turn.friendHeadExactLen[i], lengthRound = turn.round;
            if (length >= 0) {
                // A complete current chain corrects historical sonar estimates.
                world.allyLengths[id] = {turn.round, length, true};
            } else {
                length = turn.friendHeadLen[i];
                auto report = world.allyLengths.find(id);
                bool freshLength = report != world.allyLengths.end() && report->second.round <= turn.round &&
                                   turn.round - report->second.round <= ALLY_LENGTH_MAX_AGE;
                if (freshLength) {
                    if (report->second.length > length) {
                        length = report->second.length;
                        lengthRound = report->second.round;
                    }
                } else {
                    auto it = world.allyRallies.find(id);
                    if (it != world.allyRallies.end() && it->second.lengthRound <= turn.round &&
                        turn.round - it->second.lengthRound <= V4_DONATION_REPORT_AGE && it->second.length > length) {
                        length = it->second.length;
                        lengthRound = it->second.lengthRound;
                    }
                }
            }
            world.allyRallies[id] = {c, turn.round, length, lengthRound};
        }
        // A fresh visible empty cell disproves a remote head position.
        for (auto it = world.allyRallies.begin(); it != world.allyRallies.end();) {
            auto const &r = it->second;
            bool contradicted = occupancy.visible(r.c) &&
                (!occupancy.occupied(r.c) || !occupancy.isHead(r.c) ||
                 !occupancy.isMine(r.c) || occupancy.dragonId(r.c) != it->first);
            if (contradicted || turn.round - r.round > (isQueenId(it->first) ? 8 : V4_DONATION_REPORT_AGE))
                it = world.allyRallies.erase(it);
            else ++it;
        }
        bool queenVisible = false;
        for (size_t i = 0; i < turn.enemyHeads.size(); ++i) {
            int c = turn.enemyHeads[i];
            if (!isQueenId(occupancy.dragonId(c))) continue;
            int exact = turn.enemyHeadExactLen[i];
            world.enemyQueen = {c, turn.round, exact >= 0 ? exact : std::max(2, turn.enemyHeadLen[i]), exact >= 0};
            queenVisible = true;
        }
        auto const &queen = world.enemyQueen;
        if (!queenVisible && queen.c >= 0 && occupancy.visible(queen.c) &&
            (!occupancy.enemyHeadAt(queen.c) || !isQueenId(occupancy.dragonId(queen.c))))
            world.enemyQueen = {}; // disprove position, never infer death
        body.seedVisibleChain();
        body.reconstructHidden();
    }

  private:
    void noteNearbyDeaths() {
#if FEAT_V3_DEATH_PEARLS
        world.deathClues.erase(std::remove_if(world.deathClues.begin(), world.deathClues.end(),
                                              [this](DeathClue const &clue) {
                                                  return turn.round - clue.round > (clue.friendly ? 12 : 2) ||
                                                         (occupancy.visible(clue.c) &&
                                                          (!occupancy.hasPearl(clue.c) || occupancy.occupied(clue.c)));
                                              }),
                               world.deathClues.end());
        if (world.lastPartsRound == turn.round - 1) {
            std::set<int> checkedIds;
            for (auto const &old : world.lastObservedParts) {
                if (old.id == turn.myId || !checkedIds.insert(old.id).second) continue;
                bool stillSeen = false;
                for (auto const &p : turn.parts)
                    if (p.id == old.id) { stillSeen = true; break; }
                if (stillSeen) continue;
                // A head becoming a pearl is the strongest visible clue.
                // With no old head in view, require two former body cells
                // becoming pearls to avoid chasing routine movement.
                bool headPearl = false;
                int bodyPearls = 0;
                for (auto const &part : world.lastObservedParts) {
                    if (part.id != old.id || !occupancy.visible(part.c) || !occupancy.hasPearl(part.c)) continue;
                    headPearl |= part.head;
                    ++bodyPearls;
                }
                if (!headPearl && bodyPearls < 2) continue;
                for (auto const &part : world.lastObservedParts) {
                    if (part.id != old.id || (occupancy.visible(part.c) && !occupancy.hasPearl(part.c))) continue;
                    if (std::none_of(world.deathClues.begin(), world.deathClues.end(),
                                     [&](DeathClue const &clue) { return clue.c == part.c; }))
                        world.deathClues.push_back({part.c, turn.round, part.mine});
                }
            }
        }
        world.lastObservedParts = turn.parts;
        world.lastPartsRound = turn.round;
#endif
    }
};

} // namespace bot
