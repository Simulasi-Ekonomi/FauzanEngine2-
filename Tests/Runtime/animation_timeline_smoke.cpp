#include "Runtime/AnimationTimeline.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine; AnimationTimeline timeline; float value=0.0F;
    if(!timeline.AddTrack("crop-growth",{{0.0F,0.0F},{1.0F,10.0F},{3.0F,20.0F}})||!timeline.Sample("crop-growth",0.5F,AnimationPlayback::Clamp,value)||std::fabs(value-5.0F)>0.0001F||!timeline.Sample("crop-growth",4.0F,AnimationPlayback::Clamp,value)||std::fabs(value-20.0F)>0.0001F||!timeline.Sample("crop-growth",3.5F,AnimationPlayback::Loop,value)||std::fabs(value-5.0F)>0.0001F) return 1;
    if(timeline.AddTrack("crop-growth",{{0,0},{1,1}})||timeline.LastError()!=AnimationError::DuplicateTrack||timeline.AddTrack("bad",{{0,0},{0,1}})||timeline.LastError()!=AnimationError::InvalidKeys||timeline.Sample("missing",0,AnimationPlayback::Clamp,value)||timeline.LastError()!=AnimationError::MissingTrack) return 1;
    if(timeline.AddTrack(std::string("nul\0track", 9U), {{0.0F, 0.0F}, {1.0F, 1.0F}})||timeline.LastError()!=AnimationError::InvalidTrack) return 1;
    if(!timeline.AddEventMarker("crop-growth", {"start", 0.0F})||!timeline.AddEventMarker("crop-growth", {"sprout", 0.5F})||!timeline.AddEventMarker("crop-growth", {"harvest", 3.0F})||timeline.AddEventMarker("crop-growth", {"sprout", 1.0F})||timeline.LastError()!=AnimationError::DuplicateEvent) return 2;
    if(timeline.AddEventMarker("crop-growth", {std::string("nul\0event", 9U), 0.25F})||timeline.LastError()!=AnimationError::InvalidEvent) return 2;
    std::vector<std::string> events;
    if(!timeline.CollectEvents("crop-growth", 0.0F, 0.5F, AnimationPlayback::Clamp, events)||events.size()!=1U||events[0]!="sprout") return 3;
    if(!timeline.CollectEvents("crop-growth", 2.5F, 3.5F, AnimationPlayback::Loop, events)||events.size()!=3U||events[0]!="harvest"||events[1]!="start"||events[2]!="sprout") return 4;
    events = {"preserve"};
    if(timeline.CollectEvents("crop-growth", 1.0F, 0.0F, AnimationPlayback::Clamp, events)||timeline.LastError()!=AnimationError::InvalidEvent||events.size()!=1U||events[0]!="preserve") return 5;
    events = {"invalid-preserve"};
    value = 77.0F;
    const AnimationPlayback invalidPlayback = static_cast<AnimationPlayback>(255U);
    if(timeline.CollectEvents("crop-growth", 0.0F, 0.5F, invalidPlayback, events)||timeline.LastError()!=AnimationError::InvalidPlayback||events.size()!=1U||events[0]!="invalid-preserve"||timeline.Sample("crop-growth", 0.5F, invalidPlayback, value)||timeline.LastError()!=AnimationError::InvalidPlayback||value!=77.0F) return 6;
    std::printf("ANIMATION_TIMELINE_SMOKE_OK tracks=1 interpolate=1 clamp=1 loop=1 markers=1 validation=1\n"); return 0;
}
