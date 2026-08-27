#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

struct TimeSyncSample { uint64_t clientTick{}; uint64_t serverTick{}; double rttSeconds{}; };

class NetworkClock {
public:
    explicit NetworkClock(double tickRate=60.0):tickRate_(tickRate>0.0?tickRate:60.0){}
    void observe(const TimeSyncSample& s){
        if(!s.clientTick||!s.serverTick||s.rttSeconds<0.0)return;
        const double oneWayTicks=s.rttSeconds*tickRate_*0.5;
        const double estimate=double(s.serverTick)+oneWayTicks-double(s.clientTick);
        offsetTicks_=offsetTicks_*(1.0-alpha_)+estimate*alpha_;
        rtt_=rtt_*(1.0-alpha_)+s.rttSeconds*alpha_;
    }
    uint64_t serverTickEstimate(uint64_t clientTick)const{
        if(offsetTicks_<=-double(clientTick))return 0;
        return static_cast<uint64_t>(double(clientTick)+offsetTicks_);
    }
    double offsetTicks()const{return offsetTicks_;}
    double rttSeconds()const{return rtt_;}
private:
    double tickRate_; double offsetTicks_{}; double rtt_{}; static constexpr double alpha_=0.125;
};
}
