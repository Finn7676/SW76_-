#include "decide.hpp"

int main() {
    auto [ct, game] = unswbc::init();
    while (unswbc::update(ct, game)) {
        try {
            xxqz::act(ct, game);
        } catch (...) {
            try {
                ct.make_move(xxqz::last_resort(ct));
            } catch (...) {
                ct.make_move(unswbc::Direction::NORTH);
            }
        }
        unswbc::end_turn();
    }
}
