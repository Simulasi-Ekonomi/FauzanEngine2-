#pragma once

#include <cstdint>
#include <vector>

namespace NeoEngine {

struct WavAudioData {
    uint16_t audioFormat = 1; // 1 = PCM
    uint16_t numChannels = 1; // 1 = Mono, 2 = Stereo
    uint32_t sampleRate = 44100;
    uint32_t byteRate = 88200;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    std::vector<int16_t> pcmSamples;
};

class WavAudioParser {
public:
    static bool Parse(const std::vector<uint8_t>& wavBytes, WavAudioData& outData);
    static std::vector<uint8_t> GenerateSyntheticWav(uint32_t sampleRate, uint16_t numChannels, float frequencyHz, float durationSeconds);
};

} // namespace NeoEngine
