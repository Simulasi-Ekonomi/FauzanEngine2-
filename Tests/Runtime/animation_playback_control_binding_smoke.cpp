#include "Runtime/AnimationPlaybackControlBinding.h"
#include "Runtime/FlipbookPlayback.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    FlipbookPlayback playback; float sample=0.0F; if(!playback.Initialize({1.0F,true}) || !playback.Advance(0.25F,sample)) return 1;
    AnimationPlaybackControlBinding binding;
    if(binding.Configure({"idle","idle"}) || binding.LastError()!=AnimationPlaybackControlBindingError::InvalidConfiguration) return 1;
    if(!binding.Configure({"idle","move"}) || !binding.Apply("idle",playback) || !playback.IsPaused()) return 1;
    if(!playback.Advance(0.25F,sample) || std::fabs(sample-0.25F)>0.0001F || std::fabs(playback.TimeSeconds()-0.25F)>0.0001F) return 1;
    if(binding.Apply("unknown",playback) || binding.LastError()!=AnimationPlaybackControlBindingError::UnmappedState || !playback.IsPaused()) return 1;
    if(!binding.Apply("move",playback) || playback.IsPaused() || !playback.Advance(0.25F,sample) || std::fabs(sample-0.5F)>0.0001F) return 1;
    std::printf("ANIMATION_PLAYBACK_CONTROL_BINDING_SMOKE_OK paused=0 sample=0.50 atomic=1\n"); return 0;
}
