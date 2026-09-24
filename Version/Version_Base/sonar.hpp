#pragma once

#include "topology.hpp"
#include "occupancy.hpp"

namespace bot {

class Sonar {
    GameConfig const &config;
    TurnState const &turn;
    WorldModel &world;
    Topology &topology;
    OccupancyModel const &occupancy;

  public:
    Sonar(GameConfig const &config, TurnState const &turn, WorldModel &world, Topology &topology,
          OccupancyModel const &occupancy)
        : config(config), turn(turn), world(world), topology(topology), occupancy(occupancy) {}

    static constexpr uint32_t SALT = 0x5A17C3E1u;
    uint32_t tagOf(uint32_t low) { return ((low ^ SALT) * 0x9E3779B1u) >> 28; }
    uint32_t pack(int type, int c, int payload) {
        uint32_t low = (uint32_t)topology.cx(c) | ((uint32_t)topology.cy(c) << 6) |
                       ((uint32_t)(payload & 0x3FFF) << 12) | ((uint32_t)type << 26);
        return low | (tagOf(low) << 28);
    }
    enum { M_PEARL = 0, M_ENEMY = 1, M_PORTAL = 2 };

    void receive(Controller const &ct) {
        for (auto raw64 : ct.get_sonar_messages()) {
            if (raw64 > 0xFFFFFFFFull) continue;
            uint32_t raw = (uint32_t)raw64, low = raw & 0x0FFFFFFFu;
            if ((raw >> 28) != tagOf(low)) continue;
            int x = low & 63, y = (low >> 6) & 63, payload = (low >> 12) & 0x3FFF, type = (low >> 26) & 3;
            if (x >= config.width || y >= config.height) continue;
            int c = topology.cid(x, y);
            if (type == M_PEARL) {
                int due = payload & 511;
                if (world.pearlInfo[c] < turn.round - 1) {
                    world.pearlDue[c] = due;
                    world.pearlInfo[c] = turn.round - 1;
                }
            } else if (type == M_ENEMY) {
                int r = payload & 511, l = payload >> 9;
                if (turn.round - r > 6 || r > turn.round) continue;
                world.sightings.push_back({c, r, l});
            } else if (type == M_PORTAL) {
                int d = (payload & 1) ? 2 : 1, pid = payload >> 1;
                if (topology.edgeTypeAt(c * 4 + d) != E_PORTAL) topology.setEdge(c, d, E_PORTAL, pid);
            }
        }
        world.sightings.erase(std::remove_if(world.sightings.begin(), world.sightings.end(),
                                             [&](Sighting const &s) { return turn.round - s.round > 6; }),
                              world.sightings.end());
        if (world.sightings.size() > 24) world.sightings.erase(world.sightings.begin(), world.sightings.end() - 24);
    }

    uint32_t chooseMessage() {
        if (!turn.enemyHeads.empty()) {
            int best = 0;
            for (size_t i = 1; i < turn.enemyHeads.size(); ++i)
                if (turn.enemyHeadLen[i] > turn.enemyHeadLen[best]) best = (int)i;
            return pack(M_ENEMY, turn.enemyHeads[best],
                        (turn.round & 511) | (std::min(turn.enemyHeadLen[best], 31) << 9));
        }
        ++world.sonarCursor;
        if (!world.portalKeys.empty() && (world.sonarCursor & 1)) {
            int key = world.portalKeys[(world.sonarCursor / 2) % world.portalKeys.size()];
            int c = key / 4, d = key % 4;
            int pid = topology.portalIdAt(c * 4 + d);
            if (pid >= 0 && pid < 8192) return pack(M_PORTAL, c, (pid << 1) | (d == 2 ? 1 : 0));
        }
        // a pearl tile in view that will be ripe soon
        int bestC = -1, bestDue = INF;
        for (int i = 0; i < turn.visibleCount; ++i) {
            int c = turn.visibleCells[i];
            int due = world.pearlDue[c];
            if (due < 0 || due >= INF || due > 511) continue;
            if (due < turn.round && !occupancy.hasPearl(c)) continue;
            int key = due * 64 + ((c * 37 + world.sonarCursor * 11) & 63);
            if (key < bestDue) {
                bestDue = key;
                bestC = c;
            }
        }
        if (bestC >= 0) return pack(M_PEARL, bestC, world.pearlDue[bestC]);
        return 0;
    }
};

} // namespace bot
