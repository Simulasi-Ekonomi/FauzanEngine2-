#include "AnimationPlayer.h"
#include <cmath>
namespace NeoEngine {
void AnimationPlayer::Play(AnimationClip* clip){ if(!clip){currentClip=nullptr;time=0.0f;return;} currentClip=clip;time=0.0f; }
void AnimationPlayer::Update(float dt){ if(!currentClip || !std::isfinite(dt) || dt<0.0f)return; const float duration=currentClip->GetDuration(); if(!std::isfinite(duration)||duration<=0.0f){time=0.0f;return;} time=std::fmod(time+dt,duration); if(time<0.0f)time+=duration; }
}