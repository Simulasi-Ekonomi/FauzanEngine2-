#include "WavAudioParser.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace NeoEngine {

bool WavAudioParser::Parse(const std::vector<uint8_t>& wavBytes, WavAudioData& outData) {
    if (wavBytes.size() < 44) return false;

    if (std::memcmp(wavBytes.data(), "RIFF", 4) != 0) return false;
    if (std::memcmp(wavBytes.data() + 8, "WAVE", 4) != 0) return false;

    size_t offset = 12;
    bool foundFmt = false;
    bool foundData = false;
    size_t dataOffset = 0;
    uint32_t dataSize = 0;

    while (offset + 8 <= wavBytes.size()) {
        char chunkId[5] = {0};
        std::memcpy(chunkId, wavBytes.data() + offset, 4);
        uint32_t chunkSize = 0;
        std::memcpy(&chunkSize, wavBytes.data() + offset + 4, 4);
        offset += 8;

        if (std::strcmp(chunkId, "fmt ") == 0) {
            if (chunkSize < 16 || offset + chunkSize > wavBytes.size()) return false;
            std::memcpy(&outData.audioFormat, wavBytes.data() + offset, 2);
            std::memcpy(&outData.numChannels, wavBytes.data() + offset + 2, 2);
            std::memcpy(&outData.sampleRate, wavBytes.data() + offset + 4, 4);
            std::memcpy(&outData.byteRate, wavBytes.data() + offset + 8, 4);
            std::memcpy(&outData.blockAlign, wavBytes.data() + offset + 12, 2);
            std::memcpy(&outData.bitsPerSample, wavBytes.data() + offset + 14, 2);
            foundFmt = true;
            offset += chunkSize;
        } else if (std::strcmp(chunkId, "data") == 0) {
            dataOffset = offset;
            dataSize = std::min(chunkSize, static_cast<uint32_t>(wavBytes.size() - offset));
            foundData = true;
            break;
        } else {
            offset += chunkSize;
        }
    }

    if (!foundFmt || !foundData || outData.audioFormat != 1) return false;

    outData.pcmSamples.clear();
    if (outData.bitsPerSample == 16) {
        size_t sampleCount = dataSize / 2;
        if (outData.numChannels == 1) {
            outData.pcmSamples.resize(sampleCount);
            std::memcpy(outData.pcmSamples.data(), wavBytes.data() + dataOffset, sampleCount * 2);
        } else if (outData.numChannels == 2) {
            size_t frameCount = sampleCount / 2;
            outData.pcmSamples.resize(frameCount);
            const int16_t* src = reinterpret_cast<const int16_t*>(wavBytes.data() + dataOffset);
            for (size_t i = 0; i < frameCount; ++i) {
                int32_t left = src[i * 2];
                int32_t right = src[i * 2 + 1];
                outData.pcmSamples[i] = static_cast<int16_t>((left + right) / 2);
            }
        } else {
            return false;
        }
    } else if (outData.bitsPerSample == 8) {
        size_t sampleCount = dataSize;
        if (outData.numChannels == 1) {
            outData.pcmSamples.resize(sampleCount);
            const uint8_t* src = wavBytes.data() + dataOffset;
            for (size_t i = 0; i < sampleCount; ++i) {
                outData.pcmSamples[i] = static_cast<int16_t>((static_cast<int32_t>(src[i]) - 128) * 256);
            }
        } else if (outData.numChannels == 2) {
            size_t frameCount = sampleCount / 2;
            outData.pcmSamples.resize(frameCount);
            const uint8_t* src = wavBytes.data() + dataOffset;
            for (size_t i = 0; i < frameCount; ++i) {
                int32_t left = (static_cast<int32_t>(src[i * 2]) - 128) * 256;
                int32_t right = (static_cast<int32_t>(src[i * 2 + 1]) - 128) * 256;
                outData.pcmSamples[i] = static_cast<int16_t>((left + right) / 2);
            }
        } else {
            return false;
        }
    } else {
        return false;
    }

    return !outData.pcmSamples.empty();
}

std::vector<uint8_t> WavAudioParser::GenerateSyntheticWav(uint32_t sampleRate, uint16_t numChannels, float frequencyHz, float durationSeconds) {
    uint32_t totalFrames = static_cast<uint32_t>(sampleRate * durationSeconds);
    uint16_t bitsPerSample = 16;
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    uint32_t byteRate = sampleRate * blockAlign;
    uint32_t dataSize = totalFrames * blockAlign;
    uint32_t chunkSize = 36 + dataSize;

    std::vector<uint8_t> buffer(44 + dataSize);

    std::memcpy(buffer.data(), "RIFF", 4);
    std::memcpy(buffer.data() + 4, &chunkSize, 4);
    std::memcpy(buffer.data() + 8, "WAVE", 4);

    std::memcpy(buffer.data() + 12, "fmt ", 4);
    uint32_t fmtChunkSize = 16;
    uint16_t audioFormat = 1;
    std::memcpy(buffer.data() + 16, &fmtChunkSize, 4);
    std::memcpy(buffer.data() + 20, &audioFormat, 2);
    std::memcpy(buffer.data() + 22, &numChannels, 2);
    std::memcpy(buffer.data() + 24, &sampleRate, 4);
    std::memcpy(buffer.data() + 28, &byteRate, 4);
    std::memcpy(buffer.data() + 32, &blockAlign, 2);
    std::memcpy(buffer.data() + 34, &bitsPerSample, 2);

    std::memcpy(buffer.data() + 36, "data", 4);
    std::memcpy(buffer.data() + 40, &dataSize, 4);

    int16_t* pcmPtr = reinterpret_cast<int16_t*>(buffer.data() + 44);
    const double pi = 3.14159265358979323846;
    for (uint32_t f = 0; f < totalFrames; ++f) {
        double time = static_cast<double>(f) / static_cast<double>(sampleRate);
        int16_t sample = static_cast<int16_t>(16000.0 * std::sin(2.0 * pi * frequencyHz * time));
        for (uint16_t c = 0; c < numChannels; ++c) {
            *pcmPtr++ = sample;
        }
    }

    return buffer;
}

} // namespace NeoEngine
