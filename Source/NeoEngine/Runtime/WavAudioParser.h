#pragma once

#include <cstdint>
#include <vector>

namespace NeoEngine {

struct WavAudioData {
    uint32_t sampleRate = 0;
    uint16_t channels = 0;
    std::vector<int16_t> pcmSamples;
};

class WavAudioParser {
public:
    static bool Parse(const std::vector<uint8_t>& bytes, WavAudioData& out);
    static std::vector<uint8_t> GenerateSyntheticWav(uint32_t sampleRate, uint16_t channels, float frequencyHz, float durationSeconds);
};

} // namespace NeoEngine
