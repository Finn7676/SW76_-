#include "memory.hpp"
namespace mybot {
void Memory::configure(int w, int h) { width = w; height = h; }
Cell Memory::step(Cell p, Direction d) const {
    const auto [dx, dy] = d.get_offset();
    return {(p.first + dx + width) % width, (p.second + dy + height) % height};
}
EdgeKey Memory::canonical_edge(Cell p, Direction d) const {
    if (d.value != Direction::EAST && d.value != Direction::SOUTH) {
        p = step(p, d);
        d = d.get_opposite();
    }
    return {p.first, p.second, direction_index(d)};
}
Cells Memory::observe() {
    Cells occupied, visible;
    std::vector<unswbc::DragonPart> own_parts;
    for (const auto& tile : ct_.get_tiles()) {
        const Cell p = cell(tile.get_position());
        visible.insert(p);
        seen.insert(p);
        if (tile.has_pearl()) pearls.insert(p);
        else pearls.erase(p);
        if (!eta.contains(p)) eta_order.push_back(p);
        eta[p] = tile.get_pearl_time();
        if (const auto* part = tile.get_dragon()) {
            occupied.insert(p);
            if (part->get_id() == ct_.get_id()) own_parts.push_back(*part);
        }
        for (auto d : directions()) {
            const auto& e = tile.get_edge(d);
            const EdgeKey key{p.first, p.second, direction_index(d)};
            edges.insert_or_assign(key, EdgeInfo{e.get_edge_type(), e.get_portal_id()});
            if (e.is_portal()) portal_observations[e.get_portal_id()].insert(key);
        }
    }
    tick_unseen_eta(visible);
    sync_body(own_parts);
    return occupied;
}
void Memory::tick_unseen_eta(const Cells& visible) {
    const int now = game_.get_round_num();
    const int delta = last_round != 0 ? now - last_round : 0;
    last_round = now;
    if (delta <= 0) return;
    for (auto it = eta.begin(); it != eta.end();) {
        if (visible.contains(it->first) || it->second < 0) { ++it; continue; }
        const int next = it->second - delta;
        if (next < 0) it = eta.erase(it);
        else { it->second = next; ++it; }
    }
    std::erase_if(eta_order, [&](Cell p) { return !eta.contains(p); });
}
void Memory::sync_body(const std::vector<unswbc::DragonPart>& parts) {
    const Cell head = cell(ct_.get_position());
    const int length = ct_.get_length();
    if (!body.empty() && body.front() == head && static_cast<int>(body.size()) == length) {
        last_head = head;
        return;
    }
    std::map<Cell, Direction> facing_at;
    facing_at.insert_or_assign(head, ct_.get_dir());
    for (const auto& part : parts) facing_at.insert_or_assign(cell(part.get_position()), part.get_dir());
    std::vector<Cell> chain{head};
    Cell p = head;
    Direction facing = ct_.get_dir();
    // Intentionally retain the original grid-based reconstruction, including its
    // limitation when a body spans a portal.
    for (int i = 0; i < length - 1; ++i) {
        const Cell next = step(p, facing.get_opposite());
        chain.push_back(next);
        if (auto it = facing_at.find(next); it != facing_at.end()) {
            facing = it->second; p = next; continue;
        }
        auto it = std::find(body.begin(), body.end(), p);
        if (it != body.end() && it + 1 != body.end() && *(it + 1) == next) {
            p = next; continue;
        }
        break;
    }
    if (static_cast<int>(chain.size()) < length && !body.empty() && body.front() == head) {
        const auto begin = chain.size();
        for (std::size_t i = begin; i < body.size(); ++i) {
            if (contains(chain, body[i])) continue;
            chain.push_back(body[i]);
            if (static_cast<int>(chain.size()) == length) break;
        }
    }
    if (static_cast<int>(chain.size()) > length) chain.resize(length);
    body = std::move(chain);
    last_head = head;
}
Transition Memory::transit(Cell p, Direction d) const {
    unswbc::EdgeType kind;
    int id;
    if (const auto* live = ct_.get_tile(position(p))) {
        const auto& e = live->get_edge(d);
        kind = e.get_edge_type(); id = e.get_portal_id();
    } else {
        const auto it = edges.find({p.first, p.second, direction_index(d)});
        if (it == edges.end()) return {TransitKind::Unknown, step(p, d)};
        kind = it->second.kind; id = it->second.portal_id;
    }
    if (kind == unswbc::EdgeType::KELP) return {TransitKind::Kelp, std::nullopt};
    if (kind == unswbc::EdgeType::PORTAL) {
        const auto dest = portal_destination(p, d, id);
        return {dest ? TransitKind::Portal : TransitKind::UnknownPortal, dest};
    }
    return {TransitKind::Empty, step(p, d)};
}
std::optional<Cell> Memory::portal_destination(Cell p, Direction d, int id) const {
    const auto entry = canonical_edge(p, d);
    const auto found = portal_observations.find(id);
    if (found == portal_observations.end()) return std::nullopt;
    const auto ds = directions();
    for (auto [x, y, di] : found->second) {
        const Cell other{x, y};
        const Direction od = ds.at(di);
        if (canonical_edge(other, od) == entry) continue;
        if (od == d) return step(other, od);
        if (od == d.get_opposite()) return other;
    }
    return std::nullopt;
}
void Memory::apply_moves(const Moves& moves) {
    auto next_body = body;
    auto next_pearls = pearls;
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (next_body.empty()) break;
        const auto t = transit(next_body.front(), moves[i]);
        if (!t.destination || t.kind == TransitKind::Kelp) break;
        if (i > 0 && next_body.size() <= 2) break;
        const bool ate = next_pearls.contains(*t.destination);
        next_body.insert(next_body.begin(), *t.destination);
        if (!ate && next_body.size() > 1) next_body.pop_back();
        if (i > 0 && next_body.size() > 2) next_body.pop_back();
        next_pearls.erase(*t.destination);
    }
    body = std::move(next_body);
    pearls = std::move(next_pearls);
    last_head = body.empty() ? std::nullopt : std::optional<Cell>(body.front());
}
void Memory::remember_split(int child_size) {
    if (static_cast<int>(body.size()) > child_size) {
        body.resize(body.size() - child_size);
        last_head = body.front();
    }
}
}
