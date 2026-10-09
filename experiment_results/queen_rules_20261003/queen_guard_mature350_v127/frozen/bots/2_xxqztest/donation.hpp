#pragma once

#include "world.hpp"

namespace bot {

// Local endgame consolidation policy; the planner's phase order is unchanged.
struct DonationPolicy {
    int reserve, maxDonorLength, lengthGap, quietRounds, minRecipientLength;

    static DonationPolicy at(GameConfig const &config, TurnState const &turn) {
        DonationPolicy p = turn.round >= V4_DONATION_FINAL_ROUND ? DonationPolicy{2, 32, 1, 6, 3}
                         : turn.round >= V4_DONATION_MIDDLE_ROUND ? DonationPolicy{4, 20, 1, 10, 3}
                         : DonationPolicy{8, 10, 2, V3_SACRIFICE_QUIET_ROUNDS, 4};
        p.reserve = std::min(p.reserve, std::max(1, config.targetPopulation / 2));
        // Keep a long dragon plus the escort slots; the other short dragons donate.
        if (turn.round >= V3_SACRIFICE_START_ROUND)
            p.reserve = std::min(p.reserve, V4_ESCORT_RESERVE);
        return p;
    }
    static bool active(TurnState const &turn) {
        return FEAT_SACRIFICE && turn.round >= V3_SACRIFICE_START_ROUND;
    }
    static bool quiet(GameConfig const &config, TurnState const &turn, WorldModel const &world) {
        if (!active(turn) || world.firstObservedRound < 0) return false;
        int required = at(config, turn).quietRounds;
        if (turn.round - world.lastQuietTeammateReportRound <= V3_QUIET_REPORT_MAX_AGE)
            required = std::min(required, V3_SACRIFICE_ALLY_QUIET_ROUNDS);
        if (turn.round - world.firstObservedRound < required ||
            turn.round - world.lastEnemyIntelRound < required) return false;
        for (auto const &part : turn.parts)
            if (!part.mine) return false;
        return true;
    }
    bool donor(TurnState const &turn) const {
        return active(turn) && turn.population > reserve && turn.length <= maxDonorLength &&
               Constants::MAX_ROUNDS - turn.round >= (turn.length + 1) / 2 + 3;
    }
    bool recipient(TurnState const &turn, int length) const {
        return length >= minRecipientLength && length >= turn.length + lengthGap;
    }
};

// After the short phase, two ID slots stay short and scout. Everyone else that
// short answers a long dragon's sonar by giving it their body.
struct EscortPolicy {
    static bool small(TurnState const &turn) {
        return turn.length >= 2 && turn.length <= V4_ESCORT_MAX_LENGTH;
    }
    static bool isEscort(TurnState const &turn) {
        return turn.round >= V4_SUMMON_ROUND && small(turn) && turn.myId >= 0 &&
               (turn.myId % V4_ESCORT_MOD) < V4_ESCORT_SLOTS;
    }
    static bool hasSummon(TurnState const &turn, WorldModel const &world) {
        for (auto const &[id, rally] : world.allyRallies) {
            if (id == turn.myId || !rally.summoned || turn.round - rally.round > V4_SUMMON_AGE) continue;
            if (rally.length > V4_ESCORT_MAX_LENGTH && rally.length > turn.length) return true;
        }
        return false;
    }
    static bool quietEndgame(GameConfig const &config, TurnState const &turn, WorldModel const &world) {
        return DonationPolicy::quiet(config, turn, world);
    }
    static bool yieldsPearls(GameConfig const &config, TurnState const &turn, WorldModel const &world) {
        (void)config;
        if (turn.round < V4_SUMMON_ROUND || !small(turn)) return false;
        return isEscort(turn) || hasSummon(turn, world);
    }
};

} // namespace bot
