#include "AudioComponent.h"

#include <cmath>
#include <utility>

namespace NeoEngine {

bool AudioComponent::SetVoiceId(uint32_t voiceId) {
    if (voiceId == 0) return false;
    voiceId_ = voiceId;
    return true;
}

bool AudioComponent::SetSamples(std::vector<int16_t> mono) {
    if (mono.empty() || mono.size() > kMaxSamples) return false;
    samples_ = std::move(mono);
    return true;
}

void AudioComponent::ClearSamples() {
    samples_.clear();
    samples_.shrink_to_fit();
}

bool AudioComponent::Play(AudioMixer& mixer) const {
    if (voiceId_ == 0 || samples_.empty() || gainQ8_ == 0) return false;
    if (!spatialized_) return mixer.Play(voiceId_, samples_, gainQ8_);
    SpatialVoiceParams params;
    params.id = voiceId_;
    params.mono = samples_;
    params.spatialized = true;
    params.position[0] = position_[0];
    params.position[1] = position_[1];
    params.position[2] = position_[2];
    params.attenuation = attenuation_;
    params.gainQ8 = gainQ8_;
    params.looping = looping_;
    if (!std::isfinite(position_[0]) || !std::isfinite(position_[1]) || !std::isfinite(position_[2])) return false;
    return mixer.PlaySpatial(params);
}

bool AudioComponent::Stop(AudioMixer& mixer) const {
    return voiceId_ != 0 && mixer.Stop(voiceId_);
}

} // namespace NeoEngine
