#pragma once
#include "navigation.hpp"
#include <array>
#include <cmath>

namespace champion {
// A tiny contextual value model. The weights are intentionally static and
// deterministic so inference is bounded and reproducible in the tournament.
// They can later be replaced by weights exported from an offline RL run.
class LinearPolicy {
    std::array<double, 8> w{2.8, 1.9, 0.65, -0.12, -0.22, -0.08, 0.30, -0.55};
public:
    double value(double pearl, double spawn, double frontier, double distance,
                 double danger, double revisit, double portal, double crowd) const {
        return w[0]*pearl + w[1]*spawn + w[2]*frontier + w[3]*distance
             + w[4]*danger + w[5]*revisit + w[6]*portal + w[7]*crowd;
    }
};

class Strategy {
    WorldModel& w;
    LinearPolicy policy;
public:
    Target currentTarget;
    Role role=Role::Flagship;
    explicit Strategy(WorldModel& world):w(world){}
    Target selectTarget(const Navigator& nav) const {
        auto dist=w.distances(w.here);
        Target best;double bestScore=-1e20;
        for(int p=0;p<int(w.cells.size());++p) {
            int d=dist[p];if(d<=0)continue;
            const auto& c=w.cells[p];
            double food=w.pearlConfidence(p,d);
            double spawn=(!c.pearl && c.due>=w.round && c.due<=w.round+12) ? 1.0/(1.0+c.due-w.round) : 0.0;
            double frontier=0;
            for(int k=0;k<4;++k) if(w.cells[w.adjacent(p,k)].seen<0) frontier+=0.25;
            double danger=std::min(20.0,nav.dangerAt(w.position(p)))/20.0;
            double revisit=std::min(20.0,double(std::max(0,w.round-c.visited)))/20.0;
            double portal=0;
            for(int k=0;k<4;++k) if(c.edges[k]>=0) portal=1.0;
            double crowd=(c.seen==w.round && c.occupant>=0) ? 1.0 : 0.0;
            double score=policy.value(food,spawn,frontier,double(d)/double(std::max(w.width,w.height)),danger,
                                      revisit,portal,crowd);
            TargetKind kind=food>0 ? TargetKind::Pearl : (spawn>0 ? TargetKind::Spawn : TargetKind::Explore);
            if(currentTarget.valid() && currentTarget.position==w.position(p)) score+=0.15;
            // Different dragons deterministically break equal-value resource ties.
            score+=((p*17+w.id*31)%23)*0.0005;
            if(score>bestScore) {bestScore=score;best={w.position(p),kind,score};}
        }
        return best;
    }
    bool shouldSplit(const unswbc::Controller& ct,const Navigator& nav) const {
        if(!ct.can_split(2) || w.round>=Config::splitStopRound || w.round-w.previousSplit<60) return false;
        if(w.units>=Config::maxPopulation || w.length<Config::reserveSplitLength) return false;
        if(!w.enemies.empty() || nav.availableSpace()<45 || w.body.size()<4) return false;
        int tail=w.body.back();if(tail<0)return false;
        int free=0;
        for(int d=0;d<4;++d) {
            auto s=w.step(tail,d);
            if(s.cell>=0 && w.isKnownSafe(w.position(s.cell))) ++free;
        }
        // Require visible proof the reversed child can leave its old tail.
        if(free<2)return false;
        int nearbyPearls=0;
        for(const auto& c:w.cells) if(c.seen==w.round && c.pearl) ++nearbyPearls;
        return nearbyPearls>=5 && (w.units==1 || (w.length>=30 && role==Role::Forager));
    }
    Action decide(const unswbc::Controller& ct) {
        Navigator nav(w);
        // Original IDs are map dependent, so do not infer team roles from parity.
        role=Role::Flagship;
        for(const auto& c:w.cells) if(c.seen==w.round && c.head && c.team==w.team && c.occupant<w.id) role=Role::Forager;
        if(shouldSplit(ct,nav)) return Action::split(2);
        if(role==Role::Forager && w.units>1 && w.length<=8) {
            for(const auto& enemy:w.enemies) if(enemy.visibleLength>=2*w.length+2) {
                auto attack=nav.intercept(enemy);if(attack.score>=1000)return attack;
            }
        }
        currentTarget=selectTarget(nav);
        return nav.moveSafely(currentTarget);
    }
};
}
