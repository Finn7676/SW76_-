#pragma once

#include "world.hpp"

namespace bot {

// Quiet/reserve scheduling is retained for the existing expansion policy.
// Feeding eligibility is independent of that team-wide quiet heuristic.
struct DonationPolicy {
    int reserve, quietRounds, minRecipientLength; // last field is the legacy sonar packet filter

    static DonationPolicy at(GameConfig const &config, TurnState const &turn) {
        DonationPolicy p = turn.round >= V4_DONATION_FINAL_ROUND ? DonationPolicy{2, 6, 3}
                         : turn.round >= V4_DONATION_MIDDLE_ROUND ? DonationPolicy{4, 10, 3}
                         : DonationPolicy{8, V3_SACRIFICE_QUIET_ROUNDS, 4};
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
        return !turn.isQueen() && feedingActive(turn) && turn.population > 1 && turn.length >= 2 &&
               (turn.length <= V5_FEED_LARGE_DONOR_LENGTH ||
                turn.population > V5_FEED_LARGE_DONOR_POPULATION);
    }
    bool recipient(TurnState const &turn, int length, bool queen = false) const {
        if (queen) return length >= Constants::MIN_SIZE;
        return length >= turn.length + V5_FEED_MIN_LENGTH_GAIN;
    }
    static bool feedingActive(TurnState const &turn) {
        return FEAT_SACRIFICE && turn.round >= V5_FEED_START_ROUND;
    }
};

// Owned by one Planner instance; no shared leader or global length table.
struct FeedTargetMemory {
    int id = -1, cell = -1, length = 0, lastSeenRound = -1;
    int startedRound = -1, progressRound = -1, lastDistance = INF;
    int cooldownUntil = -1;
    void clear() {
        int cooldown = cooldownUntil;
        *this = {};
        cooldownUntil = cooldown;
    }
    void abandon(int round) {
        clear();
        cooldownUntil = round + V5_FEED_CHASE_COOLDOWN;
    }
    bool expired(int round) const {
        return id >= 0 && (round - lastSeenRound >= V5_FEED_TARGET_TIMEOUT ||
                           round - startedRound >= V5_FEED_MAX_CHASE_ROUNDS ||
                           round - progressRound >= V5_FEED_NO_PROGRESS_ROUNDS);
    }
};

} // namespace bot
