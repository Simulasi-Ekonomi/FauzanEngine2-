#include "Runtime/SdlAudioBridge.h"

#include <SDL.h>
#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    SdlAudioBridge bridge;
    if (bridge.Initialize(0) || bridge.LastError() != SdlAudioBridgeError::InvalidConfiguration || bridge.QueuedVoiceCount() != 0U) return 1;
    bridge.Reset();
    if (bridge.LastError() != SdlAudioBridgeError::InvalidConfiguration || !bridge.Initialize(128) || !bridge.IsReady() || bridge.FramesMixed() != 0U || bridge.QueuedVoiceCount() != 0U) return 1;
    std::vector<int16_t> longTone(AudioMixer::kMaxSamplesPerVoice, 1200);
    if (!bridge.Play(1, longTone) || !bridge.Play(2, longTone, 128) || bridge.QueuedVoiceCount() != 2U) return 1;
    SDL_Delay(40);
    if (bridge.FramesMixed() == 0U || bridge.QueuedVoiceCount() != 2U) return 1;
    bridge.Reset();
    if (bridge.IsReady() || bridge.FramesMixed() != 0U || bridge.QueuedVoiceCount() != 0U || bridge.LastError() != SdlAudioBridgeError::None || bridge.Play(1, longTone) || bridge.LastError() != SdlAudioBridgeError::NotInitialized) return 1;
    bridge.Reset();
    if (bridge.LastError() != SdlAudioBridgeError::NotInitialized || !bridge.Initialize(128) || bridge.FramesMixed() != 0U || bridge.QueuedVoiceCount() != 0U || !bridge.Play(1, std::vector<int16_t>(480U, 800)) || bridge.QueuedVoiceCount() != 1U) return 1;
    SDL_Delay(40);
    if (bridge.FramesMixed() == 0U) return 1;
    const uint64_t recoveredFrames = bridge.FramesMixed();
    bridge.Reset();
    if (bridge.IsReady() || bridge.FramesMixed() != 0U || bridge.QueuedVoiceCount() != 0U) return 1;
    std::printf("SDL_AUDIO_BRIDGE_SMOKE_OK recovery=1 frames_before_reset=%llu\n", static_cast<unsigned long long>(recoveredFrames));
    return 0;
}
