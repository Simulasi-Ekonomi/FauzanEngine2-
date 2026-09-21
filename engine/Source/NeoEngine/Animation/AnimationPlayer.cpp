#include <cassert>
#include <cmath>
#include <limits>
#include "AnimationPlayer.h"

namespace NeoEngine
{

void AnimationPlayer::Play(AnimationClip* clip)
{
    if (clip == nullptr || !std::isfinite(clip->GetDuration()) || clip->GetDuration() < 0.0f || clip->GetDuration() > 86400.0f) {
        currentClip = nullptr;
        time = 0.0f;
        return;
    }
    currentClip = clip;
    time = 0.0f;
}

void AnimationPlayer::Update(float dt)
{
    if (currentClip == nullptr) { time = 0.0f; return; }
    if (!std::isfinite(dt) || dt < 0.0f || dt > 0.25f) { time = 0.0f; return; }
    const float duration = currentClip->GetDuration();
    if (!std::isfinite(duration) || duration <= 0.0f || duration > 86400.0f) {
        time = 0.0f;
        return;
    }
    if (!std::isfinite(time) || time < 0.0f || time > duration) time = 0.0f;
    if (dt > duration) {
        const float wrapped = std::fmod(dt, duration);
        if (!std::isfinite(wrapped) || wrapped < 0.0f || wrapped >= duration) { time = 0.0f; return; }
        time = wrapped;
        return;
    }
    const float next = time + dt;
    if (!std::isfinite(next) || next < time) {
        time = 0.0f;
        return;
    }
    time = next;
    if (time >= duration) {
        const float wrapped = std::fmod(time, duration);
        if (!std::isfinite(wrapped) || wrapped < 0.0f || wrapped >= duration) { time = 0.0f; return; }
        time = wrapped;
    }
}

}
