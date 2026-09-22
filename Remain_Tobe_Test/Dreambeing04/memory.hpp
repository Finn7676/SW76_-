#pragma once
#include "types.hpp"
namespace mybot {
using EdgeKey = std::tuple<int, int, int>;
struct EdgeInfo { unswbc::EdgeType kind; int portal_id; };

// C: Persistent information owned by one dragon process.
class Memory {
public:
    explicit Memory(unswbc::Controller& controller, unswbc::Game& game)
        : ct_(controller), game_(game) {}
    int width = 0, height = 0, last_round = 0;
    Cells seen, pearls;
    std::map<EdgeKey, EdgeInfo> edges;
    std::map<int, std::set<EdgeKey>> portal_observations;
    std::map<Cell, int> eta;
    std::vector<Cell> eta_order; // Preserve Python dict insertion-order respawn ties.
    std::vector<Cell> body;
    std::optional<Cell> last_head;

    void configure(int w, int h);
    Cell step(Cell p, Direction d) const;
    EdgeKey canonical_edge(Cell p, Direction d) const;
    Cells observe();
    Transition transit(Cell p, Direction d) const;
    std::optional<Cell> portal_destination(Cell p, Direction d, int id) const;
    void apply_moves(const Moves& moves);
    void remember_split(int child_size);
private:
    unswbc::Controller& ct_;
    unswbc::Game& game_;
    void tick_unseen_eta(const Cells& visible);
    void sync_body(const std::vector<unswbc::DragonPart>& parts);
};
}
