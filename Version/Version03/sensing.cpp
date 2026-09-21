#include "sensing.hpp"
namespace mybot {
TurnContext Sensing::SenseNeaby() {
    if (mem_.width == 0) {
        const auto [w, h] = game_.get_map_size();
        mem_.configure(w, h);
    }
    mem_.observe();
    TurnContext context;
    for (const auto& tile : ct_.get_tiles()) {
        const auto* part = tile.get_dragon();
        if (part && part->get_id() != ct_.get_id()) context.occupied_others.insert(cell(tile.get_position()));
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
    Cells occupied = context.occupied_others;
    occupied.insert(mem_.body.begin(), mem_.body.end());
    context.options = safety_.classify_steps(occupied);
    return context;
}
}
