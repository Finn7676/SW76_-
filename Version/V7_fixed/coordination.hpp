#pragma once

#include "topology.hpp"
#include "occupancy.hpp"

namespace bot {

// Sonar facts and intentions are short-lived evidence. None of these methods
// changes physical occupancy or makes a reported empty cell safe to enter.
class Coordination {
    TurnState const &turn;
    WorldModel const &world;
    Topology const &topology;
    OccupancyModel const &occupancy;

  public:
    Coordination(TurnState const &turn, WorldModel const &world, Topology const &topology,
                 OccupancyModel const &occupancy)
        : turn(turn), world(world), topology(topology), occupancy(occupancy) {}

    bool fresh(int round, int age = COORDINATION_MAX_AGE) const {
        return round >= 0 && round <= turn.round && turn.round - round <= age;
    }
    bool protectedPortalExit(int c) const {
        if(c<0)return false;
        for(auto const &[key,p]:world.observedPortalPassages) {
            if(key.second==turn.myId || p.emerged || p.enteredRound<0 ||
               turn.round<p.enteredRound || turn.round-p.enteredRound>=8)continue;
            if(topology.step(p.source,p.dir)==c)return true;
        }
        return false;
    }
    bool portalBusy(int pid) const {
        for (auto const &[key, report] : world.portalOccupancy)
            if (key.first == pid && key.second != turn.myId && report.busy && fresh(report.round))
                return true;
        return false;
    }
    bool portalHeld(int c, int d, int *senderLength = nullptr) const {
        if (senderLength) *senderLength = 0;
        bool held = false;
        if (c < 0 || d < 0 || d >= 4 || topology.edgeTypeAt(c * 4 + d) != E_PORTAL) return false;
        int pid = topology.portalIdAt(c * 4 + d), senderDir = d ^ 2, side = senderDir >> 1;
        for (auto const &[key, report] : world.portalHolds) {
            if (key.first != pid || key.second == turn.myId ||
                !fresh(report.rounds[side], PORTAL_HOLD_MAX_AGE)) continue;
            bool contradicted = false;
            for (auto const &part : turn.parts) {
                if (!part.mine || !part.head || part.id != key.second) continue;
                int edge = part.c * 4 + senderDir;
                // Current visible movement can release a previous-round hold.
                // Unknown terrain does not contradict the report.
                if (topology.edgeTypeAt(edge) != E_UNKNOWN &&
                    (topology.edgeTypeAt(edge) != E_PORTAL || topology.portalIdAt(edge) != pid))
                    contradicted = true;
            }
            if (contradicted) continue;
            held = true;
            if (!senderLength) return true;
            // Length and hold are one checksummed packet from the same action.
            // Saturation at 15 is a lower bound, sufficient for the >=2 gate.
            *senderLength = std::max(*senderLength, report.lengths[side]);
        }
        return held;
    }
    bool pearlAvailable(int c, int moves = 1) const {
        int arrival = turn.round + std::max(0, moves - 1);
        for (auto const &[id, claim] : world.pearlClaims) {
            if (id == turn.myId || claim.c != c || !fresh(claim.round) ||
                claim.arrivalRound < turn.round) continue;
            if (claim.arrivalRound < arrival || (claim.arrivalRound == arrival && id < turn.myId))
                return false;
        }
        return true;
    }
    bool knownPearl(int c) const {
        if (occupancy.visible(c)) return occupancy.hasPearl(c);
        int due = world.pearlDue[c];
        return fresh(world.pearlInfo[c], 3) && due >= 0 && due <= turn.round;
    }

    // Predict the next head cell only when the report still matches the head
    // currently observed. Unknown intentions enumerate possible local moves;
    // they carry a smaller penalty, not four hard reservations.
    int headRisk(int c, int move = 1) const {
        if (c < 0) return 0;
        int risk = 0;
        for (auto const &[id, plan] : world.allyPlans) {
            if (id == turn.myId || !fresh(plan.round, 1)) continue;
            bool local = occupancy.teammateHeadAt(plan.c) && occupancy.dragonId(plan.c) == id;
            // Higher IDs have not acted yet this round. A lower ID has already
            // acted, so its next intent concerns our second move/next round.
            bool actsBeforeUsAgain = id > turn.myId || move >= 2;
            if (!occupancy.visible(plan.c) && c == plan.c) risk += 5;
            if (plan.nextDir >= 0 && actsBeforeUsAgain) {
                int q = topology.step(plan.c, plan.nextDir);
                if (q == c) risk += local ? 7 : 3;
            }
        }
        for (auto const &part : turn.parts) {
            if (!part.mine || !part.head || part.id == turn.myId ||
                (part.id < turn.myId && move < 2)) continue;
            auto it = world.allyPlans.find(part.id);
            if (it != world.allyPlans.end() && fresh(it->second.round, 1) &&
                it->second.c == part.c && it->second.nextDir >= 0) continue;
            int options = 0;
            bool reaches = false;
            for (int d = 0; d < 4; ++d) {
                if (topology.edgeTypeAt(part.c * 4 + d) == E_KELP) continue;
                int q = topology.step(part.c, d);
                if (q < 0) { ++options; continue; }
                if (occupancy.occupied(q)) continue;
                ++options;
                reaches |= q == c;
            }
            if (reaches) risk += options == 1 ? 7 : 1;
        }
        return risk;
    }
    int stepRisk(int from, int d, int move = 1) const {
        int risk = headRisk(topology.step(from, d), move);
        if (topology.edgeTypeAt(from * 4 + d) == E_PORTAL) {
            int pid = topology.portalIdAt(from * 4 + d);
            if (portalBusy(pid)) risk += 10;
            if (portalHeld(from, d)) risk += 12;
            auto it = world.portalEchoHints.find(pid);
            // A single ray found an ally somewhere along its path. This is
            // weak evidence only; it does not establish exit occupancy.
            if (it != world.portalEchoHints.end() && fresh(it->second.round, 1) && it->second.allies)
                ++risk;
        }
        return risk;
    }
};

} // namespace bot
