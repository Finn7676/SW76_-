#pragma once
#include "coordination.hpp"

namespace bot {
struct RegionDensity {
    std::vector<int> cells;
    int dragons=0, capacity=1;
    bool crowded=false, shouldLeave=false;
    int area() const {return int(cells.size());}
};
struct DensityRoute {int dir=-1, distance=INF, goal=-1;};
// Virtual kelp closes gaps for area measurement only. Physical routing always
// uses Topology::step, never these inferred walls.
class DensityModel {
    GameConfig const &config;
    TurnState const &turn;
    Topology const &topology;
    OccupancyModel const &occupancy;
    Coordination const &coordination;
    std::vector<uint8_t> virtualWall;
    bool ready=false;
    bool edgeVisible(int c,int d) const {
        return occupancy.visible(c)||occupancy.visible(topology.geo(c,d));
    }
    int kelpAlong(int c,int d,int tangent) const {
        // Stop at the current vision boundary, not remembered remote kelp.
        for(int i=1;i<Constants::VISION_SIZE;++i) {
            c=topology.geo(c,tangent);
            if(!edgeVisible(c,d))break;
            if(topology.edgeTypeAt(c*4+d)==E_KELP)return c;
        }
        return -1;
    }
    void prepare() {
        if(ready)return;
        ready=true;std::fill(virtualWall.begin(),virtualWall.end(),0);
        for(int i=0;i<turn.visibleCount;++i) {
            int c=turn.visibleCells[i];
            for(int d=0;d<4;++d) {
                if(topology.edgeTypeAt(c*4+d)!=E_OPEN)continue;
                int tangent=(d%2==0)?1:0;
                int a=kelpAlong(c,d,tangent),b=kelpAlong(c,d,tangent^2);
                if(a<0||b<0||a==b||a==c||b==c)continue;
                virtualWall[c*4+d]=1;
                virtualWall[topology.geo(c,d)*4+(d^2)]=1;
            }
        }
    }
    bool terrainOpen(int c,int d) const {
        return topology.edgeTypeAt(c*4+d)==E_OPEN;
    }
  public:
    DensityModel(GameConfig const &config,TurnState const &turn,Topology const &topology,
                 OccupancyModel const &occupancy,Coordination const &coordination)
        :config(config),turn(turn),topology(topology),occupancy(occupancy),coordination(coordination),
         virtualWall(config.cells*4,0){}
    void reset(){ready=false;}
    bool closesGap(int c,int d){prepare();return virtualWall[c*4+d]!=0;}
    RegionDensity estimate(int start) {
        prepare();RegionDensity result;
        if(start<0||!occupancy.visible(start))return result;
        std::vector<uint8_t> inside(config.cells,0);result.cells.push_back(start);inside[start]=1;
        for(size_t i=0;i<result.cells.size();++i) {
            int c=result.cells[i];
            for(int d=0;d<4;++d) {
                if(!terrainOpen(c,d)||virtualWall[c*4+d])continue;
                int q=topology.geo(c,d);
                if(!occupancy.visible(q)||inside[q])continue;
                inside[q]=1;result.cells.push_back(q);
            }
        }
        // Head evidence decides which room a dragon belongs to. A body-only
        // sighting counts once conservatively; body length is not bot count.
        std::map<int,bool> residents;
        for(auto const &part:turn.parts)if(inside[part.c])residents[part.id]=part.mine;
        for(auto const &part:turn.parts)if(part.head&&!inside[part.c])residents.erase(part.id);
        if(inside[turn.here])residents[turn.myId]=true;
        result.dragons=int(residents.size());
        result.capacity=std::max(1,result.area()/V5_DENSITY_CELLS_PER_DRAGON);
        result.crowded=result.dragons>result.capacity;
        int enemyCount=0,ownRank=0;
        for(auto const &[id,mine]:residents) {
            if(!mine)++enemyCount;
            else if(id<turn.myId)++ownRank;
        }
        // Stable IDs retain the allowed residents; only excess friendly bots
        // evacuate, rather than all chasing the same narrow exit together.
        int friendlySlots=std::max(0,result.capacity-enemyCount);
        result.shouldLeave=result.crowded&&inside[turn.here]&&ownRank>=friendlySlots;
        return result;
    }
    std::vector<uint8_t> mask(std::vector<int> const &cells) const {
        std::vector<uint8_t> result(config.cells,0);
        for(int c:cells)if(c>=0&&c<config.cells)result[c]=1;
        return result;
    }
    // Ordinary moves only: density relief does not spend length on a sprint.
    // Try each safe first move independently, without pearl-reward tie breaks.
    template<class Allowed> DensityRoute exitRoute(std::vector<int> const &room,Allowed allowed) const {
        auto inside=mask(room);DensityRoute best;int bestRisk=INT_MAX,bestPearls=INT_MAX;
        for(int first=0;first<4;++first) {
            if(!allowed(Action::move(first))||coordination.portalHeld(turn.here,first)||
               (topology.edgeTypeAt(turn.here*4+first)==E_PORTAL&&
                coordination.portalBusy(topology.portalIdAt(turn.here*4+first))))continue;
            int start=topology.step(turn.here,first);
            if(start<0)continue; // unknown portal landing is not proven relief
            if(occupancy.blockedAt(start,1))continue;
            std::vector<int> queue{start},distance(config.cells,-1),growth(config.cells,0);
            distance[start]=1;growth[start]=int(occupancy.visible(start)&&occupancy.hasPearl(start));
            int risk=coordination.stepRisk(turn.here,first);
            for(size_t i=0;i<queue.size();++i) {
                int p=queue[i],steps=distance[p];
                if(steps>best.distance)break;
                if(!inside[p]) {
                    if(steps<best.distance||(steps==best.distance&&(risk<bestRisk||
                       (risk==bestRisk&&growth[p]<bestPearls)))) {
                        best={first,steps,p};bestRisk=risk;bestPearls=growth[p];
                    }
                    continue;
                }
                if(steps>=best.distance)continue;
                for(int d=0;d<4;++d) {
                    int type=topology.edgeTypeAt(p*4+d);
                    if((type!=E_OPEN&&type!=E_PORTAL)||coordination.portalHeld(p,d)||
                       (type==E_PORTAL&&coordination.portalBusy(topology.portalIdAt(p*4+d))))continue;
                    int q=topology.step(p,d),next=steps+1;
                    if(q<0||q==turn.here||occupancy.blockedAt(q,next,growth[p]))continue;
                    int eats=growth[p]+int(occupancy.visible(q)&&occupancy.hasPearl(q));
                    if(distance[q]>=0) {
                        if(distance[q]==next&&eats<growth[q])growth[q]=eats;
                        continue;
                    }
                    distance[q]=next;growth[q]=eats;queue.push_back(q);
                }
            }
        }
        return best;
    }
    // Geometric approach distances still locate a temporarily body-blocked
    // exit. They never make its occupied cell physically traversable.
    std::vector<int> exitDistances(std::vector<int> const &room) const {
        auto inside=mask(room);std::vector<int> distance(config.cells,INF),queue;
        for(int c:room)for(int d=0;d<4;++d) {
            int type=topology.edgeTypeAt(c*4+d),q=topology.step(c,d);
            if((type!=E_OPEN&&type!=E_PORTAL)||q<0||inside[q]||coordination.portalHeld(c,d)||
               (type==E_PORTAL&&coordination.portalBusy(topology.portalIdAt(c*4+d))))continue;
            if(distance[c]==0)continue;
            distance[c]=0;queue.push_back(c);
        }
        for(size_t i=0;i<queue.size();++i)for(int d=0;d<4;++d) {
            int p=queue[i],q=topology.step(p,d);
            if(!terrainOpen(p,d)||q<0||!inside[q]||distance[q]!=INF)continue;
            distance[q]=distance[p]+1;queue.push_back(q);
        }
        return distance;
    }
};
} // namespace bot
