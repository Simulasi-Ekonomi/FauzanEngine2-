#include "Runtime/AnimationTimeline.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine; AnimationTimeline timeline; float value=0.0F;
    if(!timeline.AddTrack("crop-growth",{{0.0F,0.0F},{1.0F,10.0F},{3.0F,20.0F}})||!timeline.Sample("crop-growth",0.5F,AnimationPlayback::Clamp,value)||std::fabs(value-5.0F)>0.0001F||!timeline.Sample("crop-growth",4.0F,AnimationPlayback::Clamp,value)||std::fabs(value-20.0F)>0.0001F||!timeline.Sample("crop-growth",3.5F,AnimationPlayback::Loop,value)||std::fabs(value-5.0F)>0.0001F) return 1;
    if(timeline.AddTrack("crop-growth",{{0,0},{1,1}})||timeline.LastError()!=AnimationError::DuplicateTrack||timeline.AddTrack("bad",{{0,0},{0,1}})||timeline.LastError()!=AnimationError::InvalidKeys||timeline.Sample("missing",0,AnimationPlayback::Clamp,value)||timeline.LastError()!=AnimationError::MissingTrack) return 1;
    std::printf("ANIMATION_TIMELINE_SMOKE_OK tracks=1 interpolate=1 clamp=1 loop=1 validation=1\n"); return 0;
}
