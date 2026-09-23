#include "decide.hpp"

int main() {
    auto [ct, game] = unswbc::init();
    while (unswbc::update(ct, game)) {
        xxqz::DebugTurn diagnostics(ct, game);
        try {
            xxqz::act(ct, game);
        } catch (...) {
            xxqz::record(&xxqz::DebugStats::exceptions);
            try {
                ct.make_move(xxqz::last_resort(ct));
            } catch (...) {
                xxqz::record(&xxqz::DebugStats::emergency_defaults);
                ct.make_move(unswbc::Direction::NORTH);
            }
        }
        diagnostics.finish();
        unswbc::end_turn();
    }
}
