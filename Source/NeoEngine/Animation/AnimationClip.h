#pragma once

#include <vector>
#include "Bone.h"
#include <cstddef>
#include <cstdint>

namespace NeoEngine
{

struct Keyframe
{
    [[maybe_unused]] float time;
    Mat4 transform;
};

class AnimationClip
{
public:

    void AddKeyframe(int bone,const Keyframe& frame);

    const std::vector<Keyframe>& GetFrames(int bone) const;

    float GetDuration() const;
    bool Sample(int bone, float time, Mat4& out) const;

private:

    std::vector<std::vector<Keyframe>> tracks;
    float duration = 0;

};

}
