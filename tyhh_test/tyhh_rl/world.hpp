#pragma once
#include "types.hpp"
#include <array>
#include <unordered_map>

namespace champion {
// Edges: -3 unobserved, -2 empty, -1 kelp, >=0 portal ID.
struct Cell {
    std::array<int,4> edges{-3,-3,-3,-3};
    int seen = -10000, due = -1, visited = -10000;
    bool pearl = false, head = false;
    int occupant = -1, facing = 0;
    char team = '?';
};
struct Transition {
    int cell = -1; // -1 blocked, -2 unresolved portal
    bool portal = false, known = false;
};
class WorldModel {
public:
    int width, height, round = 0, id = -1, length = 0, units = 0, unitLimit = 0, here = 0;
    char team = 'A';
    std::vector<Cell> cells;
    std::vector<int> body; // Head first; -1 represents a segment not yet reconstructed.
    std::vector<Enemy> enemies;
    std::unordered_map<int,std::vector<int>> portals;
    unsigned symmetryMask = 7; // x reflection, y reflection, half turn
    int previousSplit = -10000;

    explicit WorldModel(const unswbc::Game& game) : width(game.width), height(game.height), cells(width*height) {}
    int index(Position p) const { return p.y*width+p.x; }
    Position position(int p) const { return {p%width,p/width}; }
    int adjacent(int p, int d) const {
        return ((p/width+DY[d]+height)%height)*width+(p%width+DX[d]+width)%width;
    }
    int distance(int a, int b) const {
        int dx = std::abs(a%width-b%width), dy = std::abs(a/width-b/width);
        return std::min(dx,width-dx)+std::min(dy,height-dy);
    }
    int edgeKey(int p, int d) const {
        if (d == 2 || d == 1) p = adjacent(p,d);
        return p*2 + (d%2);
    }
    Transition step(int p, int d) const {
        if (p < 0 || p >= int(cells.size())) return {};
        int e = cells[p].edges[d];
        if (e == -1) return {};
        if (e < 0) return {adjacent(p,d),false,e == -2};
        auto it = portals.find(e);
        if (it == portals.end() || it->second.size() != 2) return {-2,true,true};
        int key = edgeKey(p,d), other = -1;
        for (int k : it->second) if (k != key) other = k;
        if (other < 0 || (other%2) != (d%2)) return {-2,true,true};
        int q = other/2;
        if (d == 0 || d == 3) q = adjacent(q,d);
        return {q,true,true};
    }
    bool occupied(int p) const { return cells[p].seen == round && cells[p].occupant >= 0; }
    bool isKnownSafe(Position p) const {
        if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height) return false;
        int q=index(p); return cells[q].seen == round && !occupied(q);
    }
    std::optional<Position> portalDestination(int portalId, Position from, Direction direction) const {
        if(from.x<0 || from.y<0 || from.x>=width || from.y>=height) return std::nullopt;
        if (cells[index(from)].edges[dirIndex(direction)] != portalId) return std::nullopt;
        auto t=step(index(from),dirIndex(direction));
        return t.cell >= 0 ? std::optional<Position>(position(t.cell)) : std::nullopt;
    }
    // A portal ID alone is ambiguous: both endpoints and both entry sides are legal.
    std::vector<Position> portalDestination(int portalId) const {
        std::vector<Position> out;
        auto it=portals.find(portalId);
        if(it!=portals.end()) for(int k:it->second) out.push_back(position(k/2));
        return out;
    }
    int predictedSymmetry() const {
        return symmetryMask && !(symmetryMask&(symmetryMask-1)) ? int(symmetryMask) : 0;
    }
    double pearlConfidence(int p, int turns=0) const {
        const auto& c=cells[p];
        if (c.seen < 0) return 0;
        if (c.pearl) return c.seen==round ? 1.0 : std::max(0.12,0.85-(round-c.seen)*0.035);
        if(c.due>=round && c.due<=round+turns && c.occupant<0) return 0.6;
        if(c.due>=0 && c.due<round && c.occupant<0) return 0.2;
        return 0;
    }
    std::vector<int> distances(int start, bool blockBodies=true) const {
        std::vector<int> dist(cells.size(),-1), queue; queue.reserve(cells.size());
        dist[start]=0; queue.push_back(start);
        for(std::size_t i=0;i<queue.size();++i) for(int d=0;d<4;++d) {
            int q=step(queue[i],d).cell;
            if(q<0 || dist[q]>=0 || (blockBodies && occupied(q))) continue;
            dist[q]=dist[queue[i]]+1; queue.push_back(q);
        }
        return dist;
    }
    Target closestPearl() const {
        auto dist=distances(here); Target result; double best=-1;
        for(int p=0;p<int(cells.size());++p) if(dist[p]>0) {
            double s=pearlConfidence(p)/(dist[p]+0.7);
            if(s>best && s>0) {best=s;result={position(p),TargetKind::Pearl,s};}
        }
        return result;
    }
    Target bestSpawnSoon() const {
        auto dist=distances(here); Target result; double best=0;
        for(int p=0;p<int(cells.size());++p) if(dist[p]>0 && cells[p].due>=round) {
            int wait=cells[p].due-round;
            if(wait>12) continue;
            double s=0.55/(1+std::max(wait,dist[p])+std::abs(wait-dist[p]));
            if(s>best) {best=s;result={position(p),TargetKind::Spawn,s};}
        }
        return result;
    }
    void update(const unswbc::Controller& ct, const unswbc::Game& game) {
        round=game.round_num; id=ct.get_id(); length=ct.length; units=ct.unit_count;
        unitLimit=ct.unit_limit; team=ct.get_team().value; here=index(ct.get_position());
        enemies.clear();
        for(const auto& t:ct.get_tiles()) {
            int p=index(t.position); auto& c=cells[p];
            c.seen=round; c.pearl=t.pearl; c.due=t.pearl_time<0?-1:round+t.pearl_time;
            c.occupant=-1; c.head=false; c.team='?';
            if(auto part=t.get_dragon()) {
                c.occupant=part->get_id(); c.head=part->is_head(); c.team=part->get_team().value;
                c.facing=dirIndex(part->get_dir());
                if(c.head && c.team!=team) enemies.push_back({c.occupant,t.position,0});
            }
            for(int d=0;d<4;++d) {
                const auto& e=t.edges[d]; int v=e.is_portal()?e.get_portal_id():(e.is_passable()?-2:-1);
                c.edges[d]=v; cells[adjacent(p,d)].edges[(d+2)%4]=v;
                if(v>=0) {
                    auto& ends=portals[v]; int key=edgeKey(p,d);
                    if(std::find(ends.begin(),ends.end(),key)==ends.end()) ends.push_back(key);
                }
            }
        }
        for(auto& e:enemies) for(const auto& t:ct.get_tiles())
            if(t.dragon_part && t.dragon_part->dragon_id==e.id) ++e.visibleLength;
        // Predicted body remains exact between turns, including across portal links.
        bool consistent=!body.empty() && (body[0]==here || body[0]==-1) && int(body.size())==length;
        if(consistent) {
            body[0]=here;
            for(int p:body) if(p>=0 && cells[p].seen==round && cells[p].occupant!=id) consistent=false;
        }
        if(!consistent) {
            body.assign(1,here);
            while(int(body.size())<length) {
                int next=-1;
                for(const auto& t:ct.get_tiles()) {
                    int p=index(t.position); const auto& c=cells[p];
                    if(c.occupant!=id || c.head || std::find(body.begin(),body.end(),p)!=body.end()) continue;
                    if(step(p,c.facing).cell==body.back()) { next=p;break; }
                }
                if(next<0) break;
                body.push_back(next);
            }
            body.resize(length,-1);
        }
        cells[here].visited=round;
        // Compare static edge types only. Portal IDs may be relabelled by symmetry.
        for(const auto& t:ct.get_tiles()) {
            int p=index(t.position); auto pos=t.position;
            for(int bit=0;bit<3;++bit) if(symmetryMask&(1u<<bit)) {
                Position mirror{bit==1?pos.x:width-1-pos.x,bit==0?pos.y:height-1-pos.y};
                int q=index(mirror); if(cells[q].seen<0) continue;
                for(int d=0;d<4;++d) {
                    int md=d;
                    if(bit!=1 && d%2==1) md=(d+2)%4;
                    if(bit!=0 && d%2==0) md=(d+2)%4;
                    int a=cells[p].edges[d],b=cells[q].edges[md];
                    if(a>=0) a=0;
                    if(b>=0) b=0;
                    if(a!=-3 && b!=-3 && a!=b) symmetryMask&=~(1u<<bit);
                }
            }
        }
    }
    void remember(const Action& action) {
        if(action.kind==ActionKind::Split) {
            if(action.childSize>=2 && action.childSize<=int(body.size())-2) body.resize(body.size()-action.childSize);
            previousSplit=round; return;
        }
        int p=here;
        for(std::size_t i=0;i<action.directions.size();++i) {
            auto t=step(p,action.directions[i]);
            if(t.cell<0) {body.insert(body.begin(),-1); if(!body.empty())body.pop_back(); break;}
            p=t.cell; bool eat=cells[p].pearl;
            body.insert(body.begin(),p);
            if(!eat) body.pop_back();
            if(i>0 && !body.empty()) body.pop_back();
            if(eat) cells[p].pearl=false;
            cells[p].visited=round;
        }
    }
};
}
