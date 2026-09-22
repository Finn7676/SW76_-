#include "strategy.hpp"
#include "communication.hpp"

int main() {
    auto [ct, game] = unswbc::init();
    champion::WorldModel world(game);
    champion::Strategy strategy(world);
    while(unswbc::update(ct,game)) {
        world.update(ct,game);
        auto action=strategy.decide(ct);
#ifdef CHAMPION_DEBUG
        ct.set_indicator_string("round="+std::to_string(world.round)+" len="+std::to_string(world.length)+
            " target="+std::to_string(strategy.currentTarget.position.x)+","+std::to_string(strategy.currentTarget.position.y)+
            " score="+std::to_string(action.score));
#endif
        action.execute(ct);
        world.remember(action);
        unswbc::end_turn();
    }
}
