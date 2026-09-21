#pragma once
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>

namespace NeoEngine::RuntimeContractGuard {

// Ten reusable runtime invariants shared by every active production branch.
// They are deliberately side-effect free so callers can fail closed without
// mutating already-valid runtime state.
struct RuntimeContractGuard final {
    static constexpr bool ValidFixedTicks(uint32_t value) noexcept {
        return value > 0U && value <= 1000U;
    }
    static constexpr bool ValidInitialCoins(int64_t value) noexcept {
        return value >= 0 && value <= 1'000'000'000'000LL;
    }
    static constexpr bool ValidNpcCount(uint16_t value, uint16_t maxValue) noexcept {
        return value > 0U && value <= maxValue;
    }
    static constexpr bool ValidRenderExtent(uint16_t width, uint16_t height) noexcept {
        return width >= 16U && height >= 16U;
    }
    static bool ValidTimeScale(float value) noexcept {
        return std::isfinite(value) && value >= 0.0F && value <= 4.0F;
    }
    static bool ValidDelta(float value) noexcept {
        return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
    }
    static constexpr bool ValidFrameCount(uint64_t value) noexcept {
        return value != std::numeric_limits<uint64_t>::max();
    }
    static constexpr bool ValidFixedStepCount(uint64_t value) noexcept {
        return value != std::numeric_limits<uint64_t>::max();
    }
    static constexpr bool ValidPendingFixedSteps(uint32_t value) noexcept {
        return value <= 1000U;
    }
    static constexpr bool ValidPayloadSize(size_t size, size_t maximum) noexcept {
        return size <= maximum;
    }
};

} // namespace NeoEngine::RuntimeContractGuard
