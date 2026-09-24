#include "../decide.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace xxqz;
using namespace unswbc;

int checks = 0;
void check(bool condition, char const* message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

struct Scenario {
    Game game{20, 20, 64};
    Controller ct{1, Team::A, Direction::EAST, Vision{}, 64};
    Scenario() {
        unswbc::game = &game;
        ct.head.position = {10, 10};
        ct.length = 3;
        ct.unit_count = 2;
        std::vector<Tile> tiles;
        for (int x = 7; x <= 13; ++x)
            for (int y = 7; y <= 13; ++y)
                tiles.emplace_back(Position{x, y}, std::nullopt, -1);
        ct.vision = Vision(std::move(tiles));
        ct.get_tile({10, 10})->dragon_part = DragonPart({10, 10}, 1, Team::A, Direction::EAST, true);
        ct.get_tile({9, 10})->dragon_part = DragonPart({9, 10}, 1, Team::A, Direction::NORTH, false);
        ct.get_tile({9, 11})->dragon_part = DragonPart({9, 11}, 1, Team::A, Direction::NORTH, false);
        ct.get_tile({11, 10})->pearl = true;
        ct.get_tile({10, 12})->pearl = true;
    }
};

std::string without_stats(std::string const& text) {
    std::istringstream input(text);
    std::string line, result;
    while (std::getline(input, line))
        if (line.rfind("LOG V05_STATS ", 0) != 0)
            result += line + '\n';
    return result;
}

int main() {
    Scenario s;
    StrategyConfig on = strategy, off = strategy;
    on.unified_perception = off.unified_perception = false; // Preserve the V05 baseline assertions.
    on.debug_stats = true;
    off.debug_stats = false;
    std::ostringstream output;
    auto original = std::cout.rdbuf(output.rdbuf());
    memory = Memory{};
    DebugStats measured;
    {
        DebugTurn turn(s.ct, s.game, on);
        act(s.ct, s.game, on);
        measured = turn.stats;
        turn.finish();
        turn.finish();
    }
    std::string profile = output.str();
    output.str(""); output.clear();
    memory = Memory{};
    {
        DebugTurn turn(s.ct, s.game, off);
        act(s.ct, s.game, off);
        check(turn.stats.tiles_scanned == 0, "runtime-off scope counted work");
    }
    std::string release = output.str();
    check(profile == release || without_stats(profile) == release, "instrumentation changed commands");
    check(release.find("V05_STATS") == std::string::npos, "runtime-off emitted diagnostics");
    check(active_stats == nullptr, "scope did not restore active collector");

    if constexpr (XXQZ_ENABLE_STATS != 0) {
        check(measured.memory_tiles_scanned == 49, "memory scan not counted once per visible tile");
        check(measured.sense_tiles_scanned == 49, "sense scan not counted once per visible tile");
        check(measured.broadcast_tiles_scanned == 49, "broadcast scan missing");
        check(measured.tiles_scanned == 147, "total visible scans inconsistent");
        check(measured.pocket_calls == 3 && measured.pocket_nodes == 138, "free region repeated work not measured");
        check(measured.pocket_degree_nodes == 138, "pocket degree work missing");
        check(measured.route_calls == 1 && measured.route_nodes > 0 && measured.route_nodes <= 320,
              "route budget accounting invalid");
        check(measured.sonar_sent == 1, "broadcast success not measured");
        check(profile.find("LOG V05_STATS ") == profile.rfind("LOG V05_STATS "), "finish logged twice");
        check(profile.find("round=1 id=1") != std::string::npos, "identity missing");
        {
            DebugTurn outer(s.ct, s.game, on);
            record(&DebugStats::route_nodes, 2);
            {
                DebugTurn inner(s.ct, s.game, off);
                record(&DebugStats::route_nodes, 100);
            }
            record(&DebugStats::route_nodes);
            check(outer.stats.route_nodes == 3, "nested disabled scope polluted or lost outer counts");
        }
        {
            DebugTurn fresh(s.ct, s.game, on);
            check(fresh.stats.tiles_scanned == 0 && fresh.stats.route_nodes == 0, "counts leaked between turns");
            s.ct.sonar_messages = {0U, encode_pearl({2, 2}, 1)};
            receive_sonar(s.ct, s.game, memory, on);
            check(fresh.stats.sonar_received == 2 && fresh.stats.sonar_accepted == 1, "received/accepted sonar confused");
            last_resort(s.ct);
            check(fresh.stats.fallback_calls == 1, "fallback not counted");
        }
        try {
            DebugTurn failed(s.ct, s.game, on);
            record(&DebugStats::exceptions);
            throw std::runtime_error("scope unwind test");
        } catch (std::runtime_error const&) {}
        check(active_stats == nullptr, "exception left collector dangling");
    } else {
        check(measured.tiles_scanned == 0 && measured.pocket_calls == 0 && measured.sonar_sent == 0,
              "compile-time-off still counted work");
        check(profile == release, "compile-time-off changed protocol");
    }
    std::cout.rdbuf(original);
    std::cout << checks << " instrumentation checks passed (compiled=" << XXQZ_ENABLE_STATS << ")\n";
}
