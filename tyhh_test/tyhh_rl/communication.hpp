#pragma once
#include "world.hpp"

namespace champion {
// Optional advisory protocol matching the discussion: type:3, x:6, y:6,
// payload:9, round:8. It cannot authenticate a sender; never mark a tile safe
// or alter occupancy using an incoming message. Disabled by default.
struct SonarMessage { unsigned type=0,x=0,y=0,payload=0,round=0; };
struct SonarCodec {
    static std::uint32_t encode(SonarMessage m) {
        return ((m.type&7u)<<29)|((m.x&63u)<<23)|((m.y&63u)<<17)|((m.payload&511u)<<8)|(m.round&255u);
    }
    static std::optional<SonarMessage> decode(std::uint32_t n,const WorldModel& w) {
        SonarMessage m{n>>29,(n>>23)&63u,(n>>17)&63u,(n>>8)&511u,n&255u};
        unsigned age=(unsigned(w.round)-m.round)&255u;
        if(m.type>4 || m.x>=unsigned(w.width) || m.y>=unsigned(w.height) || age>2)return std::nullopt;
        return m;
    }
};
}
