#include "SdlAudioBridge.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {

SdlAudioBridge::~SdlAudioBridge() {
    Reset();
}

bool SdlAudioBridge::Initialize(uint16_t framesPerCallback) {
    Reset();
    if (framesPerCallback == 0) {
        lastError_ = SdlAudioBridgeError::InvalidConfiguration;
        return false;
    }

    const size_t requestedFrames = static_cast<size_t>(framesPerCallback) * kCallbackCapacityMultiplier;
    callbackBufferFrames_ = std::min(requestedFrames, kMaxCallbackFrames);
    if (callbackBufferFrames_ == 0 || callbackBufferFrames_ > std::numeric_limits<size_t>::max() / kStereoChannels) {
        lastError_ = SdlAudioBridgeError::InvalidConfiguration;
        return false;
    }
    callbackBuffer_.assign(callbackBufferFrames_ * kStereoChannels, 0);

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        callbackBuffer_.clear();
        callbackBufferFrames_ = 0;
        lastError_ = SdlAudioBridgeError::AudioInitializationFailed;
        return false;
    }
    audioInitialized_ = true;

    const SDL_AudioSpec desired{SDL_AUDIO_S16, 2, 48000};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, &SdlAudioBridge::AudioCallback, this);
    if (stream_ == nullptr) {
        lastError_ = SdlAudioBridgeError::DeviceOpenFailed;
        Reset();
        return false;
    }

    framesMixed_.store(0);
    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        lastError_ = SdlAudioBridgeError::DeviceOpenFailed;
        Reset();
        return false;
    }
    lastError_ = SdlAudioBridgeError::None;
    return true;
}

bool SdlAudioBridge::Play(uint32_t id, std::vector<int16_t> mono, uint16_t gainQ8) {
    if (stream_ == nullptr) {
        lastError_ = SdlAudioBridgeError::NotInitialized;
        return false;
    }
    SDL_LockAudioStream(stream_);
    const bool accepted = mixer_.Play(id, std::move(mono), gainQ8);
    SDL_UnlockAudioStream(stream_);
    if (!accepted) {
        lastError_ = SdlAudioBridgeError::MixerRejected;
        return false;
    }
    lastError_ = SdlAudioBridgeError::None;
    return true;
}

uint16_t SdlAudioBridge::QueuedVoiceCount() const {
    if (stream_ == nullptr) return 0;
    SDL_LockAudioStream(stream_);
    const auto count = static_cast<uint16_t>(mixer_.ActiveVoices());
    SDL_UnlockAudioStream(stream_);
    return count;
}

void SdlAudioBridge::Reset() {
    if (stream_ != nullptr) {
        SDL_LockAudioStream(stream_);
        mixer_.Clear();
        SDL_UnlockAudioStream(stream_);
        SDL_DestroyAudioStream(stream_);
    } else {
        mixer_.Clear();
    }
    stream_ = nullptr;
    if (audioInitialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioInitialized_ = false;
    callbackBuffer_.clear();
    callbackBufferFrames_ = 0;
    framesMixed_.store(0);
}

void SdlAudioBridge::AudioCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int /*totalAmount*/) {
    auto* bridge = static_cast<SdlAudioBridge*>(userdata);
    if (bridge == nullptr || stream == nullptr || additionalAmount <= 0) return;

    constexpr size_t bytesPerFrame = sizeof(int16_t) * kStereoChannels;
    const size_t requestedBytes = static_cast<size_t>(additionalAmount);
    const size_t frames = requestedBytes / bytesPerFrame;
    if (frames == 0) return;

    // SDL may vary additionalAmount from callback to callback. Mix in bounded
    // chunks so the realtime callback never allocates or depends on one fixed
    // device request size.
    size_t remainingFrames = frames;
    while (remainingFrames > 0) {
        const size_t chunkFrames = std::min(remainingFrames, bridge->callbackBufferFrames_);
        bridge->mixer_.Mix(chunkFrames, bridge->callbackBuffer_);
        const size_t chunkBytes = chunkFrames * bytesPerFrame;
        SDL_PutAudioStreamData(stream, bridge->callbackBuffer_.data(), static_cast<int>(chunkBytes));
        bridge->framesMixed_.fetch_add(chunkFrames, std::memory_order_relaxed);
        remainingFrames -= chunkFrames;
    }

    const size_t trailingBytes = requestedBytes - (frames * bytesPerFrame);
    if (trailingBytes > 0) {
        static const int16_t silence[] = {0, 0};
        size_t remaining = trailingBytes;
        while (remaining > 0) {
            const int chunk = static_cast<int>(std::min(remaining, sizeof(silence)));
            SDL_PutAudioStreamData(stream, silence, chunk);
            remaining -= static_cast<size_t>(chunk);
        }
    }
}

} // namespace NeoEngine
