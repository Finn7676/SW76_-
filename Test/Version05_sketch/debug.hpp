#pragma once

#include "helper.hpp"
#include "strategy.hpp"

#include <array>
#include <cstdint>
#include <sstream>

namespace xxqz {

// Per-dragon, per-turn work counts. They are observations, never decision inputs.
struct DebugStats {
    std::uint64_t tiles_scanned = 0;
    std::uint64_t memory_tiles_scanned = 0;
    std::uint64_t sense_tiles_scanned = 0;
    std::uint64_t broadcast_tiles_scanned = 0;
    std::uint64_t memory_entries_aged = 0;
    std::uint64_t frontier_cells_checked = 0;
    std::uint64_t enemy_group_tiles = 0;
    std::uint64_t goal_tiles_scanned = 0;
    std::uint64_t pocket_calls = 0;
    std::uint64_t pocket_nodes = 0;
    std::uint64_t pocket_degree_nodes = 0;
    std::uint64_t route_calls = 0;
    std::uint64_t route_nodes = 0;
    std::uint64_t route_expanded = 0;
    std::uint64_t tactical_enemy_checks = 0;
    std::uint64_t escape_calls = 0;
    std::uint64_t escape_edges = 0;
    std::uint64_t short_trade_enemy_checks = 0;
    std::uint64_t stalk_enemy_checks = 0;
    std::uint64_t split_enemy_checks = 0;
    std::uint64_t dive_calls = 0;
    std::uint64_t dive_nodes = 0;
    std::uint64_t dive_enemy_checks = 0;
    std::uint64_t sonar_received = 0;
    std::uint64_t sonar_accepted = 0;
    std::uint64_t sonar_sent = 0;
    std::uint64_t fallback_calls = 0;
    std::uint64_t exceptions = 0;
    std::uint64_t emergency_defaults = 0;
};

struct StatField {
    char const* name;
    std::uint64_t DebugStats::* member;
};

inline constexpr std::array stat_fields{
    StatField{"tiles_scanned", &DebugStats::tiles_scanned},
    StatField{"memory_tiles_scanned", &DebugStats::memory_tiles_scanned},
    StatField{"sense_tiles_scanned", &DebugStats::sense_tiles_scanned},
    StatField{"broadcast_tiles_scanned", &DebugStats::broadcast_tiles_scanned},
    StatField{"memory_entries_aged", &DebugStats::memory_entries_aged},
    StatField{"frontier_cells_checked", &DebugStats::frontier_cells_checked},
    StatField{"enemy_group_tiles", &DebugStats::enemy_group_tiles},
    StatField{"goal_tiles_scanned", &DebugStats::goal_tiles_scanned},
    StatField{"pocket_calls", &DebugStats::pocket_calls},
    StatField{"pocket_nodes", &DebugStats::pocket_nodes},
    StatField{"pocket_degree_nodes", &DebugStats::pocket_degree_nodes},
    StatField{"route_calls", &DebugStats::route_calls},
    StatField{"route_nodes", &DebugStats::route_nodes},
    StatField{"route_expanded", &DebugStats::route_expanded},
    StatField{"tactical_enemy_checks", &DebugStats::tactical_enemy_checks},
    StatField{"escape_calls", &DebugStats::escape_calls},
    StatField{"escape_edges", &DebugStats::escape_edges},
    StatField{"short_trade_enemy_checks", &DebugStats::short_trade_enemy_checks},
    StatField{"stalk_enemy_checks", &DebugStats::stalk_enemy_checks},
    StatField{"split_enemy_checks", &DebugStats::split_enemy_checks},
    StatField{"dive_calls", &DebugStats::dive_calls},
    StatField{"dive_nodes", &DebugStats::dive_nodes},
    StatField{"dive_enemy_checks", &DebugStats::dive_enemy_checks},
    StatField{"sonar_received", &DebugStats::sonar_received},
    StatField{"sonar_accepted", &DebugStats::sonar_accepted},
    StatField{"sonar_sent", &DebugStats::sonar_sent},
    StatField{"fallback_calls", &DebugStats::fallback_calls},
    StatField{"exceptions", &DebugStats::exceptions},
    StatField{"emergency_defaults", &DebugStats::emergency_defaults}
};

inline DebugStats* active_stats = nullptr;

inline void record(std::uint64_t DebugStats::* member, std::uint64_t amount = 1) {
    if constexpr (XXQZ_ENABLE_STATS != 0) {
        if (active_stats)
            active_stats->*member += amount;
    }
}

inline void record_tile(std::uint64_t DebugStats::* phase) {
    record(phase);
    record(&DebugStats::tiles_scanned);
}

// The scope covers act() and exception recovery. finish() precedes ENDTURN.
// Functions remain usable independently: with no active scope their hooks are no-ops.
class DebugTurn {
  public:
    DebugStats stats;

    DebugTurn(unswbc::Controller& ct, unswbc::Game const& game, StrategyConfig const& config = strategy)
        : ct_(ct), round_(game.get_round_num()), previous_(active_stats) {
        if constexpr (XXQZ_ENABLE_STATS != 0) {
            enabled_ = config.debug_stats;
            active_stats = enabled_ ? &stats : nullptr;
        }
    }

    DebugTurn(DebugTurn const&) = delete;
    DebugTurn& operator=(DebugTurn const&) = delete;

    void finish() noexcept {
        if constexpr (XXQZ_ENABLE_STATS != 0) {
            if (finished_)
                return;
            finished_ = true;
            active_stats = previous_;
            if (!enabled_)
                return;
            try {
                std::ostringstream line;
                line << "V05_STATS round=" << round_ << " id=" << ct_.get_id();
                for (auto const& field : stat_fields)
                    line << ' ' << field.name << '=' << stats.*field.member;
                ct_.output_log(line.str());
            } catch (...) {
                // Diagnostic allocation/output failure must not replace an already selected action.
            }
        }
    }

    ~DebugTurn() { finish(); }

  private:
    unswbc::Controller& ct_;
    int round_;
    DebugStats* previous_;
    bool enabled_ = false;
    bool finished_ = false;
};

}  // namespace xxqz
