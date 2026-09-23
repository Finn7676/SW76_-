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
    Game game{20,20,64};
    Controller ct{1,Team::A,Direction::EAST,Vision{},64};
    Fixture() {
        unswbc::game = &game;
        ct.head.position = {0,0};
        ct.length = 3;
        ct.unit_count = 2;
        std::vector<Tile> tiles;
        for (int x = -3; x <= 3; ++x)
            for (int y = -3; y <= 3; ++y)
                tiles.emplace_back(Position{(x+20)%20,(y+20)%20},std::nullopt,-1);
        ct.vision = Vision(std::move(tiles));
        part({0,0},1,Team::A,Direction::EAST,true);
        part({19,0},1,Team::A,Direction::NORTH,false);
        part({19,1},1,Team::A,Direction::NORTH,false);
    }
    void part(Position pos,int id,Team team,Direction dir,bool head) {
        ct.get_tile(pos)->dragon_part = DragonPart(pos,id,team,dir,head);
    }
    void edge(Position pos,Direction dir,EdgeType type) {
        ct.get_tile(pos)->get_edge(dir).edge_type = type;
        if (auto next=ct.get_tile(pos.add_dir(dir))) next->get_edge(dir.get_opposite()).edge_type = type;
    }
};

StrategyConfig uncached() {
    StrategyConfig c = strategy;
    c.component_cache = false; c.threat_cache = false;
    return c;
}

void compare_regions(View const& old, View const& cached) {
    check(cached.topology.ready,"reciprocal topology was not cached");
    std::size_t free = 0;
    for (auto const& [pos,cell] : old.snap) {
        if (cell.owner != Owner::Empty) continue;
        ++free;
        for (int length : {2,3,8,12,48,64})
            check(pocket_uncached(pos,old,length)==pocket(pos,cached,length),"region size/trap classification differs");
    }
    check(cached.topology.component_id.size()<=free,"occupied cells cached");
    for(auto const& step:cached.safe)
        check(cached.topology.component_id.count(step.dest),"candidate region missing from cache");
    check(old.safe.size()==cached.safe.size(),"safe-step count changed");
    for (std::size_t i=0;i<old.safe.size();++i) {
        auto const& a=old.safe[i];auto const& b=cached.safe[i];
        check(a.direction==b.direction && a.dest==b.dest && a.pocket==b.pocket
            && a.hard_trap==b.hard_trap && a.soft_trap==b.soft_trap,"safe-step semantics/order changed");
    }
}

void compare_threats(Fixture& f, View const& view) {
    int original_length=f.ct.length, original_units=f.ct.unit_count;
    for (int length : {2,6,7,12}) for (int units : {1,2}) for (int flags=0;flags<4;++flags) {
        f.ct.length=length;f.ct.unit_count=units;
        StrategyConfig c=strategy;
        c.head_risk=flags&1;c.corner_pressure=flags&2;
        c.hunt_area=500;c.head_risk_area=200;
        for (auto const& step:view.safe)
            check(tactical_score(f.ct,view,step,c)==tactical_score_uncached(f.ct,view,step,c),"cached tactical score differs");
    }
    f.ct.length=original_length;f.ct.unit_count=original_units;
    StepInfo arbitrary;arbitrary.dest={5,5};
    check(tactical_score(f.ct,view,arbitrary)==tactical_score_uncached(f.ct,view,arbitrary),"unsupported candidate did not use legacy score");
}

std::string act_output(Fixture& f, StrategyConfig config) {
    memory=Memory{};
    std::ostringstream out;
    auto saved=std::cout.rdbuf(out.rdbuf());
    try {act(f.ct,f.game,config);} catch(...) {std::cout.rdbuf(saved);throw;}
    std::cout.rdbuf(saved);
    return out.str();
}

void compare_actions(Fixture& f) {
    auto expected=act_output(f,uncached());
    Memory expected_memory=memory;
    for (bool unified : {false,true}) for (int flags=0;flags<4;++flags) {
        StrategyConfig c=strategy;
        c.unified_perception=unified;c.component_cache=flags&1;c.threat_cache=flags&2;
        check(act_output(f,c)==expected,"cache ablation changed commands");
        check(memory.seen==expected_memory.seen && memory.edges==expected_memory.edges
            && memory.pearls==expected_memory.pearls && memory.eta==expected_memory.eta
            && memory.observed_at==expected_memory.observed_at && memory.shared_pearls==expected_memory.shared_pearls,
            "derived cache mutated memory");
    }
}

void test_open_and_threat_work() {
    Fixture f;
    f.part({1,1},2,Team::B,Direction::NORTH,true); // Threatens both east and south.
    DebugStats stats;
    active_stats=&stats;
    auto view=look(f.ct,f.game);
    build_threats(view);
    active_stats=nullptr;
    auto old=look(f.ct,f.game,uncached());
    check(!old.threats.ready,"disabled/unrequested threat cache was built eagerly");
    compare_regions(old,view);
    compare_threats(f,view);
    if constexpr (XXQZ_ENABLE_STATS != 0) {
        check(stats.component_nodes==45 && stats.component_regions==1 && stats.pocket_calls==0,
              "open region was not visited once");
        check(stats.component_lookups==3 && stats.threat_enemy_checks==1 && stats.escape_calls==1,
              "threat/body work was duplicated");
    }
    auto const& enemy=view.threats.enemies.front();
    check(enemy.reachable_mask==((1U<<1)|(1U<<2)) && enemy.forward_mask==(1U<<1),"reach/facing masks wrong");
    check(enemy.escapes==4,"open enemy escape count changed");
    f.edge({1,1},Direction::EAST,EdgeType::KELP);
    f.edge({1,1},Direction::SOUTH,EdgeType::KELP);
    view=look(f.ct,f.game);
    build_threats(view);
    check(view.threats.enemies.front().escapes==2,"trapped enemy escape count wrong");
    compare_threats(f,view);
    f.edge({1,1},Direction::SOUTH,EdgeType::PORTAL);
    view=look(f.ct,f.game);
    build_threats(view);
    check(view.threats.enemies.front().escapes==3,"portal was not a possible enemy escape");
    compare_threats(f,view);
    compare_actions(f);
    // Explicit rebuilding invalidates old facts, including an empty new snapshot.
    view.snap.clear();view.enemies.clear();view.safe.clear();
    build_topology(view);build_threats(view);
    check(view.topology.component_id.empty() && view.topology.regions.empty()
        && view.threats.enemies.empty() && !view.threats.candidate_mask,"cache leaked across rebuilds");
}

void test_regions_and_directed_fallback() {
    Fixture f;
    // A closed narrow chain at the east candidate.
    for (auto const& tile:f.ct.get_tiles())
        for (auto d:directions()) f.edge(tile.position,d,EdgeType::KELP);
    f.edge({0,0},Direction::EAST,EdgeType::EMPTY);
    f.edge({1,0},Direction::EAST,EdgeType::EMPTY);
    auto view=look(f.ct,f.game);
    check(view.safe.size()==1 && view.safe[0].pocket==2 && view.safe[0].hard_trap,"closed chain not hard trap");
    compare_regions(look(f.ct,f.game,uncached()),view);
    f.edge({2,0},Direction::EAST,EdgeType::EMPTY);
    f.edge({3,0},Direction::EAST,EdgeType::EMPTY); // Unknown outside vision.
    view=look(f.ct,f.game);
    check(view.safe[0].soft_trap && !view.safe[0].hard_trap,"open corridor not soft trap");
    compare_regions(look(f.ct,f.game,uncached()),view);
    f.edge({3,0},Direction::EAST,EdgeType::PORTAL);
    view=look(f.ct,f.game);
    check(view.safe[0].hard_trap,"portal incorrectly connected pocket to unknown");
    f.ct.get_tile({2,0})->get_edge(Direction::WEST).edge_type=EdgeType::KELP;
    view=look(f.ct,f.game);
    check(!view.topology.ready,"asymmetric free edges must retain directed flood fill");
    for(auto const& step:view.safe)
        check(pocket(step.dest,view,f.ct.length)==pocket_uncached(step.dest,view,f.ct.length),"directed fallback changed");
    compare_actions(f);
}

void test_small_torus() {
    Fixture f;
    f.game.width=2;f.game.height=2;
    std::vector<Tile> tiles;
    for(int x=0;x<2;++x) for(int y=0;y<2;++y)
        tiles.emplace_back(Position{x,y},std::nullopt,-1);
    f.ct.vision=Vision(std::move(tiles));
    f.part({0,0},1,Team::A,Direction::EAST,true);
    f.part({1,1},2,Team::B,Direction::NORTH,true);
    auto view=look(f.ct,f.game);
    compare_regions(look(f.ct,f.game,uncached()),view);
    compare_threats(f,view);
    check(view.threats.enemies.front().forward_mask==0,
          "tiny-torus duplicate destination lost legacy facing overwrite order");
    compare_actions(f);
}

void test_generated() {
    std::mt19937 rng(270923);
    for (int test=0;test<80;++test) {
        Fixture f;
        f.ct.length=2+test%12;f.game.round_num=1+test*5;
        for(auto& tile:f.ct.vision.tiles) {
            tile.pearl=rng()%7==0;
            tile.pearl_time=static_cast<int>(rng()%16)-1;
            if(!tile.dragon_part && rng()%5==0) {
                int id=2+rng()%6;
                tile.dragon_part=DragonPart(tile.position,id,id==2?Team::A:Team::B,directions()[rng()%4],rng()%2==0);
            }
            // Mirrored assignment models the engine's shared physical edges.
            for(auto dir:directions()) f.edge(tile.position,dir,static_cast<EdgeType>(rng()%3));
        }
        auto old=look(f.ct,f.game,uncached());auto view=look(f.ct,f.game);
        compare_regions(old,view);compare_threats(f,view);compare_actions(f);
    }
}

int main() {
    test_open_and_threat_work();
    test_regions_and_directed_fallback();
    test_small_torus();
    test_generated();
    std::cout<<checks<<" derived-cache checks passed (compiled="<<XXQZ_ENABLE_STATS<<")\n";
}
