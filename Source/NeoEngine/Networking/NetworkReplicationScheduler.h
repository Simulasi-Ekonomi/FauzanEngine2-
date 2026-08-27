#pragma once
#include <array>
#include <cstdint>
#include "NetworkReplicationBudget.h"

namespace NeoEngine::Networking {

struct ReplicationCandidate { uint32_t networkId{}; uint32_t estimatedBytes{}; uint16_t priority{}; uint64_t lastSentTick{}; };

class ReplicationScheduler {
public:
    static constexpr uint16_t Capacity=1024;
    static constexpr uint16_t MaxPerTick=256;

    uint16_t schedule(const ReplicationCandidate* candidates,uint16_t count,ReplicationBudget& budget,uint32_t* output,uint16_t capacity,uint64_t tick){
        if(!candidates||!output||!capacity)return 0;
        if(count>Capacity)count=Capacity;
        for(uint16_t i=0;i<count;++i) order_[i]=i;
        for(uint16_t i=1;i<count;++i){uint16_t key=order_[i],j=i;while(j>0&&higher(candidates[key],candidates[order_[j-1]],tick)){order_[j]=order_[j-1];--j;}order_[j]=key;}
        uint16_t n=0,limit=capacity<MaxPerTick?capacity:MaxPerTick;
        for(uint16_t i=0;i<count&&n<limit;++i){const auto&c=candidates[order_[i]];if(!c.networkId||!c.estimatedBytes)continue;if(budget.reserve(c.estimatedBytes))output[n++]=c.networkId;}
        return n;
    }
private:
    static bool higher(const ReplicationCandidate&a,const ReplicationCandidate&b,uint64_t tick){
        if(a.priority!=b.priority)return a.priority>b.priority;
        const uint64_t ageA=tick>=a.lastSentTick?tick-a.lastSentTick:0,ageB=tick>=b.lastSentTick?tick-b.lastSentTick:0;
        if(ageA!=ageB)return ageA>ageB;
        return a.networkId<b.networkId;
    }
    std::array<uint16_t,Capacity> order_{};
};
}
