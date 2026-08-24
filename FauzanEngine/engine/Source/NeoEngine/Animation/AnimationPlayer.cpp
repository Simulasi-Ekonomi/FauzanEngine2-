#include <cassert>
#include "AnimationPlayer.h"

namespace NeoEngine
{

void AnimationPlayer::Play(AnimationClip* clip)
{
    currentClip = clip;
    time = 0;
}

void AnimationPlayer::Update(float dt)
{
    if(!currentClip)
        return;

    time += dt;

    if(time > currentClip->GetDuration())
        time = 0;
}

}
