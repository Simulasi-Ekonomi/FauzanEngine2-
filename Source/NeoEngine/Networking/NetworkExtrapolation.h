#pragma once
#include <cstdint>
#include <cmath>

namespace NeoEngine::Networking {

struct ExtrapolationSample { uint64_t tick{}; float x{},y{},z{}; };
struct ExtrapolatedTransform { float x{},y{},z{}; bool valid{}; };

class SnapshotExtrapolator {
public:
    static ExtrapolatedTransform predict(const ExtrapolationSample& previous,const ExtrapolationSample& latest,uint64_t targetTick,uint64_t maxTicks=6){
        if(!previous.tick||!latest.tick||latest.tick<=previous.tick||targetTick<latest.tick)return {latest.x,latest.y,latest.z,false};
        const uint64_t dt=latest.tick-previous.tick;const uint64_t ahead=targetTick-latest.tick;if(ahead>maxTicks)return {latest.x,latest.y,latest.z,false};
        const float vx=(latest.x-previous.x)/float(dt),vy=(latest.y-previous.y)/float(dt),vz=(latest.z-previous.z)/float(dt);
        return {latest.x+vx*float(ahead),latest.y+vy*float(ahead),latest.z+vz*float(ahead),std::isfinite(vx)&&std::isfinite(vy)&&std::isfinite(vz)};
    }
};
}
