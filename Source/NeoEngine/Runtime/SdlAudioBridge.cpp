#include "SdlAudioBridge.h"

#include <cstring>

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
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
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
    std::lock_guard<std::mutex> lock(mixerMutex_);
    if (!mixer_.Play(id, std::move(mono), gainQ8)) {
        lastError_ = SdlAudioBridgeError::MixerRejected;
        return false;
    }
    lastError_ = SdlAudioBridgeError::None;
    return true;
}

uint16_t SdlAudioBridge::QueuedVoiceCount() const {
    std::lock_guard<std::mutex> lock(mixerMutex_);
    return static_cast<uint16_t>(mixer_.ActiveVoices());
}

void SdlAudioBridge::Reset() {
    if (stream_ != nullptr) {
        SDL_LockAudioStream(stream_);
        {
            std::lock_guard<std::mutex> lock(mixerMutex_);
            mixer_.Clear();
        }
        SDL_UnlockAudioStream(stream_);
        SDL_DestroyAudioStream(stream_);
    } else {
        std::lock_guard<std::mutex> lock(mixerMutex_);
        mixer_.Clear();
    }
    stream_ = nullptr;
    if (audioInitialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioInitialized_ = false;
    framesMixed_.store(0);
}

void SdlAudioBridge::AudioCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int /*totalAmount*/) {
    auto* bridge = static_cast<SdlAudioBridge*>(userdata);
    if (bridge == nullptr || stream == nullptr || additionalAmount <= 0) return;
    const size_t frames = static_cast<size_t>(additionalAmount) / (sizeof(int16_t) * 2U);
    if (frames == 0) return;
    std::vector<int16_t> mixed;
    {
        std::lock_guard<std::mutex> lock(bridge->mixerMutex_);
        bridge->mixer_.Mix(frames, mixed);
    }
    const size_t byteCount = std::min(static_cast<size_t>(additionalAmount), mixed.size() * sizeof(int16_t));
    if (byteCount > 0) SDL_PutAudioStreamData(stream, mixed.data(), static_cast<int>(byteCount));
    if (byteCount < static_cast<size_t>(additionalAmount)) {
        std::vector<uint8_t> silence(static_cast<size_t>(additionalAmount) - byteCount, 0);
        SDL_PutAudioStreamData(stream, silence.data(), static_cast<int>(silence.size()));
    }
    bridge->framesMixed_.fetch_add(frames);
}

} // namespace NeoEngine
