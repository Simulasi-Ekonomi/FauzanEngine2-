#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

struct HistoricalTransform { uint64_t serverTick{}; float x{},y{},z{}; uint64_t revision{}; };

class LagCompensationHistory {
public:
    static constexpr uint16_t Capacity=256;
    bool record(uint64_t tick,float x,float y,float z,uint64_t revision){if(!tick)return false;history_[head_]={tick,x,y,z,revision};head_=(head_+1)%Capacity;if(count_<Capacity)++count_;return true;}
    bool sample(uint64_t targetTick, HistoricalTransform& out) const {
        if (!count_) return false;
        const HistoricalTransform* best = nullptr;
        for (uint16_t i = 0U; i < count_; ++i) {
            const auto& h = history_[(head_ + Capacity - count_ + i) % Capacity];
            if (h.serverTick <= targetTick && (!best || h.serverTick > best->serverTick)) best = &h;
        }
        if (!best) return false;
        out = *best;
        return true;
    }
    [[nodiscard]] uint16_t size()const{return count_;}
private: std::array<HistoricalTransform,Capacity> history_{};uint16_t head_{},count_{};
};
}
