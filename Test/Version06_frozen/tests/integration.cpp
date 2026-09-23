#include "../decide.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace xxqz;
using namespace unswbc;

int checks = 0;
void check(bool ok, char const* message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}

struct Fixture {
    Game game{20, 20, 64};
    Controller ct{1, Team::A, Direction::EAST, Vision{}, 64};
    Fixture() {
        unswbc::game = &game;
        ct.head.position = {10, 10};
        ct.length = 3;
        ct.unit_count = 2;
        std::vector<Tile> tiles;
        for (int x = 7; x <= 13; ++x)
            for (int y = 7; y <= 13; ++y)
                tiles.emplace_back(Position{x, y}, std::nullopt, -1);
        ct.vision = Vision(std::move(tiles));
        part({10, 10}, 1, Team::A, Direction::EAST, true);
        part({9, 10}, 1, Team::A, Direction::NORTH, false);
        part({9, 11}, 1, Team::A, Direction::NORTH, false);
    }
    void part(Position pos, int id, Team team, Direction dir, bool head) {
        ct.get_tile(pos)->dragon_part = DragonPart(pos, id, team, dir, head);
    }
    void edge(Position pos, Direction dir, EdgeType type) {
        ct.get_tile(pos)->get_edge(dir).edge_type = type;
        if (auto tile = ct.get_tile(pos.add_dir(dir)))
            tile->get_edge(dir.get_opposite()).edge_type = type;
    }
    void pearl(Position pos) { ct.get_tile(pos)->pearl = true; }
};

void test_memory_and_sonar() {
    Fixture f;
    Memory map;
    f.pearl({13, 10});
    map.observe(f.ct, f.game);
    check(map.pearls.count({13, 10}), "direct pearl missing");
    f.ct.vision = Vision{};
    f.game.round_num = 2;
    map.observe(f.ct, f.game);
    check(map.pearls.count({13, 10}), "spawn -1 must not erase unseen pearl");
    f.game.round_num = 34;
    map.observe(f.ct, f.game);
    check(!map.pearls.count({13, 10}), "direct pearl TTL must expire");

    check(!decode_pearl(123U, 34, 20, 20), "legacy/raw message accepted");
    check(!decode_pearl(encode_pearl({20, 0}, 34), 34, 20, 20), "off-map report accepted");
    check(!decode_pearl(encode_pearl({0, 0}, 35), 34, 20, 20), "future report accepted");
    check(!decode_pearl(encode_pearl({0, 0}, 25), 34, 20, 20), "expired report accepted");
    check(!decode_pearl(encode_pearl({0, 0}, 34) | 0x00200000U, 34, 20, 20), "reserved bits ignored");
    auto last = decode_pearl(encode_pearl({63, 63}, 500), 500, 64, 64);
    check(last && last->round == 500 && last->position == Position{63, 63}, "packet boundary mismatch");

    f.ct.sonar_messages = {encode_pearl({2, 2}, 33)};
    receive_sonar(f.ct, f.game, map);
    check(map.shared_pearls.at({2, 2}) == 33, "report not received");
    f.game.round_num = 40;
    map.observe(f.ct, f.game);
    receive_sonar(f.ct, f.game, map);
    check(map.shared_pearls.at({2, 2}) == 33, "repeated report rejuvenated");
    f.game.round_num = 42;
    map.observe(f.ct, f.game);
    receive_sonar(f.ct, f.game, map);
    check(!map.shared_pearls.count({2, 2}), "stale repeated report survived");

    f.ct.vision = Vision({Tile({2, 2}, std::nullopt, -1, false)});
    f.ct.sonar_messages = {encode_pearl({2, 2}, 42)};
    map.observe(f.ct, f.game);
    receive_sonar(f.ct, f.game, map);
    check(!map.shared_pearls.count({2, 2}), "current empty observation overwritten");
    f.ct.vision = Vision{};
    f.game.round_num = 43;
    map.observe(f.ct, f.game);
    receive_sonar(f.ct, f.game, map);
    check(!map.shared_pearls.count({2, 2}), "older report overrode remembered empty tile");
    f.ct.sonar_messages = {encode_pearl({2, 2}, 43)};
    receive_sonar(f.ct, f.game, map);
    check(map.shared_pearls.count({2, 2}), "newer report incorrectly rejected");
    StrategyConfig off = strategy;
    off.sonar = false;
    f.ct.sonar_messages = {encode_pearl({3, 3}, 43)};
    receive_sonar(f.ct, f.game, map, off);
    check(!map.shared_pearls.count({3, 3}), "sonar disable switch ignored");
}

void test_routing() {
    Fixture f;
    f.pearl({12, 10});
    f.edge({10, 10}, Direction::EAST, EdgeType::KELP);
    Memory map;
    map.observe(f.ct, f.game);
    View view = look(f.ct, f.game);
    check(view.body_complete && view.body.size() == 3, "bent-body reconstruction regressed");
    auto route = find_forage_route(view, map, view.safe);
    check(route && route->first == Direction::NORTH && route->distance == 4, "BFS failed to detour around kelp");
    auto picked = choose(f.ct, view, map);
    check(picked && picked->step.direction == Direction::NORTH && picked->routed, "BFS not integrated into choose");
    check(route->kind == RouteKind::VisiblePearl, "visible pearl priority lost");
    StrategyConfig no_route = strategy;
    no_route.routing = false;
    auto legacy = choose(f.ct, view, map, no_route);
    check(legacy && !legacy->routed, "routing disable switch ignored");
    std::vector<StepInfo> south_only;
    for (auto const& step : view.safe)
        if (step.direction == Direction::SOUTH)
            south_only.push_back(step);
    auto constrained = find_forage_route(view, map, south_only);
    check(constrained && constrained->first == Direction::SOUTH, "BFS bypassed safety tier restriction");

    f.edge({12, 10}, Direction::NORTH, EdgeType::KELP);
    f.edge({12, 10}, Direction::EAST, EdgeType::KELP);
    f.edge({12, 10}, Direction::SOUTH, EdgeType::KELP);
    f.edge({12, 10}, Direction::WEST, EdgeType::KELP);
    f.pearl({10, 7});
    map.observe(f.ct, f.game);
    view = look(f.ct, f.game);
    route = find_forage_route(view, map, view.safe);
    check(route && route->goal == Position{10, 7}, "unreachable near pearl hides reachable pearl");
    check(!find_forage_route(view, map, {}), "empty first-step set ignored");
    StrategyConfig limited = strategy;
    limited.route_nodes = 1;
    check(!find_forage_route(view, map, view.safe, limited), "node budget ignored");
    limited = strategy;
    limited.route_depth = 1;
    auto shallow = find_forage_route(view, map, view.safe, limited);
    check(!shallow, "depth limit ignored");

    Fixture remote;
    Memory distant;
    distant.observe(remote.ct, remote.game);
    distant.shared_pearls[{15, 10}] = 1;
    View rv = look(remote.ct, remote.game);
    auto shared = find_forage_route(rv, distant, rv.safe);
    check(shared && shared->kind == RouteKind::SharedPearl && shared->distance == 5, "remote shared pearl not routed");
    StrategyConfig off = strategy;
    off.sonar = false;
    auto no_shared = find_forage_route(rv, distant, rv.safe, off);
    check(no_shared && no_shared->kind == RouteKind::Explore, "disabled sonar still supplies goals");
    remote.pearl({10, 8});
    distant.observe(remote.ct, remote.game);
    rv = look(remote.ct, remote.game);
    auto own = find_forage_route(rv, distant, rv.safe);
    check(own && own->kind == RouteKind::VisiblePearl, "shared report displaced local resource");
    distant.edges[{15, 10, 1}] = EdgeType::PORTAL;
    check(!route_edge_open({15, 10}, 1, rv, distant), "unknown portal treated as ordinary movement");
}

void test_tactics() {
    Fixture f;
    f.ct.length = 2;
    f.part({11, 10}, 2, Team::B, Direction::WEST, true);
    f.part({12, 10}, 2, Team::B, Direction::WEST, false);
    f.part({13, 10}, 2, Team::B, Direction::WEST, false);
    View view = look(f.ct, f.game);
    auto trade = short_trade(f.ct, view);
    check(trade && *trade == Direction::EAST, "length-2 trade for length-3 missing");
    f.ct.unit_count = 1;
    check(!short_trade(f.ct, view), "last dragon sacrificed");
    f.ct.unit_count = 2;
    StrategyConfig off = strategy;
    off.short_trade = false;
    check(!short_trade(f.ct, view, off), "trade switch ignored");
    View equal = view;
    equal.enemies.front().visible = 2;
    check(!short_trade(f.ct, equal), "non-profitable trade accepted");
    f.ct.length = 3;
    check(!short_trade(f.ct, view), "short trade used a longer dragon");
    f.ct.length = 2;
    f.edge({10, 10}, Direction::EAST, EdgeType::KELP);
    view = look(f.ct, f.game);
    check(!short_trade(f.ct, view), "trade crossed kelp");

    Fixture danger;
    danger.part({12, 10}, 2, Team::B, Direction::WEST, true);
    View dv = look(danger.ct, danger.game);
    StepInfo east;
    east.direction = Direction::EAST;
    east.dest = {11, 10};
    check(tactical_score(danger.ct, dv, east) == -250, "head-facing risk mismatch");
    StrategyConfig passive = strategy;
    passive.head_risk = false;
    passive.corner_pressure = false;
    check(tactical_score(danger.ct, dv, east, passive) == 0, "tactical switches ignored");
    danger.edge({12, 10}, Direction::WEST, EdgeType::KELP);
    dv = look(danger.ct, danger.game);
    check(tactical_score(danger.ct, dv, east) == 0, "enemy behind kelp still penalizes move");
    danger.edge({12, 10}, Direction::WEST, EdgeType::EMPTY);
    danger.edge({12, 10}, Direction::NORTH, EdgeType::KELP);
    danger.edge({12, 10}, Direction::SOUTH, EdgeType::KELP);
    dv = look(danger.ct, danger.game);
    dv.w = 15; dv.h = 15;
    check(tactical_score(danger.ct, dv, east) > 0, "small-map corner pressure missing");
    danger.ct.length = 12;
    check(tactical_score(danger.ct, dv, east) < 0, "long dragon used as corner hunter");
    danger.edge({12, 10}, Direction::NORTH, EdgeType::PORTAL);
    dv = look(danger.ct, danger.game);
    check(enemy_escapes(dv.enemies.front(), dv) == 3, "portal escape falsely considered blocked");
}

void test_wrapping_and_live_edges() {
    Game game(20, 20, 64);
    unswbc::game = &game;
    View view;
    view.head = {0, 0}; view.w = 20; view.h = 20;
    view.snap[{0, 0}].owner = Owner::Me;
    view.snap[{19, 0}].pearl = true;
    StepInfo west;
    west.direction = Direction::WEST; west.dest = {19, 0};
    Memory map;
    map.configure(20, 20);
    map.edges[{0, 0, 3}] = EdgeType::KELP;
    auto route = find_forage_route(view, map, {west});
    check(route && route->first == Direction::WEST && route->distance == 1, "torus wrapping or live edge precedence failed");
    view.snap[{0, 0}].edges[3] = EdgeType::PORTAL;
    check(!find_forage_route(view, map, {west}), "route crossed visible portal");
}

void test_action_wiring() {
    Fixture f;
    f.pearl({11, 10});
    f.pearl({10, 12});
    memory = Memory{};
    std::ostringstream output;
    auto old = std::cout.rdbuf(output.rdbuf());
    act(f.ct, f.game);
    std::cout.rdbuf(old);
    std::string text = output.str();
    check(text.find("MOVE E\n") != std::string::npos, "act failed to eat adjacent pearl");
    check(text.find("SONAR " + std::to_string(encode_pearl({10, 12}, 1))) != std::string::npos,
          "act did not broadcast uneaten pearl");
    check(text.find(std::to_string(encode_pearl({11, 10}, 1))) == std::string::npos,
          "act broadcast pearl it is consuming");
    check(!memory.pearls.count({11, 10}), "consumed pearl retained");
    auto move_pos = text.find("MOVE ");
    check(text.find("MOVE ", move_pos + 1) == std::string::npos, "multiple actions emitted");
}

int main() {
    test_memory_and_sonar();
    test_routing();
    test_tactics();
    test_wrapping_and_live_edges();
    test_action_wiring();
    std::cout << checks << " checks passed\n";
}
