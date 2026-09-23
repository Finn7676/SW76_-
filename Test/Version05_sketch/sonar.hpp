#pragma once

#include "memory.hpp"

#include <cstdint>
#include <optional>

namespace xxqz {

struct PearlReport {
    Position position;
    int round = 0;
};

// 8-bit signature, 3 reserved bits, 9-bit round, 6-bit x, 6-bit y.
// Absolute round numbers cover all 500 rounds; raw sonar2 coordinates are rejected.
inline constexpr std::uint32_t PEARL_MESSAGE = 0xA7000000U;

inline std::uint32_t encode_pearl(Position pos, int round) {
    return PEARL_MESSAGE | (static_cast<std::uint32_t>(round) << 12U)
        | (static_cast<std::uint32_t>(pos.x) << 6U) | static_cast<std::uint32_t>(pos.y);
}

inline std::optional<PearlReport> decode_pearl(std::uint32_t message, int round, int width, int height,
                                               StrategyConfig const& config = strategy) {
    if ((message & 0xFFE00000U) != PEARL_MESSAGE)
        return std::nullopt;
    PearlReport report{{static_cast<int>((message >> 6U) & 63U), static_cast<int>(message & 63U)},
                       static_cast<int>((message >> 12U) & 511U)};
    if (report.position.x >= width || report.position.y >= height || report.round < 1
        || report.round > round || round - report.round > config.sonar_ttl)
        return std::nullopt;
    return report;
}

inline void receive_sonar(Controller const& ct, Game const& game, Memory& map,
                          StrategyConfig const& config = strategy) {
    if (!config.sonar)
        return;
    for (std::uint32_t message : ct.get_sonar_messages()) {
        record(&DebugStats::sonar_received);
        auto report = decode_pearl(message, game.get_round_num(), map.w, map.h, config);
        if (!report || ct.get_tile(report->position))
            continue;
        auto observed = map.observed_at.find(report->position);
        if (observed != map.observed_at.end() && observed->second >= report->round)
            continue;
        record(&DebugStats::sonar_accepted);
        int& timestamp = map.shared_pearls[report->position];
        timestamp = std::max(timestamp, report->round); // Arrival never refreshes the observation age.
    }
}

inline void broadcast_pearl(Controller& ct, Game const& game, Memory const& map,
                            std::optional<Position> consumed = std::nullopt,
                            StrategyConfig const& config = strategy) {
    if (!config.sonar)
        return;
    std::unordered_set<Position, PositionHash> visible;
    for (auto const& tile : ct.get_tiles()) {
        record_tile(&DebugStats::broadcast_tiles_scanned);
        if (tile.has_pearl() && !tile.get_dragon() && (!consumed || tile.get_position() != *consumed))
            visible.insert(tile.get_position());
    }
    // Only direct sightings are broadcast; never relay reports in a feedback loop.
    if (auto nearest = map.nearest(ct.get_position(), visible)) {
        if (ct.send_sonar(encode_pearl(*nearest, game.get_round_num())))
            record(&DebugStats::sonar_sent);
    }
}

}  // namespace xxqz
