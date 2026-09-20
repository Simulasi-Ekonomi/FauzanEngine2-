#pragma once

#include "AudioMixer.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace NeoEngine { class SdlAudioBridge; }

namespace NeoEngine {

class AudioComponent {
public:
    static constexpr size_t kMaxSamples = AudioMixer::kMaxSamplesPerVoice;

    explicit AudioComponent(uint32_t voiceId = 0) : voiceId_(voiceId) {}

    bool SetVoiceId(uint32_t voiceId);
    [[nodiscard]] uint32_t VoiceId() const { return voiceId_; }
    bool SetSamples(std::vector<int16_t> mono);
    void ClearSamples();
    [[nodiscard]] bool HasSamples() const { return !samples_.empty(); }
    [[nodiscard]] size_t SampleCount() const { return samples_.size(); }
    void SetGainQ8(uint16_t gainQ8) { gainQ8_ = gainQ8; }
    [[nodiscard]] uint16_t GainQ8() const { return gainQ8_; }
    void SetSpatialized(bool enabled) { spatialized_ = enabled; }
    [[nodiscard]] bool IsSpatialized() const { return spatialized_; }
    void SetPosition(float x, float y, float z) { position_[0] = x; position_[1] = y; position_[2] = z; }
    [[nodiscard]] const float* Position() const { return position_; }
    void SetAttenuation(const AudioAttenuation& attenuation) { attenuation_ = attenuation; }
    [[nodiscard]] const AudioAttenuation& Attenuation() const { return attenuation_; }
    void SetLooping(bool looping) { looping_ = looping; }
    [[nodiscard]] bool IsLooping() const { return looping_; }
    void SetPitch(float pitch) { pitch_ = pitch; }
    [[nodiscard]] float Pitch() const { return pitch_; }
    bool Play(AudioMixer& mixer) const;
    bool Play(SdlAudioBridge& bridge) const;
    bool Stop(AudioMixer& mixer) const;
    bool Stop(SdlAudioBridge& bridge) const;

private:
    uint32_t voiceId_ = 0;
    std::vector<int16_t> samples_;
    uint16_t gainQ8_ = 256;
    bool spatialized_ = false;
    bool looping_ = false;
    float pitch_ = 1.0f;
    float position_[3]{0.0f, 0.0f, 0.0f};
    AudioAttenuation attenuation_{};
};

} // namespace NeoEngine
