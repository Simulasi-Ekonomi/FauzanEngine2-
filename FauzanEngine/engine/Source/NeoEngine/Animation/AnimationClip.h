#pragma once

#include <vector>
#include "Bone.h"

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

private:

    [[maybe_unused]] std::vector<std::vector<Keyframe>> tracks;
    float duration = 0;

};

}
