#pragma once
#include <cmath>
#include <cstddef>
#include <vector>

struct AnimationKeyframe {
    float time = 0.0F;
    float value[4] = {0.0F, 0.0F, 0.0F, 1.0F};
};

struct AnimationChannel {
    int node = -1;
    std::vector<AnimationKeyframe> keyframes;
};

class GLTFAnimation {
public:
    void AddChannel(const AnimationChannel& channel);
    [[nodiscard]] bool AddChannelChecked(const AnimationChannel& channel);
    [[nodiscard]] const std::vector<AnimationChannel>& GetChannels() const noexcept { return channels; }
    [[nodiscard]] bool Validate() const noexcept;

private:
    std::vector<AnimationChannel> channels;
};