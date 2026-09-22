#include "AnimationPlayer.h"
#include <cmath>
namespace NeoEngine { void AnimationPlayer::Play(AnimationClip*c){if(!c){currentClip=nullptr;time=0;return;}currentClip=c;time=0;} void AnimationPlayer::Update(float dt){if(!currentClip||!std::isfinite(dt)||dt<0)return;float d=currentClip->GetDuration();if(!std::isfinite(d)||d<=0){time=0;return;}time=std::fmod(time+dt,d);if(time<0)time+=d;} }