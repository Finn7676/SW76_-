#pragma once

#include "world.hpp"

namespace bot {

class Topology {
    GameConfig const &config;
    WorldModel &world;

  public:
    Topology(GameConfig const &config, WorldModel &world) : config(config), world(world) {}
    uint8_t edgeTypeAt(int key) const { return world.edgeT[key]; }
    int portalIdAt(int key) const { return world.edgePid[key]; }
    void resolvePendingLanding(int here) {
        if (world.pendingPortalKey < 0) return;
        world.learnedExit[world.pendingPortalKey] = here;
        world.pendingPortalKey = -1;
    }
    inline int cid(int x, int y) const { return y * config.width + x; }
    inline int cx(int c) const { return c % config.width; }
    inline int cy(int c) const { return c / config.width; }
    inline int geo(int c, int d) const {
        int x = cx(c) + DX[d], y = cy(c) + DY[d];
        if (x < 0)
            x += config.width;
        else if (x >= config.width)
            x -= config.width;
        if (y < 0)
            y += config.height;
        else if (y >= config.height)
            y -= config.height;
        return cid(x, y);
    }
    inline int wrapd(int a, int m) const {
        a = a < 0 ? -a : a;
        a %= m;
        return std::min(a, m - a);
    }
    inline int manh(int a, int b) const {
        return wrapd(cx(a) - cx(b), config.width) + wrapd(cy(a) - cy(b), config.height);
    }
    inline int cheb(int a, int b) const {
        return std::max(wrapd(cx(a) - cx(b), config.width), wrapd(cy(a) - cy(b), config.height));
    }
    inline int canon(int c, int d) const { return (d == 1 || d == 2) ? c * 4 + d : geo(c, d) * 4 + (d ^ 2); }

    void addPortal(int pid, int key) {
        auto &v = world.portalEnds[pid];
        if (std::find(v.begin(), v.end(), key) != v.end()) return;
        v.push_back(key);
        world.portalKeys.push_back(key);
    }

    void setEdge(int c, int d, uint8_t t, int pid) {
        int n = geo(c, d);
        world.edgeT[c * 4 + d] = t;
        world.edgeT[n * 4 + (d ^ 2)] = t;
        world.edgePid[c * 4 + d] = pid;
        world.edgePid[n * 4 + (d ^ 2)] = pid;
        if (t == E_PORTAL) addPortal(pid, canon(c, d));
    }

    // Cell the head lands on when leaving c through side d, -1 if unknown.
    int portalExit(int c, int d) const {
        int k = c * 4 + d;
        if (world.learnedExit[k] >= 0) return world.learnedExit[k];
        auto it = world.portalEnds.find(world.edgePid[k]);
        if (it == world.portalEnds.end()) return -1;
        int ck = canon(c, d);
        for (int key : it->second) {
            if (key == ck) continue;
            int b = key / 4, o = key % 4;
            if (o != d && o != (d ^ 2)) continue;
            return o == d ? geo(b, o) : b;
        }
        return -1;
    }

    // Destination of a single step, -1 for kelp or an unresolved portal.
    inline int step(int c, int d) const {
        uint8_t e = world.edgeT[c * 4 + d];
        if (e == E_KELP) return -1;
        if (e == E_PORTAL) return portalExit(c, d);
        return geo(c, d);
    }
};

} // namespace bot
