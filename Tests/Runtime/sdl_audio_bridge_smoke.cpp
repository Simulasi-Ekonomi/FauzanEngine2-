#include "Runtime/SdlAudioBridge.h"

#include <SDL.h>
#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    SdlAudioBridge bridge;
    if (bridge.Initialize(0) || bridge.LastError() != SdlAudioBridgeError::InvalidConfiguration || !bridge.Initialize(128)) return 1;
    std::vector<int16_t> tone(480, 1200);
    if (!bridge.Play(1, tone) || !bridge.Play(2, tone, 128)) return 1;
    SDL_Delay(40);
    if (bridge.FramesMixed() == 0) return 1;
    std::printf("SDL_AUDIO_BRIDGE_SMOKE_OK frames=%llu\n", static_cast<unsigned long long>(bridge.FramesMixed()));
    return 0;
}
