#include "../decide.hpp"

#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

using namespace xxqz;
using namespace unswbc;

int checks = 0;
void check(bool ok, char const* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}

struct Fixture {
    Game game{20, 20, 64};
    Controller ct{1, Team::A, Direction::EAST, Vision{}, 64};
    explicit Fixture(Position head = {10, 10}) {
        unswbc::game = &game;
        ct.head.position = head;
        ct.length = 3;
        ct.unit_count = 2;
        std::vector<Tile> tiles;
        for (int x = -3; x <= 3; ++x)
            for (int y = -3; y <= 3; ++y)
                tiles.emplace_back(Position{(head.x + x + 20) % 20, (head.y + y + 20) % 20}, std::nullopt, -1);
        ct.vision = Vision(std::move(tiles));
        part(head, 1, Team::A, Direction::EAST, true);
        auto neck = head.add_dir(Direction::WEST);
        part(neck, 1, Team::A, Direction::NORTH, false);
        part(neck.add_dir(Direction::SOUTH), 1, Team::A, Direction::NORTH, false);
    }
    void part(Position pos, int id, Team team, Direction dir, bool head) {
        ct.get_tile(pos)->dragon_part = DragonPart(pos, id, team, dir, head);
    }
};

void same_memory(Memory const& a, Memory const& b) {
    check(a.w == b.w && a.h == b.h && a.last_round == b.last_round, "memory metadata changed");
    check(a.seen == b.seen && a.edges == b.edges && a.pearls == b.pearls && a.eta == b.eta
        && a.observed_at == b.observed_at && a.shared_pearls == b.shared_pearls, "memory facts changed");
}

void same_view(View const& a, View const& b) {
    check(a.head == b.head && a.w == b.w && a.h == b.h && a.body == b.body
        && a.body_complete == b.body_complete && a.ally_heads == b.ally_heads, "body/allies changed");
    check(a.snap.size() == b.snap.size(), "snapshot size changed");
    auto ai = a.snap.begin();
    auto bi = b.snap.begin();
    for (; ai != a.snap.end(); ++ai, ++bi) {
        auto const& x = ai->second;
        auto const& y = bi->second;
        check(ai->first == bi->first && x.pearl == y.pearl && x.eta == y.eta && x.owner == y.owner
            && x.dragon_id == y.dragon_id && x.is_head == y.is_head && x.facing == y.facing
            && x.edges == y.edges, "snapshot facts or traversal order changed");
    }
    check(a.enemies.size() == b.enemies.size() && a.safe.size() == b.safe.size(), "derived size changed");
    for (std::size_t i = 0; i < a.enemies.size(); ++i) {
        auto const& x = a.enemies[i]; auto const& y = b.enemies[i];
        check(x.id == y.id && x.has_head == y.has_head && (!x.has_head || x.head == y.head)
            && x.visible == y.visible, "enemy order/grouping changed");
    }
    for (std::size_t i = 0; i < a.safe.size(); ++i) {
        auto const& x = a.safe[i]; auto const& y = b.safe[i];
        check(x.direction == y.direction && x.dest == y.dest && x.pearl == y.pearl && x.eta == y.eta
            && x.hard_trap == y.hard_trap && x.soft_trap == y.soft_trap && x.pocket == y.pocket,
            "safe step facts/order changed");
    }
}

Perception compare_sensing(Fixture& f, Memory& old_map, Memory& new_map, StrategyConfig config = strategy) {
    old_map.observe(f.ct, f.game, config);
    receive_sonar(f.ct, f.game, old_map, config);
    View old_view = look(f.ct, f.game);
    auto perceived = sense_nearby(f.ct, f.game, new_map, config);
    same_memory(old_map, new_map);
    same_view(old_view, perceived.view);
    check(perceived.round == f.game.round_num, "perception round is stale");
    std::vector<Position> pearls, heads;
    for (auto const& tile : f.ct.get_tiles()) {
        if (tile.has_pearl() && !tile.get_dragon()) pearls.push_back(tile.get_position());
        if (auto part = tile.get_dragon(); part && part->get_team() != f.ct.get_team() && part->is_head())
            heads.push_back(tile.get_position());
    }
    check(pearls == perceived.visible_pearls && heads == perceived.visible_enemy_heads, "cached candidates differ");
    return perceived;
}

std::string capture(auto action) {
    std::ostringstream out;
    auto previous = std::cout.rdbuf(out.rdbuf());
    try { action(); } catch (...) { std::cout.rdbuf(previous); throw; }
    std::cout.rdbuf(previous);
    return out.str();
}

std::string compare_actions(Fixture& f, Memory initial = {}, StrategyConfig config = strategy) {
    DebugStats old_stats, new_stats;
    Memory old_after;
    config.unified_perception = false;
    memory = initial;
    active_stats = &old_stats;
    auto old_output = capture([&] { act(f.ct, f.game, config); });
    old_after = memory;
    config.unified_perception = true;
    memory = initial;
    active_stats = &new_stats;
    auto new_output = capture([&] { act(f.ct, f.game, config); });
    active_stats = nullptr;
    check(old_output == new_output, "unified pipeline changed decision protocol");
    same_memory(old_after, memory);
    if constexpr (XXQZ_ENABLE_STATS != 0) {
        check(new_stats.tiles_scanned == f.ct.get_tiles().size()
            && new_stats.perception_tiles_scanned == new_stats.tiles_scanned
            && new_stats.memory_tiles_scanned == 0 && new_stats.sense_tiles_scanned == 0
            && new_stats.broadcast_tiles_scanned == 0, "unified pipeline must visit each raw tile exactly once");
        for (auto const& field : stat_fields) {
            std::string name = field.name;
            if (name.find("tiles_scanned") != std::string::npos && name != "goal_tiles_scanned") continue;
            check(old_stats.*field.member == new_stats.*field.member, "non-sensing work changed");
        }
    } else {
        check(old_stats.tiles_scanned == 0 && new_stats.tiles_scanned == 0, "compile-off counts are active");
    }
    return new_output;
}

void test_reconciliation() {
    Fixture f;
    Memory a, b;
    f.ct.get_tile({11, 10})->pearl = true;
    f.ct.get_tile({12, 10})->pearl_time = 3;
    auto p = compare_sensing(f, a, b);
    check(p.view.body_complete && p.view.body == std::vector<Position>{{10,10},{9,10},{9,11}}, "curved body lost");
    f.game.round_num = 2;
    f.ct.get_tile({11, 10})->pearl = false;
    f.ct.sonar_messages = {encode_pearl({11,10},2), encode_pearl({2,2},2), 0U, encode_pearl({3,3},3)};
    compare_sensing(f, a, b);
    check(!b.pearls.count({11,10}) && !b.shared_pearls.count({11,10}) && b.shared_pearls.at({2,2}) == 2,
          "current observation failed to override sonar");
    f.ct.get_tile({13,13})->pearl = true;
    compare_sensing(f, a, b);
    f.ct.vision = Vision{};
    f.game.round_num = 4;
    f.ct.sonar_messages = {encode_pearl({11,10},1), encode_pearl({2,2},2)};
    compare_sensing(f, a, b);
    check(!b.shared_pearls.count({11,10}) && b.shared_pearls.at({2,2}) == 2 && b.pearls.count({13,13}),
          "stale report resurrected or report lifetime refreshed");
    f.game.round_num = 11;
    compare_sensing(f, a, b);
    check(b.shared_pearls.empty() && b.eta.empty() && b.pearls.count({13,13}), "independent aging failed");
    f.game.round_num = 35;
    compare_sensing(f, a, b);
    check(b.pearls.empty(), "direct pearl TTL did not expire");
    auto fallback = compare_actions(f, b);
    check(fallback.find("INDICATOR fallback") != std::string::npos, "empty vision fallback not exercised");
}

void test_broadcast_and_branches() {
    Fixture f;
    Memory a, b;
    f.ct.get_tile({11,10})->pearl = true;
    f.ct.get_tile({10,9})->pearl = true;
    f.ct.get_tile({10,11})->pearl = true;
    f.part({11,10}, 2, Team::A, Direction::EAST, true); // Occupied pearl is not broadcastable.
    auto p = compare_sensing(f, a, b);
    for (auto consumed : {std::optional<Position>{}, std::optional<Position>{{10,9}}}) {
        auto old_output = capture([&] { broadcast_pearl(f.ct, f.game, a, consumed); });
        auto new_output = capture([&] { broadcast_pearl(f.ct, f.game, b, p.visible_pearls, consumed); });
        check(old_output == new_output, "cached broadcast differs");
        Position expected = consumed ? Position{10,11} : Position{10,9};
        check(new_output == "SONAR " + std::to_string(encode_pearl(expected,1)) + "\n", "sonar tie/occupancy/consumption failed");
    }
    compare_actions(f);
    f.ct.length = 6;
    check(compare_actions(f).find("SPLIT 2") != std::string::npos, "voluntary split not exercised");
    for (auto dir : directions()) f.ct.get_tile({10,10})->get_edge(dir).edge_type = EdgeType::KELP;
    check(compare_actions(f).find("split trap") != std::string::npos, "emergency split not exercised");
    Fixture trade;
    trade.ct.length = 2;
    trade.part({11,10}, 2, Team::B, Direction::WEST, true);
    trade.part({12,10}, 2, Team::B, Direction::WEST, false);
    trade.part({13,10}, 2, Team::B, Direction::WEST, false);
    check(compare_actions(trade).find("short-trade") != std::string::npos, "short-trade branch not exercised");
    Fixture dive;
    dive.ct.length = 3;
    dive.part({12,10},2,Team::B,Direction::WEST,true);
    for (auto pos : std::vector<Position>{{12,11},{12,12},{12,13},{13,13},{13,12},{13,11}})
        dive.part(pos,2,Team::B,Direction::NORTH,false);
    check(compare_actions(dive).find("INDICATOR kill") != std::string::npos, "dive branch not exercised");
    Fixture consume;
    consume.ct.get_tile({11,10})->pearl = true;
    consume.ct.get_tile({10,12})->pearl = true;
    check(compare_actions(consume).find("MOVE E") != std::string::npos, "normal movement branch not exercised");
    check(!memory.pearls.count({11,10}), "consumed pearl remains in memory");
    StrategyConfig off = strategy; off.sonar = false;
    auto no_sonar = compare_actions(consume, {}, off);
    check(no_sonar.find("SONAR") == std::string::npos, "sonar off flag ignored");
}

void test_generated_views() {
    std::mt19937 rng(260923);
    for (int case_num = 0; case_num < 100; ++case_num) {
        Fixture f(case_num % 2 ? Position{0,0} : Position{10,10});
        f.game.round_num = 1 + case_num * 4;
        f.ct.length = 2 + case_num % 12;
        for (auto& tile : f.ct.vision.tiles) {
            tile.pearl = rng() % 7 == 0;
            tile.pearl_time = static_cast<int>(rng() % 15) - 1;
            if (!tile.dragon_part && rng() % 5 == 0) {
                int id = 2 + rng() % 5;
                tile.dragon_part = DragonPart(tile.position,id,id == 2 ? Team::A : Team::B,
                    directions()[rng() % 4], rng() % 2 == 0);
            }
            for (auto& edge : tile.edges) edge.edge_type = static_cast<EdgeType>(rng() % 3);
        }
        Memory a, b;
        auto p = compare_sensing(f, a, b);
        check(!p.view.body_complete || f.ct.length <= 3, "incomplete body was marked complete");
        compare_actions(f);
    }
}

int main() {
    test_reconciliation();
    test_broadcast_and_branches();
    test_generated_views();
    std::cout << checks << " unified-perception checks passed (compiled=" << XXQZ_ENABLE_STATS << ")\n";
}
