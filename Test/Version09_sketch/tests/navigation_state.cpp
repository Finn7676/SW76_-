#include "../decide.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace xxqz;
using namespace unswbc;
int checks=0;
void check(bool ok,char const* message) { ++checks;if(!ok)throw std::runtime_error(message); }

struct Fixture {
    Game game{20,20,64};
    Controller ct;
    Memory map;
    explicit Fixture(int id=3):ct(id,Team::A,Direction::EAST,Vision{},64) {
        unswbc::game=&game;ct.length=3;ct.unit_count=2;refresh({10,10});
    }
    void refresh(Position head) {
        ct.head.position=head;
        std::vector<Tile> tiles;
        for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y)
            tiles.emplace_back(Position{(head.x+x+20)%20,(head.y+y+20)%20},std::nullopt,-1);
        ct.vision=Vision(std::move(tiles));
        ct.get_tile(head)->dragon_part=DragonPart(head,ct.get_id(),Team::A,Direction::EAST,true);
        auto neck=head.add_dir(Direction::WEST);
        ct.get_tile(neck)->dragon_part=DragonPart(neck,ct.get_id(),Team::A,Direction::NORTH,false);
        auto tail=neck.add_dir(Direction::SOUTH);
        ct.get_tile(tail)->dragon_part=DragonPart(tail,ct.get_id(),Team::A,Direction::NORTH,false);
    }
    View sense() { return sense_nearby(ct,game,map).view; }
};

void test_sectors_and_economy() {
    std::unordered_set<int> sectors;
    for(int id : {0,2,4,6}) {
        Fixture f(id);auto view=f.sense();auto& nav=f.map.nav;
        observe_navigation(f.ct,view,1,nav,strategy);sectors.insert(nav.sector);
        auto route=find_navigation_route(f.ct,view,f.map,view.safe,nav,strategy);
        check(route && route->kind==RouteKind::Explore,"empty world failed to explore");
        int dx=signed_wrap_delta(view.head.x,route->goal.x,view.w);
        int dy=signed_wrap_delta(view.head.y,route->goal.y,view.h);
        auto [sx,sy]=directions()[nav.sector].get_offset();
        check(dx*sx+dy*sy>0,"exploration ignored assigned sector");
    }
    check(sectors.size()==4,"even-ID team did not receive all four sectors");
    Fixture f;f.ct.get_tile({13,13})->pearl=true;auto view=f.sense();auto& nav=f.map.nav;
    observe_navigation(f.ct,view,1,nav,strategy);
    auto route=find_navigation_route(f.ct,view,f.map,view.safe,nav,strategy);
    check(route && route->kind==RouteKind::Explore,"early expansion did not outrank distant pearl");
    StrategyConfig no_expand=strategy;no_expand.expansion=false;
    NavigationState fresh;observe_navigation(f.ct,view,1,fresh,no_expand);
    route=find_navigation_route(f.ct,view,f.map,view.safe,fresh,no_expand);
    check(route && route->goal==Position{13,13},"disabling expansion did not preserve pearl priority");
    StrategyConfig compact=strategy;compact.expansion_min_area=401;
    NavigationState limited;observe_navigation(f.ct,view,1,limited,compact);
    route=find_navigation_route(f.ct,view,f.map,view.safe,limited,compact);
    check(route && route->kind==RouteKind::VisiblePearl,"compact map expanded at resource expense");
    view.enemies.push_back(Enemy{7,true,{13,12},2});
    NavigationState contested;observe_navigation(f.ct,view,1,contested,strategy);
    route=find_navigation_route(f.ct,view,f.map,view.safe,contested,strategy);
    check(route && route->kind==RouteKind::VisiblePearl,"expansion ignored visible enemy head");
    f.ct.get_tile({11,10})->pearl=true;view=f.sense();
    route=find_navigation_route(f.ct,view,f.map,view.safe,nav,strategy);
    check(route && route->goal==Position{11,10} && route->first==Direction::EAST,"expansion displaced immediate pearl");
    check(signed_wrap_delta(0,19,20)==-1 && signed_wrap_delta(19,0,20)==1,"sector wrapping wrong");
    nav.sector=0;
    auto base=exploration_value({10,6},view,f.map,nav,strategy);
    view.ally_heads.push_back({10,7});
    check(exploration_value({10,6},view,f.map,nav,strategy)<base,"ally exploration overlap not penalized");
}

void test_lifecycle_and_recovery() {
    Fixture f;auto view=f.sense();auto& nav=f.map.nav;
    observe_navigation(f.ct,view,1,nav,strategy);
    nav.target=Position{13,10};nav.target_kind=RouteKind::RememberedPearl;nav.best_distance=3;
    for(int round=2;round<=4;++round) {
        f.game.round_num=round;f.refresh(round%2?Position{10,10}:Position{9,10});view=f.sense();
        observe_navigation(f.ct,view,round,nav,strategy);
    }
    check(nav.oscillating && nav.temporarily_avoid==Direction::EAST,"A-B-A-B loop did not avoid reversal");
    check(nav.stagnant_rounds==3,"non-improving target progress count wrong");
    int recent=nav.recent_count;observe_navigation(f.ct,view,4,nav,strategy);
    check(nav.recent_count==recent,"same turn updated history twice");
    nav.target=Position{13,10};nav.best_distance=4;nav.escape_until=8;nav.mode=NavMode::Escape;
    f.game.round_num=5;f.refresh({10,10});view=f.sense();observe_navigation(f.ct,view,5,nav,strategy);
    check(nav.mode==NavMode::Route && nav.escape_until==-1 && nav.stagnant_rounds==0,"progress did not exit escape mode");
    nav.target=Position{18,18};nav.target_kind=RouteKind::RememberedPearl;f.map.pearls.insert({18,18});
    nav.stagnant_rounds=strategy.retarget_rounds;
    auto route=find_navigation_route(f.ct,view,f.map,view.safe,nav,strategy);
    check(nav.rejected_target==Position{18,18} && nav.reject_until==5+strategy.target_cooldown,
          "stagnant target was not cooled down");
    check(!route || route->goal!=Position{18,18},"rejected target immediately selected again");
    check(nav.escape_until==8,"retargeting did not arm local recovery");
    nav.target=Position{11,10};nav.target_kind=RouteKind::RememberedPearl;
    check(!navigation_target_valid(nav,view,f.map,strategy),"visible empty tile remained a pearl target");
    nav.target=Position{2,2};nav.target_kind=RouteKind::SharedPearl;f.map.shared_pearls[{2,2}]=1;
    check(navigation_target_valid(nav,view,f.map,strategy),"fresh sonar target dropped");
    f.map.last_round=10;
    check(!navigation_target_valid(nav,view,f.map,strategy),"expired sonar target retained");
    nav.interrupt();check(!nav.target && !nav.recent_count && !nav.temporarily_avoid,"combat/split interruption kept stale intent");
    nav.dragon_id=999;observe_navigation(f.ct,view,10,nav,strategy);
    check(nav.dragon_id==f.ct.get_id() && nav.recent_count==1,"new dragon inherited old navigation");
}

void test_body_feasible_cycle() {
    Fixture f;
    std::array<Position,6> lap{Position{10,10},Position{11,10},Position{11,11},Position{10,11},Position{10,10},Position{11,10}};
    View view;
    for (int i=0;i<6;++i) {
        f.game.round_num=i+1;f.refresh(lap[i]);view=f.sense();
        observe_navigation(f.ct,view,i+1,f.map.nav,strategy);
    }
    check(f.map.nav.oscillating && f.map.nav.temporarily_avoid==Direction::SOUTH,
          "four-step dragon loop did not avoid the next repeated cell");
    f.map.pearls.insert({11,14});
    auto route=find_navigation_route(f.ct,view,f.map,view.safe,f.map.nav,strategy);
    check(!route || route->first!=Direction::SOUTH,"bounded reroute repeated the detected lap");
}

void test_bounds_and_escape() {
    Fixture f;auto view=f.sense();NavigationState nav;
    observe_navigation(f.ct,view,1,nav,strategy);
    StrategyConfig tiny=strategy;tiny.route_nodes=2;tiny.route_depth=1;
    DebugStats stats;active_stats=&stats;
    auto route=find_navigation_route(f.ct,view,f.map,view.safe,nav,tiny);
    active_stats=nullptr;
    check(!route,"route exceeded one-step search horizon");
    if constexpr(XXQZ_ENABLE_STATS!=0) check(stats.route_nodes<=2 && stats.route_calls==1,"bounded route budget exceeded");
    f.ct.get_tile({11,10})->pearl=true;view=f.sense();
    std::vector<StepInfo> south;
    for(auto step:view.safe)if(step.direction==Direction::SOUTH)south.push_back(step);
    route=find_navigation_route(f.ct,view,f.map,south,nav,strategy);
    check(!route || route->first==Direction::SOUTH,"planner escaped allowed safety class");
    f.ct.get_tile({10,10})->get_edge(Direction::EAST).edge_type=EdgeType::PORTAL;
    view=f.sense();route=find_navigation_route(f.ct,view,f.map,view.safe,nav,strategy);
    check(!route || route->first!=Direction::EAST,"navigation executed through portal");
    StepInfo cramped,open;cramped.dest={10,9};cramped.direction=Direction::NORTH;cramped.pocket=2;
    open=cramped;open.dest={10,11};open.direction=Direction::SOUTH;open.pocket=20;
    check(navigation_step_score(f.ct,view,open,nav,1,true,strategy)>
          navigation_step_score(f.ct,view,cramped,nav,1,true,strategy),"escape did not value larger space");
    nav.temporarily_avoid=Direction::SOUTH;nav.avoid_until=3;
    check(navigation_step_score(f.ct,view,open,nav,1,false,strategy)<0,"temporary avoidance missing");
    f.ct.get_tile({12,10})->dragon_part=DragonPart({12,10},8,Team::B,Direction::WEST,true);
    View danger=look(f.ct,f.game);
    std::vector<StepInfo> restricted;
    for(auto step:danger.safe)if(step.direction==Direction::EAST || step.direction==Direction::SOUTH)restricted.push_back(step);
    danger.safe=restricted;
    check(navigation_step_score(f.ct,danger,open,nav,1,false,strategy)==0,
          "avoidance forced a more threatened alternative");
    open.pearl=true;
    check(navigation_step_score(f.ct,view,open,nav,1,false,strategy)==0,"avoidance penalized immediate pearl");
}

void test_retention_and_ablation() {
    Fixture f;auto view=f.sense();auto& nav=f.map.nav;
    observe_navigation(f.ct,view,1,nav,strategy);
    f.map.pearls.insert({15,10});nav.target=Position{15,10};nav.target_kind=RouteKind::RememberedPearl;nav.best_distance=5;
    StrategyConfig config=strategy;config.expansion=false;
    auto route=find_navigation_route(f.ct,view,f.map,view.safe,nav,config);
    check(route && route->goal==Position{15,10},"valid reachable target not retained");
    StrategyConfig off=strategy;
    off.expansion=off.sector_exploration=off.nav_memory=off.anti_oscillation=off.retargeting=off.escape_mode=false;
    check(!navigation_enabled(off),"all-off ablation still enabled");
    auto expected=find_forage_route(view,f.map,view.safe,off);
    auto pick=choose(f.ct,view,f.map,off);
    check(expected && pick && pick->goal==expected->goal,"all-off route policy changed");
    config.routing=false;check(!navigation_enabled(config),"routing disabled still ran stateful planner");
}

int main() {
    test_sectors_and_economy();test_lifecycle_and_recovery();test_body_feasible_cycle();test_bounds_and_escape();test_retention_and_ablation();
    std::cout<<checks<<" V08 navigation sequence checks passed (compiled="<<XXQZ_ENABLE_STATS<<")\n";
}
