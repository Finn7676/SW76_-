#pragma once

#include "view.hpp"
#include "debug.hpp"

namespace xxqz {

inline int enemy_escapes(Enemy const& enemy, View const& view) {
    record(&DebugStats::escape_calls);
    auto head = view.snap.find(enemy.head);
    if (head == view.snap.end())
        return 4;
    int count = 0;
    for (int index = 0; index < 4; ++index) {
        record(&DebugStats::escape_edges);
        EdgeType edge = head->second.edges[index];
        if (edge == EdgeType::KELP)
            continue;
        // An unknown portal exit is a possible escape, never evidence of a trap.
        if (edge == EdgeType::PORTAL) {
            ++count;
            continue;
        }
        auto dest = view.snap.find(enemy.head.add_dir(directions()[index]));
        if (dest == view.snap.end() || dest->second.owner == Owner::Empty)
            ++count;
    }
    return count;
}

// Rebuilt from the completed current snapshot. No persistent or stale facts.
inline void build_threats(View const& view) {
    record(&DebugStats::threat_builds);
    view.threats = ThreatCache{};
    auto& cache = view.threats;
    for (auto const& step : view.safe)
        cache.candidate_mask |= static_cast<std::uint8_t>(1U << dir_index(step.direction));
    if (!cache.candidate_mask) {
        cache.ready = true;
        return;
    }
    std::array<Position, 4> destinations;
    for (int i = 0; i < 4; ++i)
        destinations[i] = view.head.add_dir(directions()[i]);
    for (Enemy const& enemy : view.enemies) {
        record(&DebugStats::threat_enemy_checks);
        if (!enemy.has_head)
            continue;
        auto head = view.snap.find(enemy.head);
        if (head == view.snap.end())
            continue;
        EnemyThreat threat;
        threat.enemy_id = enemy.id;
        threat.head = enemy.head;
        threat.visible_length = enemy.visible;
        for (int edge = 0; edge < 4; ++edge) {
            record(&DebugStats::threat_edges);
            if (head->second.edges[edge] != EdgeType::EMPTY)
                continue;
            Direction direction = directions()[edge];
            Position dest = enemy.head.add_dir(direction);
            for (int candidate = 0; candidate < 4; ++candidate) {
                auto bit = static_cast<std::uint8_t>(1U << candidate);
                if (!(cache.candidate_mask & bit) || dest != destinations[candidate])
                    continue;
                threat.reachable_mask |= bit;
                // Preserve the legacy last-direction-wins rule on tiny tori.
                if (direction == head->second.facing)
                    threat.forward_mask |= bit;
                else
                    threat.forward_mask &= static_cast<std::uint8_t>(~bit);
            }
        }
        if (threat.reachable_mask) {
            threat.escapes = enemy_escapes(enemy, view);
            for (int candidate = 0; candidate < 4; ++candidate) {
                auto bit = static_cast<std::uint8_t>(1U << candidate);
                if (!(threat.reachable_mask & bit))
                    continue;
                auto& bucket = cache.candidates[candidate];
                ++bucket.reachable[threat.escapes];
                if (threat.forward_mask & bit)
                    ++bucket.forward[threat.escapes];
            }
        }
        cache.enemies.push_back(threat);
    }
    cache.ready = true;
}

}  // namespace xxqz
