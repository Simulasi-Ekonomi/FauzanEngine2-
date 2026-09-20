#pragma once

#include "AudioMixer.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdint>
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
    bool PlaySpatial(const SpatialVoiceParams& params);
    bool Stop(uint32_t id);
    bool UpdateVoicePosition(uint32_t id, const float position[3]);
    bool UpdateVoicePitch(uint32_t id, float pitch);
    bool UpdateVoiceGain(uint32_t id, uint16_t gainQ8);
    bool SetListener(const AudioListener& listener);
    void Reset();

    [[nodiscard]] bool IsReady() const { return stream_ != nullptr; }
    [[nodiscard]] uint64_t FramesMixed() const { return framesMixed_.load(); }
    [[nodiscard]] uint16_t QueuedVoiceCount() const;
    [[nodiscard]] SdlAudioBridgeError LastError() const { return lastError_; }

private:
    static constexpr size_t kCallbackCapacityMultiplier = 4;
    static constexpr size_t kMaxCallbackFrames = 4096;
    static constexpr size_t kStereoChannels = 2;

    static void AudioCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount);

    SDL_AudioStream* stream_ = nullptr;
    bool audioInitialized_ = false;
    AudioMixer mixer_;
    std::vector<int16_t> callbackBuffer_;
    size_t callbackBufferFrames_ = 0;
    std::atomic<uint64_t> framesMixed_{0};
    SdlAudioBridgeError lastError_ = SdlAudioBridgeError::None;
};

} // namespace NeoEngine
