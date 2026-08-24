#include <cassert>
#include "GLTFAnimation.h"

void GLTFAnimation::AddChannel(const AnimationChannel& channel)
{
    channels.push_back(channel);
}

const std::vector<AnimationChannel>& GLTFAnimation::GetChannels() const
{
    return channels;
}
