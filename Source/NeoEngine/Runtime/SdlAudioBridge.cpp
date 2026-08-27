#include "SdlAudioBridge.h"

#include <SDL.h>

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
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        lastError_ = SdlAudioBridgeError::AudioInitializationFailed;
        return false;
    }
    audioInitialized_ = true;
    SDL_AudioSpec desired{};
    desired.freq = 48000;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = framesPerCallback;
    desired.callback = &SdlAudioBridge::AudioCallback;
    desired.userdata = this;
    SDL_AudioSpec obtained{};
    const SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device == 0 || obtained.freq != desired.freq || obtained.format != desired.format || obtained.channels != desired.channels) {
        if (device != 0) SDL_CloseAudioDevice(device);
        lastError_ = SdlAudioBridgeError::DeviceOpenFailed;
        Reset();
        return false;
    }
    deviceId_ = device;
    framesMixed_.store(0);
    SDL_PauseAudioDevice(deviceId_, 0);
    lastError_ = SdlAudioBridgeError::None;
    return true;
}

bool SdlAudioBridge::Play(uint32_t id, std::vector<int16_t> mono, uint16_t gainQ8) {
    if (deviceId_ == 0) {
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
    if (deviceId_ != 0) {
        SDL_PauseAudioDevice(deviceId_, 1);
        SDL_LockAudioDevice(deviceId_);
        {
            std::lock_guard<std::mutex> lock(mixerMutex_);
            mixer_.Clear();
        }
        SDL_UnlockAudioDevice(deviceId_);
        SDL_CloseAudioDevice(deviceId_);
    } else {
        std::lock_guard<std::mutex> lock(mixerMutex_);
        mixer_.Clear();
    }
    deviceId_ = 0;
    if (audioInitialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioInitialized_ = false;
    framesMixed_.store(0);
}

void SdlAudioBridge::AudioCallback(void* userdata, uint8_t* stream, int length) {
    auto* bridge = static_cast<SdlAudioBridge*>(userdata);
    if (bridge == nullptr || stream == nullptr || length <= 0) return;
    const size_t frames = static_cast<size_t>(length) / (sizeof(int16_t) * 2U);
    std::vector<int16_t> mixed;
    {
        std::lock_guard<std::mutex> lock(bridge->mixerMutex_);
        bridge->mixer_.Mix(frames, mixed);
    }
    const size_t byteCount = std::min(static_cast<size_t>(length), mixed.size() * sizeof(int16_t));
    if (byteCount > 0) std::memcpy(stream, mixed.data(), byteCount);
    if (byteCount < static_cast<size_t>(length)) std::memset(stream + byteCount, 0, static_cast<size_t>(length) - byteCount);
    bridge->framesMixed_.fetch_add(frames);
}

} // namespace NeoEngine
