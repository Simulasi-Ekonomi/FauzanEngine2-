#include "AnimationPlayer.h"
#include <algorithm>
namespace NeoEngine {
void AnimationPlayer::Play(AnimationClip* clip){ currentClip=clip; time=0.0f; }
void AnimationPlayer::Update(float dt){
    if(!currentClip || dt<=0.0f) return;
    const float duration=currentClip->GetDuration(); if(duration<=0.0f){time=0.0f;return;}
    time=std::fmod(time+dt,duration);
}
}
