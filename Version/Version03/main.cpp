#include "bot.hpp"

int main() {
    auto [controller, game] = unswbc::init();
    mybot::Bot bot(controller, game);
    while (unswbc::update(controller, game)) {
        try {
            bot.execute_turn();
        } catch (const std::exception&) {
            try {
                bot.emergency();
            } catch (const std::exception&) {
                controller.make_move(unswbc::Direction::NORTH);
            }
        }
        unswbc::end_turn();
    }
}
