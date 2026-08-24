#pragma once

#include "Skeleton.h"
#include "AnimationClip.h"

namespace NeoEngine
{

class AnimationPlayer
{
public:

    void Play(AnimationClip* clip);

    void Update(float dt);

private:

    AnimationClip* currentClip = nullptr;

    float time = 0;

};

}
