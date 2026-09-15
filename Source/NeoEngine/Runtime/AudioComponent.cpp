#include "AudioComponent.h"

namespace NeoEngine {

void AudioComponent::SetSound(uint32_t soundId, std::vector<int16_t> monoSamples) {
    soundId_ = soundId;
    samples_ = std::move(monoSamples);
}

void AudioComponent::SetPosition(const AudioVector3& pos) {
    position_ = pos;
}

void AudioComponent::SetAttenuation(const AudioAttenuationSettings& att) {
    attenuation_ = att;
}

void AudioComponent::SetPitch(float pitch) {
    pitch_ = pitch;
}

void AudioComponent::SetGain(uint16_t gainQ8) {
    gainQ8_ = gainQ8;
}

void AudioComponent::SetLooping(bool looping) {
    looping_ = looping;
}

void AudioComponent::SetSpatialized(bool spatialized) {
    spatialized_ = spatialized;
}

bool AudioComponent::Play(AudioMixer& mixer) {
    if (soundId_ == 0 || samples_.empty()) return false;
    SpatialVoiceParams params;
    params.id = soundId_;
    params.mono = samples_;
    params.gainQ8 = gainQ8_;
    params.pitch = pitch_;
    params.looping = looping_;
    params.spatialized = spatialized_;
    params.position = position_;
    params.attenuation = attenuation_;

    isPlaying_ = mixer.PlaySpatial(params);
    return isPlaying_;
}

bool AudioComponent::Stop(AudioMixer& mixer) {
    if (!isPlaying_) return false;
    bool stopped = mixer.Stop(soundId_);
    if (stopped) isPlaying_ = false;
    return stopped;
}

void AudioComponent::Update(AudioMixer& mixer, const AudioVector3& worldPosition) {
    position_ = worldPosition;
    if (isPlaying_) {
        mixer.UpdateVoicePosition(soundId_, position_);
    }
}

} // namespace NeoEngine
