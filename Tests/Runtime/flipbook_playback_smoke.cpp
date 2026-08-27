#include "Runtime/FlipbookFrameSelector.h"
#include "Runtime/FlipbookPlayback.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    FlipbookPlayback playback; float sample = 77.0F;
    if (playback.Initialize({0.0F,true}) || playback.LastError() != FlipbookPlaybackError::InvalidConfiguration) return 1;
    if (!playback.Initialize({1.0F,true}) || !playback.Advance(0.25F,sample) || std::fabs(sample-0.25F)>0.0001F || std::fabs(playback.TimeSeconds()-0.25F)>0.0001F) return 1;
    const float preservedSample = sample;
    if (playback.Initialize({0.0F,false}) || playback.LastError()!=FlipbookPlaybackError::InvalidConfiguration || !playback.Advance(0.0F,sample) || std::fabs(sample-preservedSample)>0.0001F || std::fabs(playback.TimeSeconds()-0.25F)>0.0001F) return 1;
    FlipbookFrameSelector selector; SpriteSourceRect rect{};
    if (!selector.Initialize({4U,1U,1U,1U,4U}) || !selector.Select(sample,rect) || rect.x != 1U) return 1;
    playback.SetPaused(true); if (!playback.Advance(0.25F,sample) || !playback.IsPaused() || std::fabs(sample-0.25F)>0.0001F || std::fabs(playback.TimeSeconds()-0.25F)>0.0001F || !selector.Select(sample,rect) || rect.x != 1U) return 1;
    playback.SetPaused(false);
    if (!playback.Advance(0.75F,sample) || sample != 0.0F || playback.TimeSeconds() != 0.0F || !selector.Select(sample,rect) || rect.x != 0U) return 1;
    const float stableTime = playback.TimeSeconds(); const float stableSample = sample;
    if (playback.Advance(std::nanf(""),sample) || playback.LastError() != FlipbookPlaybackError::InvalidDelta || playback.TimeSeconds() != stableTime || sample != stableSample) return 1;
    FlipbookPlayback clamp; if (!clamp.Initialize({1.0F,false}) || !clamp.Advance(2.0F,sample) || sample != 1.0F || clamp.TimeSeconds() != 1.0F) return 1;
    std::printf("FLIPBOOK_PLAYBACK_SMOKE_OK loop=0.00 paused=stable frame=%u atomic=1 clamp=1.00\n", rect.x);
    return 0;
}
