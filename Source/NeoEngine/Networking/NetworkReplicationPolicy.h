#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

struct ReplicationEntity {
    uint32_t networkId{};
    float x{}, y{}, z{};
    float priority{};
    uint32_t ownerId{};
    uint64_t revision{};
};

struct ReplicationCandidate {
    uint32_t networkId{};
    float priority{};
};

class ReplicationPolicy {
public:
    static constexpr uint16_t MaxEntities=1024;
    static constexpr uint16_t MaxCandidates=256;

    static bool buildInterest(const ReplicationEntity& viewer,
                              const ReplicationEntity* entities,
                              uint16_t entityCount,
                              float radius,
                              uint32_t* output,
                              uint16_t& outputCount) {
        if (!entities || !output || !entityCount || radius<=0.0F || radius!=radius) return false;
        outputCount=0;
        const float r2=radius*radius;
        for (uint16_t i=0;i<entityCount && i<MaxEntities;++i) {
            const auto&e=entities[i];
            const float dx=e.x-viewer.x,dy=e.y-viewer.y,dz=e.z-viewer.z;
            if (dx*dx+dy*dy+dz*dz<=r2 && outputCount<MaxCandidates) output[outputCount++]=e.networkId;
        }
        return true;
    }

    static uint16_t prioritize(const ReplicationEntity& viewer,
                               const ReplicationEntity* entities,
                               uint16_t entityCount,
                               ReplicationCandidate* output,
                               uint16_t capacity) {
        if (!entities || !output || !capacity) return 0;
        uint16_t n=0;
        for(uint16_t i=0;i<entityCount && i<MaxEntities;++i){
            const auto&e=entities[i];
            const float dx=e.x-viewer.x,dy=e.y-viewer.y,dz=e.z-viewer.z;
            const float d2=dx*dx+dy*dy+dz*dz;
            if(d2>0.0F || e.networkId==viewer.networkId){
                if(n<capacity) output[n++]={e.networkId,e.priority/(1.0F+d2)};
            }
        }
        for(uint16_t i=1;i<n;++i){auto key=output[i];int j=i-1;while(j>=0&&output[j].priority<key.priority){output[j+1]=output[j];--j;}output[j+1]=key;}
        return n;
    }
};

} // namespace NeoEngine::Networking
