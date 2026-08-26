#pragma once

#include <cstdint>

namespace NeoEngine {

enum class FlipbookPlaybackError : uint8_t { None, NotInitialized, InvalidConfiguration, InvalidDelta };
struct FlipbookPlaybackConfig { float durationSeconds = 0.0F; bool loop = true; };
class FlipbookPlayback {
public:
    bool Initialize(FlipbookPlaybackConfig config);
    bool Advance(float deltaSeconds, float& normalizedSample);
    void SetPaused(bool paused) { paused_ = paused; }
    [[nodiscard]] float TimeSeconds() const { return timeSeconds_; }
    [[nodiscard]] bool IsPaused() const { return paused_; }
    [[nodiscard]] FlipbookPlaybackError LastError() const { return lastError_; }
private:
    FlipbookPlaybackConfig config_{};
    float timeSeconds_ = 0.0F;
    bool paused_ = false;
    bool initialized_ = false;
    mutable FlipbookPlaybackError lastError_ = FlipbookPlaybackError::NotInitialized;
};

} // namespace NeoEngine
