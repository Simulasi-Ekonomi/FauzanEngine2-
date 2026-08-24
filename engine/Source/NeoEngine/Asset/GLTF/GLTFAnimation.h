#include <cassert>
#pragma once
#include <vector>

struct AnimationKeyframe
{
    [[maybe_unused]] float time;
    float value[4];
};

struct AnimationChannel
{
    [[maybe_unused]] int node;
    [[maybe_unused]] std::vector<AnimationKeyframe> keyframes;
};

class GLTFAnimation
{
public:

    void AddChannel(const AnimationChannel& channel);

    const std::vector<AnimationChannel>& GetChannels() const;

private:

    [[maybe_unused]] std::vector<AnimationChannel> channels;
};
