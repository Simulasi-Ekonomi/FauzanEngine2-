#pragma once
#include <array>
#include <cstdint>

namespace NeoEngine::Networking {

struct InterpolationSample { uint64_t tick{}; float x{},y{},z{}; };
struct InterpolatedTransform { float x{},y{},z{}; };

class SnapshotInterpolator {
public:
    static constexpr uint16_t Capacity=32;
    bool push(const InterpolationSample&s){if(!s.tick)return false;samples_[(head_+count_)%Capacity]=s;if(count_<Capacity)++count_;else head_=(head_+1)%Capacity;return true;}
    bool sample(double renderTick, InterpolatedTransform& out) const {
        if (!count_) return false;
        if (count_ == 1U) {
            const auto& s = samples_[head_];
            out = {s.x, s.y, s.z};
            return true;
        }
        const InterpolationSample* a = nullptr;
        const InterpolationSample* b = nullptr;
        for (uint16_t i = 0U; i < count_; ++i) {
            const auto& s = samples_[(head_ + i) % Capacity];
            if (static_cast<double>(s.tick) <= renderTick) a = &s;
            if (static_cast<double>(s.tick) >= renderTick) {
                b = &s;
                break;
            }
        }
        if (!a) a = &samples_[head_];
        if (!b) b = &samples_[(head_ + count_ - 1U) % Capacity];
        if (a->tick == b->tick) {
            out = {a->x, a->y, a->z};
            return true;
        }
        double t = (renderTick - static_cast<double>(a->tick)) / static_cast<double>(b->tick - a->tick);
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        out = {static_cast<float>(a->x + (b->x - a->x) * t), static_cast<float>(a->y + (b->y - a->y) * t), static_cast<float>(a->z + (b->z - a->z) * t)};
        return true;
    }
private: std::array<InterpolationSample,Capacity> samples_{};uint16_t head_{},count_{};
};
}
