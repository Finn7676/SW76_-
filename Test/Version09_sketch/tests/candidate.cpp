#include "../decide.hpp"
#include <iostream>
#include <stdexcept>
using namespace xxqz;
using namespace unswbc;
int checks=0;
void check(bool ok,char const* message) { ++checks; if(!ok) throw std::runtime_error(message); }
struct Fixture {
    Game game{20,20,64};
    Controller ct{3,Team::A,Direction::EAST,Vision{},64};
    Memory map;
    Fixture() {
        unswbc::game=&game;ct.head.position={10,10};ct.length=3;ct.unit_count=2;
        std::vector<Tile> tiles;
        for(int x=7;x<=13;++x)for(int y=7;y<=13;++y)tiles.emplace_back(Position{x,y},std::nullopt,-1);
        ct.vision=Vision(std::move(tiles));
        part({10,10},3,Team::A,Direction::EAST,true);
        part({9,10},3,Team::A,Direction::NORTH,false);
        part({9,11},3,Team::A,Direction::NORTH,false);
    }
    void part(Position p,int id,Team team,Direction d,bool head) {ct.get_tile(p)->dragon_part=DragonPart(p,id,team,d,head);}
    View view() {return sense_nearby(ct,game,map).view;}
};
StepInfo step(View const& v,Direction d) {
    for(auto s:v.safe)if(s.direction==d)return s;
    throw std::runtime_error("fixture has no requested move");
}
void test_growth_and_legality() {
    Fixture f;auto v=f.view();auto s=step(v,Direction::SOUTH);
    auto a=evaluate_move(f.ct,v,s);
    check(a.tail_released && a.exits==3,"vacated tail missing from resulting exits");
    s.pearl=true;auto b=evaluate_move(f.ct,v,s);
    check(!b.tail_released && b.exits==2 && b.space==a.space-1,"growth did not retain tail");
    v.body_complete=false;s.pearl=false;
    auto c=evaluate_move(f.ct,v,s);
    check(!c.tail_released && c.exits==2,"incomplete body guessed tail movement");
    s.dest={9,10};s.direction=Direction::WEST;
    check(evaluate_move(f.ct,v,s).category==MoveClass::Illegal,"occupied first step allowed");
    s=step(v,Direction::SOUTH);v.snap[v.head].edges[2]=EdgeType::PORTAL;
    check(evaluate_move(f.ct,v,s).category==MoveClass::Illegal,"portal treated as normal first step");
}
void test_threat_and_routing() {
    Fixture f;f.ct.get_tile({11,10})->pearl=true;
    f.part({12,10},8,Team::B,Direction::WEST,true);
    auto v=f.view();auto e=evaluate_move(f.ct,v,step(v,Direction::EAST));
    check(e.enemy_reach==1 && e.enemy_forward_reach==1 && e.category==MoveClass::Exposure,"head pressure missing");
    auto pick=choose(f.ct,v,f.map);
    check(pick && pick->step.direction!=Direction::EAST && pick->evaluation->category==MoveClass::Clear,"pearl/route bonus overrode danger class");
    StrategyConfig off=strategy;off.candidate_micro=false;
    auto old=choose(f.ct,v,f.map,off);
    check(old && old->step.direction==Direction::EAST && !old->evaluation,"off switch changed V08 pearl policy");
    v.enemies[0].visible=12;v.threats={};
    e=evaluate_move(f.ct,v,step(v,Direction::EAST));
    check(e.favorable_trade && e.category==MoveClass::FavorableExposure,"visible lower bound missed favorable sacrifice");
    f.ct.unit_count=1;e=evaluate_move(f.ct,v,step(v,Direction::EAST));
    check(!e.favorable_trade,"last dragon approved a favorable sacrifice");
    f.ct.unit_count=2;f.ct.length=12;e=evaluate_move(f.ct,v,step(v,Direction::EAST));
    check(!e.favorable_trade,"valuable long dragon treated as disposable");
    f.ct.length=3;v.snap[{12,10}].facing=Direction::NORTH;v.threats={};
    e=evaluate_move(f.ct,v,step(v,Direction::EAST));
    check(e.enemy_reach==1 && e.enemy_forward_reach==0,"entry orientation ignored");
    // Every alternative exposed still returns a deterministic executable move.
    v.safe={step(v,Direction::EAST)};v.threats={};
    auto one=choose(f.ct,v,f.map);auto two=choose(f.ct,v,f.map);
    check(one && two && one->step.direction==two->step.direction,"all-risk fallback missing");
}
void test_closed_and_unknown() {
    Fixture f;auto v=f.view();auto s=step(v,Direction::EAST);
    v.snap[s.dest].edges.fill(EdgeType::KELP);
    auto e=evaluate_move(f.ct,v,s);
    check(e.exits==0 && e.space==1 && e.category==MoveClass::NoExit,"sealed destination not classified");
    v.snap[s.dest].edges[1]=EdgeType::PORTAL;
    e=evaluate_move(f.ct,v,s);
    check(e.unknown_exits==1 && e.open_boundary && !e.enters_trap,"unknown portal falsely proved no exit");
    v.snap[s.dest].edges[1]=EdgeType::EMPTY;v.snap.erase({12,10});
    e=evaluate_move(f.ct,v,s);
    check(e.unknown_exits==1 && !e.enters_trap,"unseen region falsely classified closed");
    v.snap[s.dest].edges.fill(EdgeType::KELP);v.safe={s};
    auto pick=choose(f.ct,v,f.map);
    check(pick && pick->evaluation->category==MoveClass::NoExit,"least-bad move discarded");
}
void test_bounds_and_parity() {
    for(int mask=0;mask<8;++mask) {
        Fixture f;f.ct.get_tile({11,10})->pearl=true;
        StrategyConfig c=strategy;c.unified_perception=mask&1;c.component_cache=mask&2;c.threat_cache=mask&4;
        auto v=look(f.ct,f.game,c);f.map.w=20;f.map.h=20;f.map.last_round=1;
        DebugStats stats;active_stats=&stats;
        auto pick=choose(f.ct,v,f.map,c);active_stats=nullptr;
        check(pick && pick->step.direction==Direction::EAST,"cache flags changed clear pearl choice");
        if constexpr(XXQZ_ENABLE_STATS) {
            check(stats.micro_candidates==v.safe.size() && stats.micro_nodes<=4*49,"candidate work exceeded visible bound");
            check(stats.micro_selected==1 && stats.route_calls<=1 && stats.route_nodes<=320,"selection or navigation ran twice");
        }
    }
    Fixture f;auto v=f.view();auto a=evaluate_move(f.ct,v,step(v,Direction::EAST));
    auto b=a;b.ally_pressure=1;
    check(rank_move(f.ct,v,a,std::nullopt,false,0,0)>rank_move(f.ct,v,b,std::nullopt,false,0,0),"ally congestion lost");
    check(f.map.nav.dragon_id==-1,"feature extraction changed persistent navigation");
    // Wrap geometry and self-edge checks use the same toroidal convention.
    Game tiny{2,2,64};unswbc::game=&tiny;View t;t.w=t.h=2;t.head={0,0};
    Cell own;own.owner=Owner::Me;t.snap[t.head]=own;Cell free;t.snap[{1,0}]=free;
    StepInfo s;s.direction=Direction::WEST;s.dest={1,0};t.safe={s};
    auto e=evaluate_move(f.ct,t,s);check(e.category!=MoveClass::Illegal,"wrap move rejected");
}
int main() {
    test_growth_and_legality();test_threat_and_routing();test_closed_and_unknown();test_bounds_and_parity();
    std::cout<<checks<<" V09 candidate checks passed (compiled="<<XXQZ_ENABLE_STATS<<")\n";
}
