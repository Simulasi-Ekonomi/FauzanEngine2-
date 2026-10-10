#include "GLTFAnimation.h"

#include <algorithm>

void GLTFAnimation::AddChannel(const AnimationChannel& channel) {
    channels.push_back(channel);
}

bool GLTFAnimation::AddChannelChecked(const AnimationChannel& channel) {
    if (channel.node < 0 || channel.keyframes.empty()) return false;
    float previousTime = -1.0F;
    for (const auto& keyframe : channel.keyframes) {
        if (!std::isfinite(keyframe.time) || keyframe.time < 0.0F || keyframe.time < previousTime) return false;
        for (float component : keyframe.value) if (!std::isfinite(component)) return false;
        previousTime = keyframe.time;
    }
    channels.push_back(channel);
    return true;
}

bool GLTFAnimation::Validate() const noexcept {
    if (channels.empty()) return false;
    for (const auto& channel : channels) {
        if (channel.node < 0 || channel.keyframes.empty()) return false;
        float previousTime = -1.0F;
        for (const auto& keyframe : channel.keyframes) {
            if (!std::isfinite(keyframe.time) || keyframe.time < 0.0F || keyframe.time < previousTime) return false;
            for (float component : keyframe.value) if (!std::isfinite(component)) return false;
            previousTime = keyframe.time;
        }
    }
    return true;
}