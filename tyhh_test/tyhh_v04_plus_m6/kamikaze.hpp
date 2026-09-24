#pragma once

#include "sense.hpp"

#include <deque>
#include <optional>
#include <unordered_set>
#include <vector>

namespace xxqz {

struct DiveKey {
    int x = 0;
    int y = 0;
    bool known = false;
    std::vector<Position> body;
    bool operator==(DiveKey const& other) const {
        return x == other.x && y == other.y && known == other.known && body == other.body;
    }
};

struct DiveKeyHash {
    std::size_t operator()(DiveKey const& key) const {
        std::size_t hash = (static_cast<std::size_t>(key.x) << 16) ^ static_cast<std::size_t>(key.y);
        hash ^= key.known ? 0x9e3779b97f4a7c15ULL : 0ULL;
        for (Position const& part : key.body)
            hash ^= PositionHash{}(part) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};

inline std::optional<std::vector<Position>> advance_body(std::vector<Position> const& body, Position dest, bool ate, int step_index) {
    if (step_index > 0 && static_cast<int>(body.size()) <= 2)
        return std::nullopt;
    std::vector<Position> updated;
    updated.reserve(body.size() + 1);
    updated.push_back(dest);
    updated.insert(updated.end(), body.begin(), body.end());
    if (!ate && !updated.empty())
        updated.pop_back();
    if (step_index > 0) {
        if (updated.size() <= 1)
            return std::nullopt;
        updated.pop_back();
    }
    return updated;
}

inline std::optional<std::vector<Direction>> find_dive(Controller const& ct, View const& view) {
    int length = ct.get_length();
    if (length > MISSILE_MAX_LENGTH || length < 2)
        return std::nullopt;
    if (ct.get_unit_count() < 2)
        return std::nullopt;

    std::unordered_set<int> targets;
    for (Enemy const& enemy : view.enemies) {
        if (enemy.has_head && worth_killing(enemy.visible, length))
            targets.insert(enemy.id);
    }
    if (targets.empty())
        return std::nullopt;

    struct Node {
        Position pos;
        std::vector<Direction> path;
        std::optional<std::vector<Position>> body;
    };

    std::optional<std::vector<Position>> start_body;
    if (view.body_complete)
        start_body = view.body;

    std::deque<Node> queue;
    queue.push_back(Node{view.head, {}, start_body});
    std::unordered_set<DiveKey, DiveKeyHash> seen;
    seen.insert(DiveKey{view.head.x, view.head.y, start_body.has_value(), start_body.value_or(std::vector<Position>{})});

    int expanded = 0;
    int max_steps = length - 1;
    while (!queue.empty() && expanded < 400) {
        Node node = std::move(queue.front());
        queue.pop_front();
        expanded++;
        if (static_cast<int>(node.path.size()) >= max_steps)
            continue;
        if (!node.path.empty() && !view.body_complete)
            continue;
        if (!node.path.empty() && node.body && static_cast<int>(node.body->size()) <= 2)
            continue;
        auto cell = view.snap.find(node.pos);
        if (cell == view.snap.end())
            continue;
        for (int index = 0; index < 4; index++) {
            if (cell->second.edges[static_cast<std::size_t>(index)] != EdgeType::EMPTY)
                continue;
            Direction direction = directions()[static_cast<std::size_t>(index)];
            Position nxt = node.pos.add_dir(direction);
            auto info = view.snap.find(nxt);
            if (info == view.snap.end())
                continue;
            if (info->second.owner == Owner::Foe && info->second.is_head && targets.count(info->second.dragon_id)) {
                auto path = node.path;
                path.push_back(direction);
                return path;
            }
            if (info->second.owner != Owner::Empty)
                continue;
            if (node.body && contains_pos(*node.body, nxt))
                continue;
            if (!view.body_complete)
                continue;
            auto new_body = advance_body(*node.body, nxt, info->second.pearl, static_cast<int>(node.path.size()));
            if (!new_body)
                continue;
            DiveKey key{nxt.x, nxt.y, true, *new_body};
            if (!seen.insert(key).second)
                continue;
            auto path = node.path;
            path.push_back(direction);
            queue.push_back(Node{nxt, std::move(path), std::move(new_body)});
        }
    }
    return std::nullopt;
}

}  // namespace xxqz
