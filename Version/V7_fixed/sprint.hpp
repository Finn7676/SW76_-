#pragma once
#include "body.hpp"
#include "coordination.hpp"

namespace bot {
struct SprintTrace {
    bool legal = false, collision = false;
    int head = -1;
    std::vector<int> segments, cells, eaten;
    int length() const { return int(segments.size()); }
};
struct SprintEscape {
    Action action;
    int clearance = -1, space = 0, length = 0;
    int risk = 3; // 0: no attack in complete known search, 1: uncertainty, 2: proven head attack
};
// Physical sprint simulation and the visible enemies' next-action reach.
// Paid steps check affordability first; collisions precede movement and tail removal.
// Free-step allowance is fixed from the length at action start.
class SprintModel {
    GameConfig const &config;
    TurnState const &turn;
    Topology const &topology;
    OccupancyModel const &occupancy;
    BodyMemory const &body;
    Coordination const &coordination;
    std::vector<int> enemyDistance;
    bool reachReady = false;

    std::vector<int> ownSegments() const {
        std::vector<int> result;
        for(int i=0;i<turn.length;++i)
            result.push_back(i==0?turn.here:body.cellAtTailOffset(turn.length-1-i));
        return result;
    }
    SprintTrace initial() const {
        SprintTrace result;result.legal=true;result.head=turn.here;result.segments=ownSegments();return result;
    }
    bool occupiedAfter(int c,SprintTrace const &state,std::vector<int> const &original) const {
        if(std::find(state.segments.begin(),state.segments.end(),c)!=state.segments.end())return true;
        if(!occupancy.occupied(c))return false;
        return !occupancy.isMine(c)||occupancy.dragonId(c)!=turn.myId||
               std::find(original.begin(),original.end(),c)==original.end();
    }
    SprintTrace advance(SprintTrace const &before,int d,int step,bool attack,
                        std::vector<int> const &original) const {
        SprintTrace next;
        if(!before.legal||before.collision||d<0||d>=4||
           (MovementRules::paidStep(turn.length,step)&&before.length()<=Constants::MIN_SIZE))return next;
        bool cost=MovementRules::paidStep(turn.length,step);
        int q=topology.step(before.head,d);
        // A known open cell outside the current 7x7 is still a legal sprint step.
        // Pearls and enemy bodies there are not visible, so this step does not eat.
        if(q<0||(!occupancy.visible(q)&&!topology.mapped(q))||
           std::find(before.segments.begin(),before.segments.end(),q)!=before.segments.end())return next;
        if(occupancy.occupied(q)) {
            bool releasedOwn=occupancy.isMine(q)&&occupancy.dragonId(q)==turn.myId&&
                std::find(original.begin(),original.end(),q)!=original.end();
            if(!releasedOwn) {
                if(!attack||!occupancy.enemyHeadAt(q))return next;
                next=before;next.head=q;next.segments=before.segments;next.cells.push_back(q);
                next.collision=true;return next;
            }
        }
        bool eats=occupancy.visible(q)&&occupancy.hasPearl(q)&&std::find(before.eaten.begin(),before.eaten.end(),q)==before.eaten.end();
        next=before;next.head=q;next.segments=before.segments;next.cells.push_back(q);
        if(eats)next.eaten.push_back(q);
        else next.segments.pop_back();
        next.segments.insert(next.segments.begin(),q);
        if(cost)next.segments.pop_back();
        return next;
    }
    int spaceAfter(SprintTrace const &state,std::vector<int> const &original,int cap=24) const {
        std::vector<uint8_t> seen(config.cells,0);std::vector<int> queue{state.head};seen[state.head]=1;
        for(size_t i=0;i<queue.size()&&int(queue.size())<cap;++i) {
            for(int d=0;d<4&&int(queue.size())<cap;++d) {
                int p=queue[i],q=topology.step(p,d);
                if(q<0||seen[q]||topology.edgeTypeAt(p*4+d)==E_UNKNOWN||
                   coordination.portalHeld(p,d)||
                   (topology.edgeTypeAt(p*4+d)==E_PORTAL&&coordination.portalBusy(topology.portalIdAt(p*4+d)))||
                   occupiedAfter(q,state,original))continue;
                seen[q]=1;queue.push_back(q);
            }
        }
        return int(queue.size());
    }
    void buildReach() {
        if(reachReady)return;
        reachReady=true;enemyDistance.assign(config.cells,INF);
        std::vector<uint8_t> region(config.cells,0);
        std::vector<int> unseenBudget(config.cells,-1);
        constexpr int maxSteps=8,maxStates=256;
        for(size_t index=0;index<turn.enemyHeads.size();++index) {
            int start=turn.enemyHeads[index];if(start<0||start>=config.cells)continue;
            int id=occupancy.dragonId(start);
            int exact=index<turn.enemyHeadExactLen.size()?turn.enemyHeadExactLen[index]:-1;
            int observed=index<turn.enemyHeadLen.size()?turn.enemyHeadLen[index]:2;
            int length=exact>=2?exact:std::max(observed,maxSteps+1);
            // Enumerate only eight steps, but do not treat that CPU bound as
            // the engine's movement limit. Longer or unknown paths are unsafe.
            int reachBound=exact>=2
                ? std::min(config.cells,std::max(maxSteps,MovementRules::freeSteps(length)+length-2+
                                               int(turn.visPearls.size())))
                : config.cells;
            std::vector<int> segments{start};
            while(int(segments.size())<length) {
                int predecessor=-1;
                for(auto const &part:turn.parts)
                    if(!part.mine&&!part.head&&part.id==id&&topology.step(part.c,part.facing)==segments.back()&&
                       std::find(segments.begin(),segments.end(),part.c)==segments.end()) {predecessor=part.c;break;}
                if(predecessor<0)break;
                segments.push_back(predecessor);
            }
            segments.resize(length,-1);
            auto original=segments;std::vector<int> eaten;int states=0;bool truncated=false;
            region[start]=1;
            auto visit=[&](auto &&self,int head,int steps,std::vector<int> const &chain)->void {
                bool cost=MovementRules::paidStep(length,steps);
                if(cost&&chain.size()<=2)return;
                if(steps>=maxSteps){truncated=true;return;}
                if(++states>maxStates){truncated=true;return;}
                for(int d=0;d<4;++d) {
                    uint8_t edge=topology.edgeTypeAt(head*4+d);
                    int q=topology.predictStep(head,d);
                    if(q<0||std::find(chain.begin(),chain.end(),q)!=chain.end())continue;
                    bool occupied=occupancy.occupied(q);
                    bool released=occupied&&occupancy.dragonId(q)==id&&!occupancy.isMine(q)&&
                        std::find(original.begin(),original.end(),q)!=original.end();
                    if(occupied&&!released&&occupancy.dragonId(q)==id)continue;
                    region[q]=1; // attacking another dragon is a reachable terminal collision
                    if(occupied&&!released)continue;
                    if(edge==E_UNKNOWN || !occupancy.visible(q)) {
                        // Unseen pearls may fund further moves. Beyond this
                        // frontier overestimate terrain reach for the remaining
                        // bounded steps instead of assuming the cell is empty.
                        unseenBudget[q]=std::max(unseenBudget[q],reachBound-steps-1);
                        continue;
                    }
                    bool pearl=occupancy.visible(q)&&occupancy.hasPearl(q)&&
                        std::find(eaten.begin(),eaten.end(),q)==eaten.end();
                    auto after=chain;if(!pearl)after.pop_back();after.insert(after.begin(),q);
                    if(cost)after.pop_back();
                    if(pearl)eaten.push_back(q);
                    self(self,q,steps+1,after);
                    if(pearl)eaten.pop_back();
                }
            };
            visit(visit,start,0,segments);
            if(truncated) {
                // Budget exhaustion must overestimate enemy reach, never make
                // an unexplored attack branch look safe. Ignore bodies/fuel here.
                std::vector<int> distance(config.cells,-1),queue{start};distance[start]=0;
                for(size_t i=0;i<queue.size();++i) {
                    int p=queue[i];region[p]=1;if(distance[p]>=reachBound)continue;
                    for(int d=0;d<4;++d) {
                        int q=topology.predictStep(p,d);if(q<0||distance[q]>=0)continue;
                        distance[q]=distance[p]+1;queue.push_back(q);
                    }
                }
            }
        }
        std::vector<int> uncertain;
        for(int c=0;c<config.cells;++c)if(unseenBudget[c]>=0)uncertain.push_back(c);
        for(size_t i=0;i<uncertain.size();++i) {
            int p=uncertain[i];region[p]=1;if(unseenBudget[p]<=0)continue;
            for(int d=0;d<4;++d) {
                int q=topology.predictStep(p,d);
                if(q<0||unseenBudget[q]>=unseenBudget[p]-1)continue;
                unseenBudget[q]=unseenBudget[p]-1;uncertain.push_back(q);
            }
        }
        // Multi-source terrain distance to the complete bounded reach region,
        // following kelp, map wrapping and learned portal landings.
        std::vector<int> queue;
        for(int c=0;c<config.cells;++c)if(region[c]){enemyDistance[c]=0;queue.push_back(c);}
        for(size_t i=0;i<queue.size();++i) {
            int p=queue[i];
            for(int d=0;d<4;++d) {
                int q=topology.predictStep(p,d);if(q<0||enemyDistance[q]!=INF)continue;
                enemyDistance[q]=enemyDistance[p]+1;queue.push_back(q);
            }
        }
    }
  public:
    SprintModel(GameConfig const &config,TurnState const &turn,Topology const &topology,
                OccupancyModel const &occupancy,BodyMemory const &body,Coordination const &coordination)
        :config(config),turn(turn),topology(topology),occupancy(occupancy),body(body),coordination(coordination){}
    void reset(){reachReady=false;}
    int distanceToEnemyReach(int c){buildReach();return c<0?0:enemyDistance[c];}
    SprintTrace trace(Action const &action,bool attack=false) const {
        if((action.type!=ActionType::MOVE&&action.type!=ActionType::SPRINT)||action.steps<1||action.steps>8)return {};
        auto state=initial();auto original=state.segments;
        for(int i=0;i<action.steps;++i) {
            state=advance(state,action.dirs[i],i,attack,original);
            if(!state.legal||(state.collision&&i+1<action.steps))return {};
        }
        return state;
    }
    // Evaluate one enemy action against the COMPLETE resulting body. A known
    // attack outranks uncertainty; a CPU/visibility bound never proves safety.
    int terminalRisk(SprintTrace const &state) {
        if(!state.legal || state.collision)return 2;
        buildReach();int risk=0;
        auto ourOriginal=ownSegments();
        for(size_t i=0;i<turn.enemyHeads.size();++i) {
            int start=turn.enemyHeads[i],id=occupancy.dragonId(start);
            int exact=i<turn.enemyHeadExactLen.size()?turn.enemyHeadExactLen[i]:-1;
            int observed=i<turn.enemyHeadLen.size()?turn.enemyHeadLen[i]:2;
            int length=exact>=2?exact:std::max(2,observed);
            std::vector<int> chain{start};
            while(int(chain.size())<length) {
                int prev=-1;
                for(auto const &part:turn.parts)
                    if(!part.mine && !part.head && part.id==id &&
                       topology.step(part.c,part.facing)==chain.back() &&
                       std::find(chain.begin(),chain.end(),part.c)==chain.end()){prev=part.c;break;}
                if(prev<0)break;chain.push_back(prev);
            }
            bool complete=exact>=2 && int(chain.size())==length;
            // Partial length/body hypotheses cannot certify safety. Retain
            // the broad reach bound for them, but do not reuse pre-action
            // reach to reject a fully proved body shield or consumed pearl.
            if(!complete) {
                bool possible=enemyDistance[state.head]==0;
                // A vacated tail may open a route that the pre-action reach
                // stopped at. Unknown extra fuel cannot certify that route safe.
                for(int c:ourOriginal)if(c>=0 && enemyDistance[c]==0 &&
                    std::find(state.segments.begin(),state.segments.end(),c)==state.segments.end())possible=true;
                if(possible)risk=1;
            }
            chain.resize(length,-1);auto original=chain;
            std::vector<int> eaten,entered;int states=0;bool confirmed=false,uncertain=false;
            auto visit=[&](auto &&self,int head,int steps,std::vector<int> const &segments)->void {
                bool cost=MovementRules::paidStep(length,steps);
                if(confirmed || (cost && segments.size()<=Constants::MIN_SIZE))return;
                if(steps>=8 || ++states>128){uncertain=true;return;}
                for(int d=0;d<4 && !confirmed;++d) {
                    if(topology.edgeTypeAt(head*4+d)==E_UNKNOWN) {
                        uncertain=true;continue;
                    }
                    int q=topology.step(head,d);
                    if(q<0){if(topology.edgeTypeAt(head*4+d)==E_PORTAL)uncertain=true;continue;}
                    if(std::find(segments.begin(),segments.end(),q)!=segments.end() ||
                       (!complete && std::find(entered.begin(),entered.end(),q)!=entered.end()))continue;
                    // With unknown length, an earlier visited cell may remain
                    // in a longer body's tail. Only simple paths prove an attack.
                    if(!occupancy.visible(q)){uncertain=true;continue;}
                    // Intermediate own neck/body is a lethal obstacle, not a
                    // possible stop from which the enemy can continue attacking.
                    if(q==state.head){confirmed=true;break;}
                    if(std::find(state.segments.begin(),state.segments.end(),q)!=state.segments.end())continue;
                    if(occupancy.occupied(q)) {
                        bool releasedUs=occupancy.isMine(q) && occupancy.dragonId(q)==turn.myId &&
                            std::find(ourOriginal.begin(),ourOriginal.end(),q)!=ourOriginal.end();
                        bool releasedEnemy=complete && !occupancy.isMine(q) && occupancy.dragonId(q)==id &&
                            std::find(original.begin(),original.end(),q)!=original.end();
                        if(!releasedUs && !releasedEnemy) {
                            int blocker=occupancy.dragonId(q);
                            // A different dragon scheduled before this enemy
                            // may vacate the cell. That is uncertainty, never a
                            // proved attack or a dependable permanent shield.
                            bool earlier=(blocker>turn.myId)!=(id>turn.myId)
                                ? blocker>turn.myId : blocker<id;
                            if(blocker!=id && (earlier || blocker==turn.myId))uncertain=true;
                            continue;
                        }
                    }
                    // Our pearls have already been consumed. Enemy pearls pay
                    // only AFTER collision/affordability, with fixed free steps.
                    bool pearl=occupancy.hasPearl(q) &&
                        std::find(state.eaten.begin(),state.eaten.end(),q)==state.eaten.end() &&
                        std::find(eaten.begin(),eaten.end(),q)==eaten.end();
                    auto after=segments;if(!pearl)after.pop_back();after.insert(after.begin(),q);
                    if(cost)after.pop_back();if(pearl)eaten.push_back(q);
                    entered.push_back(q);self(self,q,steps+1,after);entered.pop_back();
                    if(pearl)eaten.pop_back();
                }
            };
            visit(visit,start,0,chain);
            if(confirmed)return 2;
            if(uncertain)risk=1;
#if FEAT_V3_TACTICAL_SPLIT
            // Preserve the existing early tail-child attack hypothesis. Enemy
            // population capacity is hidden, so a possible split is uncertain.
            if(turn.round<CH_START && id>=0 && id<turn.myId && observed>=8 &&
               turn.countOf(turn.myId)>=5 && topology.manh(turn.here,start)>2) {
                bool nearFriend=false;
                for(int head:turn.friendHeads)if(topology.manh(start,head)<=2)nearFriend=true;
                if(!nearFriend)for(auto const &part:turn.parts) {
                    if(part.mine || part.id!=id || part.head || occupancy.freeAt(part.c)!=2)continue;
                    int visibleBody=0;
                    for(int c:state.segments)if(c>=0 && topology.cheb(part.c,c)<=3)++visibleBody;
                    for(int d=0;visibleBody>=3 && d<4;++d)
                        if(topology.edgeTypeAt(part.c*4+d)==E_OPEN && topology.step(part.c,d)==state.head)risk=1;
                }
            }
#endif
        }
        return risk;
    }
    // Hard path constraints prune expansion. Terminal policy rejection only
    // rejects this candidate: an unsafe prefix may still reach a safe endpoint.
    template<class HardAllowed,class Allowed> SprintEscape escape(HardAllowed hardAllowed,Allowed allowed) {
        if(turn.enemyHeads.empty())return {};
        buildReach();auto start=initial();auto original=start.segments;
        std::array<uint8_t,8> path{};SprintEscape best;
        int bestSteps=99,bestCost=99,bestPearls=-1,states=0;
        int maxSteps=std::min(8,MovementRules::freeSteps(turn.length)+2);
        auto visit=[&](auto &&self,SprintTrace const &state,int steps)->void {
            if(steps>=maxSteps||++states>1024)return;
            for(int d=0;d<4;++d) {
                if(coordination.portalHeld(state.head,d)||
                   (topology.edgeTypeAt(state.head*4+d)==E_PORTAL&&
                    coordination.portalBusy(topology.portalIdAt(state.head*4+d))))continue;
                auto next=advance(state,d,steps,false,original);if(!next.legal)continue;
                path[steps]=uint8_t(d);
                Action action{steps==0?ActionType::MOVE:ActionType::SPRINT,path,uint8_t(steps+1),0};
                if(!hardAllowed(action))continue;
                if(allowed(action)) {
                    int space=spaceAfter(next,original),distance=enemyDistance[next.head];
                    int cost=MovementRules::cost(turn.length,steps+1),pearls=int(next.eaten.size());
                    // Once a proved-safe free route exists, a paid route cannot
                    // outrank it. Avoid attack proofs for those paid candidates.
                    if(space>=2 && (!best.action || cost<=bestCost || best.risk>0)) {
                        int risk=terminalRisk(next);
                        bool better=!best.action || risk<best.risk || (risk==best.risk &&
                            (cost<bestCost || (cost==bestCost &&
                             (steps+1<bestSteps || (steps+1==bestSteps &&
                              (pearls>bestPearls || (pearls==bestPearls &&
                               (distance>best.clearance || (distance==best.clearance && space>best.space)))))))));
                        if(better) {
                            best={action,distance,space,next.length(),risk};bestSteps=steps+1;
                            bestCost=cost;bestPearls=pearls;
                        }
                    }
                }
                self(self,next,steps+1);
            }
        };
        visit(visit,start,0);return best;
    }
    template<class Allowed> SprintEscape escape(Allowed allowed) {
        return escape([](Action const &){return true;},allowed);
    }
    std::vector<uint8_t> enemyReachAfterSuicide() const {
        std::vector<uint8_t> danger(config.cells,0);
        for(size_t i=0;i<turn.enemyHeads.size();++i) {
            int start=turn.enemyHeads[i];
            int exact=i<turn.enemyHeadExactLen.size()?turn.enemyHeadExactLen[i]:-1;
            int fuel=exact+turn.length+int(turn.visPearls.size());
            // Assume all donor segments drop edible pearls, all are eaten,
            // and the enemy gets two actions before the Queen escapes again.
            int bound=exact>=2?std::min(config.cells,2*(MovementRules::freeSteps(fuel)+fuel-2)):config.cells;
            std::vector<int> distance(config.cells,-1),queue{start};distance[start]=0;
            for(size_t n=0;n<queue.size();++n) {
                int c=queue[n];danger[c]=1;
                if(!occupancy.visible(c))return std::vector<uint8_t>(config.cells,1); // unseen pearls prevent certification
                if(distance[c]>=bound)continue;
                for(int d=0;d<4;++d) {
                    if(topology.edgeTypeAt(c*4+d)==E_UNKNOWN || topology.unexploredPortal(c,d))
                        return std::vector<uint8_t>(config.cells,1); // unknown landing cannot certify rescue
                    int next=topology.step(c,d);
                    if(next<0||distance[next]>=0)continue;
                    // Ignore bodies, including the removed donor: overestimate enemy reach.
                    distance[next]=distance[c]+1;queue.push_back(next);
                }
            }
        }
        return danger;
    }
    template<class Worth,class Allowed> Action attack(Worth worth,Allowed allowed) const {
        auto start=initial();auto original=start.segments;
        std::array<uint8_t,8> path{};Action best;int enemySize=-1,bestSteps=99;
        auto visit=[&](auto &&self,SprintTrace const &state,int steps)->void {
            if(steps>=4)return;
            for(int d=0;d<4;++d) {
                if(coordination.portalHeld(state.head,d)||
                   (topology.edgeTypeAt(state.head*4+d)==E_PORTAL&&
                    coordination.portalBusy(topology.portalIdAt(state.head*4+d))))continue;
                auto next=advance(state,d,steps,true,original);if(!next.legal)continue;
                path[steps]=uint8_t(d);Action action{ActionType::SPRINT,path,uint8_t(steps+1),0};
                if(!allowed(action))continue;
                if(next.collision) {
                    int size=turn.countOf(occupancy.dragonId(next.head));
                    if(steps>0&&worth(next.head,next.length(),size)&&
                       (size>enemySize||(size==enemySize&&steps+1<bestSteps))) {
                        best=action;enemySize=size;bestSteps=steps+1;
                    }
                } else self(self,next,steps+1);
            }
        };
        visit(visit,start,0);return best;
    }
};
} // namespace bot
