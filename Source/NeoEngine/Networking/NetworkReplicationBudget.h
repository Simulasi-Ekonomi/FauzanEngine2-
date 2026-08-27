#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

struct ReplicationBudgetStats { uint32_t budgetBytes{}; uint32_t usedBytes{}; uint32_t accepted{}; uint32_t rejected{}; };

class ReplicationBudget {
public:
    explicit ReplicationBudget(uint32_t bytesPerTick=12000):limit_(bytesPerTick){}
    void reset(){used_=0;accepted_=rejected_=0;}
    bool reserve(uint32_t bytes){if(bytes>limit_-used_){++rejected_;return false;}used_+=bytes;++accepted_;return true;}
    [[nodiscard]] uint32_t remaining()const{return limit_-used_;}
    [[nodiscard]] ReplicationBudgetStats stats()const{return {limit_,used_,accepted_,rejected_};}
private:
    uint32_t limit_{12000},used_{},accepted_{},rejected_{};
};
}
