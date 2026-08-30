#pragma once

#include "AudioMixer.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace NeoEngine {

enum class SdlAudioBridgeError : uint8_t { None, InvalidConfiguration, AudioInitializationFailed, DeviceOpenFailed, NotInitialized, MixerRejected };

class SdlAudioBridge {
public:
    SdlAudioBridge() = default;
    ~SdlAudioBridge();

    SdlAudioBridge(const SdlAudioBridge&) = delete;
    SdlAudioBridge& operator=(const SdlAudioBridge&) = delete;

    bool Initialize(uint16_t framesPerCallback = 256);
    bool Play(uint32_t id, std::vector<int16_t> mono, uint16_t gainQ8 = 256);
    void Reset();

    [[nodiscard]] bool IsReady() const { return stream_ != nullptr; }
    [[nodiscard]] uint64_t FramesMixed() const { return framesMixed_.load(); }
    [[nodiscard]] uint16_t QueuedVoiceCount() const;
    [[nodiscard]] SdlAudioBridgeError LastError() const { return lastError_; }

private:
    static void AudioCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount);

    SDL_AudioStream* stream_ = nullptr;
    bool audioInitialized_ = false;
    AudioMixer mixer_;
    mutable std::mutex mixerMutex_;
    std::atomic<uint64_t> framesMixed_{0};
    SdlAudioBridgeError lastError_ = SdlAudioBridgeError::None;
};

} // namespace NeoEngine
