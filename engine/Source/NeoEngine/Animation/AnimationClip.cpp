#include <cassert>
#include "AnimationClip.h"

namespace NeoEngine
{

void AnimationClip::AddKeyframe(int bone,const Keyframe& frame)
{
    if(static_cast<size_t>(static_cast<size_t>(bone)) >= tracks.size())
        tracks.resize(bone+1);

    tracks[bone].push_back(frame);

    if(frame.time > duration)
        duration = frame.time;
}

const std::vector<Keyframe>& AnimationClip::GetFrames(int bone) const
{
    return tracks[bone];
}

float AnimationClip::GetDuration() const
{
    return duration;
}

}
