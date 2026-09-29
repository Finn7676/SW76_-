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

} // namespace bot
