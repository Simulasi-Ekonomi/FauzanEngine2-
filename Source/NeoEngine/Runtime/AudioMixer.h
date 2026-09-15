#pragma once
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

namespace NeoEngine {

struct AudioVector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct AudioListener {
    AudioVector3 position{0.0f, 0.0f, 0.0f};
    AudioVector3 forward{0.0f, 0.0f, 1.0f};
    AudioVector3 up{0.0f, 1.0f, 0.0f};
};

enum class AudioAttenuationModel : uint8_t {
    None,
    Linear,
    Logarithmic,
    InverseSquare
};

struct AudioAttenuationSettings {
    AudioAttenuationModel model = AudioAttenuationModel::Linear;
    float minDistance = 1.0f;
    float maxDistance = 50.0f;
    float minVolume = 0.0f;
};

struct SpatialVoiceParams {
    uint32_t id = 0;
    std::vector<int16_t> mono;
    uint16_t gainQ8 = 256;
    float pitch = 1.0f;
    bool looping = false;
    bool spatialized = false;
    AudioVector3 position{0.0f, 0.0f, 0.0f};
    AudioAttenuationSettings attenuation{};
};

class AudioMixer {
public:
    static constexpr size_t kMaxVoices = 64;
    static constexpr size_t kMaxSamplesPerVoice = 480000;

    AudioMixer() = default;
    ~AudioMixer() = default;

    bool Play(uint32_t id, std::vector<int16_t> mono, uint16_t gainQ8 = 256);
    bool PlaySpatial(const SpatialVoiceParams& params);
    bool Stop(uint32_t id);
    void Clear();
    void Mix(size_t frames, std::vector<int16_t>& stereo);
    [[nodiscard]] size_t ActiveVoices() const { return m_Voices.size(); }

    void SetListener(const AudioListener& listener) { m_Listener = listener; }
    [[nodiscard]] const AudioListener& GetListener() const { return m_Listener; }

    bool UpdateVoicePosition(uint32_t id, const AudioVector3& position);
    bool UpdateVoicePitch(uint32_t id, float pitch);
    bool UpdateVoiceGain(uint32_t id, uint16_t gainQ8);

private:
    struct Voice {
        uint32_t id = 0;
        std::vector<int16_t> samples{};
        size_t cursor = 0;
        double cursorSubframe = 0.0;
        uint16_t gain = 256;
        float pitch = 1.0f;
        bool looping = false;
        bool spatialized = false;
        AudioVector3 position{0.0f, 0.0f, 0.0f};
        AudioAttenuationSettings attenuation{};
    };

    std::vector<Voice> m_Voices{};
    AudioListener m_Listener{};
};

} // namespace NeoEngine
