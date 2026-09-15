#pragma once

#include "AudioMixer.h"
#include <cstdint>
#include <vector>

namespace NeoEngine {

class AudioComponent {
public:
    AudioComponent() = default;
    ~AudioComponent() = default;

    void SetSound(uint32_t soundId, std::vector<int16_t> monoSamples);
    void SetPosition(const AudioVector3& pos);
    void SetAttenuation(const AudioAttenuationSettings& att);
    void SetPitch(float pitch);
    void SetGain(uint16_t gainQ8);
    void SetLooping(bool looping);
    void SetSpatialized(bool spatialized);

    bool Play(AudioMixer& mixer);
    bool Stop(AudioMixer& mixer);
    void Update(AudioMixer& mixer, const AudioVector3& worldPosition);

    [[nodiscard]] uint32_t SoundId() const { return soundId_; }
    [[nodiscard]] bool IsPlaying() const { return isPlaying_; }
    [[nodiscard]] const AudioVector3& Position() const { return position_; }
    [[nodiscard]] float Pitch() const { return pitch_; }
    [[nodiscard]] uint16_t Gain() const { return gainQ8_; }

private:
    uint32_t soundId_ = 0;
    std::vector<int16_t> samples_;
    AudioVector3 position_{0.0f, 0.0f, 0.0f};
    AudioAttenuationSettings attenuation_{};
    float pitch_ = 1.0f;
    uint16_t gainQ8_ = 256;
    bool looping_ = false;
    bool spatialized_ = true;
    bool isPlaying_ = false;
};

} // namespace NeoEngine
