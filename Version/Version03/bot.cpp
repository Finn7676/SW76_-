#include "bot.hpp"
#include "config.hpp"
namespace mybot {
Bot::Bot(unswbc::Controller& controller, unswbc::Game& game)
    : ct_(controller), game_(game), mem_(controller, game), navigation_(mem_),
      safety_(mem_, controller), scoring_(mem_, controller, navigation_, safety_),
      strategy_(mem_, controller, navigation_) {}

void Bot::execute_turn() {
    if (mem_.width == 0) {
        const auto [w, h] = game_.get_map_size();
        mem_.configure(w, h);
    }
    mem_.observe();
    Cells occupied_others;
    for (const auto& tile : ct_.get_tiles()) {
        const auto* part = tile.get_dragon();
        if (part && part->get_id() != ct_.get_id()) occupied_others.insert(cell(tile.get_position()));
    }
    const Cell head = cell(ct_.get_position());
    if (mem_.body.empty() || mem_.body.front() != head) {
        std::vector<Cell> repaired{head};
        for (Cell p : mem_.body) if (p != head) repaired.push_back(p);
        if (static_cast<int>(repaired.size()) > ct_.get_length()) repaired.resize(ct_.get_length());
        if (repaired.empty()) repaired.push_back(head);
        mem_.body = std::move(repaired);
        mem_.last_head = mem_.body.front();
    }
    Target target = strategy_.choose_target();
    Path path = target.goals.empty() ? std::nullopt : navigation_.local_bfs(occupied_others, target.goals, true);
    const Moves preferred = target.position ? navigation_.heading_towards(*target.position) : directions();
    Cells occupied = occupied_others;
    occupied.insert(mem_.body.begin(), mem_.body.end());
    const Options options = safety_.classify_steps(occupied);
    if (path && !path->empty()) {
        const bool open_first_step = std::any_of(options.open.begin(), options.open.end(), [&](const Step& step) {
            return step.direction == path->front();
        });
        if (!open_first_step) path.reset();
    }
    Moves moves;
    if (!options.open.empty()) {
        if (strategy_.want_split(occupied_others, path) && strategy_.normal_split()) return;
        moves = scoring_.pick_from_options(options.open, occupied_others, preferred, target.position, path);
    } else if (strategy_.split_to_small()) {
        return;
    } else {
        target.reason = "dead-end";
        moves = scoring_.pick_from_options(options.dead, occupied_others, preferred, target.position, path);
    }
    if (moves.empty()) moves = safety_.fallback_move();
    if constexpr (config::replay_indicators) {
        if (target.position) ct_.draw_indicator_dot(position(*target.position), 255, 210, 40);
        ct_.set_indicator_string(target.reason + " " + std::string(1, moves.front().value));
    }
    // Keep the Python bot's single-step action policy; simulation is reusable
    // for multiple steps, but sprint planning is not enabled by this port.
    ct_.make_move(moves.front());
    mem_.apply_moves(moves);
}
void Bot::emergency() {
    const auto moves = safety_.fallback_move();
    ct_.make_move(moves.front());
}
}
