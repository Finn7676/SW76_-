#pragma once
#include "world.hpp"
#include <numeric>

namespace champion {
class Navigator {
    struct Node {
        std::vector<int> body, eaten;
        Action action;
        int p=0, time=0;
        double gain=0, risk=0, novelty=0, rank=0;
    };
    WorldModel& w;
    std::vector<std::array<int,4>> graph;
    std::vector<double> danger;
    std::vector<unsigned char> obstacles;
    std::vector<int> route;
    Target target;
    int startDistance=0;
    bool cautious=true;

    bool contains(const std::vector<int>& v,int p) const {return std::find(v.begin(),v.end(),p)!=v.end();}
    bool fixedBlocked(int p) const { return obstacles[p]!=0; }
    bool advance(Node& s,int d,int time,bool extra) const {
        if(extra && s.body.size()<=2) return false;
        int q=graph[s.p][d];
        if(q<0 || fixedBlocked(q) || contains(s.body,q)) return false;
        const auto& c=w.cells[q];
        double food=contains(s.eaten,q)?0:w.pearlConfidence(q,time);
        // At time zero, only a currently present pearl can prevent the tail moving.
        bool grow=time==0 ? c.pearl : food>=0.5;
        s.body.insert(s.body.begin(),q);
        if(!grow) s.body.pop_back();
        if(extra) s.body.pop_back();
        if(food>0) {s.eaten.push_back(q); s.gain+=12.0*food/(1.0+0.13*time);}
        if(extra) s.gain-=14.5;
        if(w.cells[q].seen<0) s.novelty+=0.25/(1+time);
        if(w.round-c.visited<12) s.novelty-=0.8/(1+time*0.25);
        if(c.seen!=w.round && time==0) s.risk+=c.seen<0?22:9;
        s.p=q; s.time=time;
        return true;
    }
    int exits(const Node& s) const {
        int n=0;
        for(int q:graph[s.p]) if(q>=0 && !fixedBlocked(q) && !contains(s.body,q)) ++n;
        return n;
    }
    struct Region { int area=0, release=100000, tailDistance=100000; };
    Region region(const Node& s,int limit=Config::floodLimit) const {
        std::vector<int> release(w.cells.size(),0), dist(w.cells.size(),-1);
        for(int i=0;i<int(s.body.size());++i) if(s.body[i]>=0) release[s.body[i]]=int(s.body.size())-i;
        std::vector<int> queue; queue.reserve(std::min(limit,int(w.cells.size())));
        queue.push_back(s.p); dist[s.p]=0; Region result;
        for(std::size_t i=0;i<queue.size() && int(queue.size())<limit;++i) {
            int p=queue[i];
            for(int q:graph[p]) if(q>=0 && !fixedBlocked(q)) {
                if(release[q]) {
                    if(q!=s.p)result.release=std::min(result.release,release[q]);
                    if(release[q]<=3)result.tailDistance=std::min(result.tailDistance,dist[p]+1);
                    continue;
                }
                if(dist[q]>=0)continue;
                dist[q]=dist[p]+1;queue.push_back(q);
            }
        }
        result.area=int(queue.size()); return result;
    }
    int space(const Node& s,int limit=Config::floodLimit) const {return region(s,limit).area;}
    double confinement(const Node& s) const {
        auto r=region(s);
        // A small region is safe only if a sufficiently old body segment can
        // vacate before we exhaust it. Penalise pockets bordered by the neck.
        double need=std::min(double(Config::floodLimit-1),8.0+s.body.size()*0.9);
        double penalty=std::max(0.0,need-r.area)*2.0;
        if(r.tailDistance<100000)penalty*=0.3;
        if(r.release>r.area && r.area<need)penalty+=120+4.0*(std::min(r.release,int(need))-r.area);
        return penalty;
    }
    double grade(const Node& s) const {
        int dist=route.empty()?-1:route[s.p];
        double progress=dist<0 ? -5 : 1.5*(startDistance-dist);
        int e=exits(s);
        return s.gain-s.risk+s.novelty+progress+(e>=2?1.8:e==1?-1.5:-60);
    }
    void makeGraph() {
        obstacles.assign(w.cells.size(),0);
        for(int p=0;p<int(w.cells.size());++p) {
            const auto& c=w.cells[p];
            obstacles[p]=c.seen==w.round && c.occupant>=0 && (c.occupant!=w.id || !contains(w.body,p));
        }
        graph.resize(w.cells.size());
        for(int p=0;p<int(graph.size());++p) for(int d=0;d<4;++d) graph[p][d]=w.step(p,d).cell;
        danger.assign(w.cells.size(),0);
        // Every enemy acts once before our next turn, independent of ID. Its current
        // state already includes this round's moves of lower IDs.
        for(const auto& enemy:w.enemies) {
            if(w.round==unswbc::Constants::MAX_ROUNDS-1 && enemy.id<w.id) continue;
            int start=w.index(enemy.position);
            std::vector<int> dist(w.cells.size(),-1), queue{start};dist[start]=0;
            for(std::size_t i=0;i<queue.size();++i) {
                int p=queue[i]; if(dist[p]>=4) continue;
                for(int q:graph[p]) {
                    if(q<0 || dist[q]>=0 || w.occupied(q)) continue;
                    dist[q]=dist[p]+1;queue.push_back(q);
                    static constexpr double penalty[5]={0,90,36,13,4};
                    danger[q]+=penalty[dist[q]];
                }
            }
        }
    }
    void setTarget(Target t) {
        target=t;
        route.assign(w.cells.size(),-1);
        if(!t.valid() || t.position.x>=w.width || t.position.y>=w.height) {startDistance=0;return;}
        int end=w.index(t.position);
        // Reverse graph is essential: portals are directed transitions between tiles.
        std::vector<std::vector<int>> reverse(graph.size());
        for(int p=0;p<int(graph.size());++p) for(int q:graph[p]) if(q>=0 && !fixedBlocked(q)) reverse[q].push_back(p);
        std::vector<int> queue{end};route[end]=0;
        for(std::size_t i=0;i<queue.size();++i) for(int p:reverse[queue[i]]) if(route[p]<0) {
            route[p]=route[queue[i]]+1;queue.push_back(p);
        }
        startDistance=std::max(0,route[w.here]);
    }
    void generate(const Node& parent,int depth,std::vector<Node>& roots) const {
        for(int d=0;d<4;++d) {
            Node child=parent;
            if(!advance(child,d,0,depth>0)) continue;
            child.action.directions.push_back(d);child.action.safe=true;
            // Threat applies at the final sprint position, not to intermediate cells.
            Node ready=child;
            ready.risk+=danger[ready.p]*(cautious?1.0:0.65);
            ready.rank=grade(ready);
            roots.push_back(std::move(ready));
            if(depth+1<Config::maxSprint) generate(child,depth+1,roots);
        }
    }
    Action plan(Target t,bool safe) {
        cautious=safe;setTarget(t);
        Node start;start.body=w.body;start.p=w.here;
        std::vector<Node> roots;generate(start,0,roots);
        if(roots.empty()) return emergency();
        if(w.round==unswbc::Constants::MAX_ROUNDS-1) {
            // No future movement or room requirement remains after round 499.
            auto finalScore=[&](const Node& n) {return 100.0*(int(n.body.size())-w.length)-5*n.risk;};
            auto best=std::max_element(roots.begin(),roots.end(),[&](const Node& a,const Node& b){return finalScore(a)<finalScore(b);});
            Action a=best->action;a.score=finalScore(*best);return a;
        }
        for(auto& s:roots) {
            s.risk+=confinement(s)*0.5;
            s.rank=grade(s);
        }
        std::stable_sort(roots.begin(),roots.end(),[](const Node& a,const Node& b){return a.rank>b.rank;});
        if(roots.size()>16) roots.resize(16);
        std::vector<Node> beam=roots;
        // Bounded beam search simulates the body and consumed food for every step.
        for(int time=1;time<Config::horizon && w.round+time<unswbc::Constants::MAX_ROUNDS;++time) {
            std::vector<Node> next;next.reserve(beam.size()*3);
            for(const auto& node:beam) for(int d=0;d<4;++d) {
                Node child=node;
                if(!advance(child,d,time,false)) continue;
                if(time<=2) child.risk+=danger[child.p]*(time==1?0.10:0.025);
                child.rank=grade(child);
                next.push_back(std::move(child));
            }
            if(next.empty()) break;
            // Inspect space before pruning, so a food-rich trap cannot crowd all
            // escape trajectories out of the beam before terminal evaluation.
            if(time%2==0) for(auto& n:next) n.rank-=confinement(n)*0.7;
            std::stable_sort(next.begin(),next.end(),[](const Node& a,const Node& b){return a.rank>b.rank;});
            beam.clear();
            // Keep distinct trajectories without letting one root monopolise search.
            std::array<int,64> rootCounts{};
            for(auto& n:next) {
                int code=0; for(int d:n.action.directions)code=code*4+d+1;
                code%=64;
                if(rootCounts[code]>=12)continue;
                bool duplicate=false;
                for(const auto& kept:beam) if(n.p==kept.p && n.body==kept.body && n.eaten==kept.eaten && n.action.directions==kept.action.directions) {duplicate=true;break;}
                if(duplicate)continue;
                ++rootCounts[code];beam.push_back(std::move(n));
                if(beam.size()>=Config::beamWidth)break;
            }
        }
        for(auto& n:beam) {
            n.rank=grade(n)-confinement(n);
        }
        auto best=std::max_element(beam.begin(),beam.end(),[](const Node&a,const Node&b){return a.rank<b.rank;});
        Action result=best->action;result.score=best->rank;
        // If all first-step continuations are bad, staying put through a legal split
        // can preserve both a parent and a child instead of crashing immediately.
        return result;
    }
    Action emergency() const {
        if(w.length>=4 && w.units<w.unitLimit) return Action::split(2);
        // An unresolved portal is a last-resort escape, never a normal grid edge.
        for(int d=0;d<4;++d) if(graph[w.here][d]==-2) return Action::move(d);
        for(int d=0;d<4;++d) {
            int q=graph[w.here][d];
            if(q>=0 && w.cells[q].head && w.cells[q].team!=w.team) return Action::move(d);
        }
        return Action::move(0); // Physically trapped; every legal action is fatal.
    }
public:
    explicit Navigator(WorldModel& world):w(world){makeGraph();}
    Action moveToward(Target t) {return plan(t,false);}
    Action moveSafely(Target t) {return plan(t,true);}
    Action escape() {return plan({},true);}
    Action intercept(Enemy enemy) {
        if(w.units<=1 || enemy.visibleLength<w.length*2) return escape();
        // The first steps must survive exactly; only the final enemy-head hit is intentional.
        struct Attack {Node node;};
        std::vector<Node> frontier(1);frontier[0].body=w.body;frontier[0].p=w.here;
        for(int depth=0;depth<5;++depth) {
            std::vector<Node> next;
            for(auto node:frontier) for(int d=0;d<4;++d) {
                int q=graph[node.p][d];
                if(q==w.index(enemy.position) && !contains(node.body,q) && (depth==0 || node.body.size()>2)) {
                    node.action.directions.push_back(d);node.action.safe=true;node.action.score=1000;return node.action;
                }
                Node child=node;
                if(advance(child,d,0,depth>0)) {child.action.directions.push_back(d);next.push_back(std::move(child));}
            }
            frontier=std::move(next);
        }
        return escape();
    }
    double dangerAt(Position p) const {return danger[w.index(p)];}
    int availableSpace() const {Node n;n.p=w.here;n.body=w.body;return space(n);}
};
}
