#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

struct ReconnectDecision {
    bool allowed{};
    uint32_t delayMs{};
    uint32_t attempt{};
};

class ReconnectPolicy {
public:
    constexpr ReconnectPolicy(uint32_t initialDelayMs=250,uint32_t maxDelayMs=8000,uint32_t maxAttempts=8)
        : initial_(initialDelayMs), max_(maxDelayMs), attempts_(maxAttempts) {}

    [[nodiscard]] ReconnectDecision next(uint32_t attempt) const {
        if (attempt == 0 || attempt > attempts_) return {};
        uint64_t delay=initial_;
        for (uint32_t i=1;i<attempt;++i) delay*=2;
        if (delay>max_) delay=max_;
        return {true,static_cast<uint32_t>(delay),attempt};
    }
    [[nodiscard]] uint32_t maxAttempts() const { return attempts_; }
private:
    uint32_t initial_,max_,attempts_;
};

}
