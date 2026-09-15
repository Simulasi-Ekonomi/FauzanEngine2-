#include "WavAudioParser.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <limits>

namespace NeoEngine {

bool WavAudioParser::Parse(const std::vector<uint8_t>& wavBytes, WavAudioData& outData) {
    outData = WavAudioData{};
    if (wavBytes.size() < 12) return false;

    if (std::memcmp(wavBytes.data(), "RIFF", 4) != 0) return false;
    if (std::memcmp(wavBytes.data() + 8, "WAVE", 4) != 0) return false;

    size_t offset = 12;
    bool foundFmt = false;
    bool foundData = false;
    size_t dataOffset = 0;
    size_t dataSize = 0;

    while (offset + 8 <= wavBytes.size()) {
        char chunkId[5] = {0};
        std::memcpy(chunkId, wavBytes.data() + offset, 4);
        uint32_t chunkSize = 0;
        std::memcpy(&chunkSize, wavBytes.data() + offset + 4, 4);

        size_t chunkDataOffset = offset + 8;
        if (chunkSize > wavBytes.size() || chunkDataOffset > wavBytes.size() - chunkSize) {
            if (std::strcmp(chunkId, "data") == 0 && chunkDataOffset < wavBytes.size()) {
                dataOffset = chunkDataOffset;
                dataSize = wavBytes.size() - chunkDataOffset;
                foundData = true;
                break;
            } else {
                return false;
            }
        }

        if (std::strcmp(chunkId, "fmt ") == 0) {
            if (chunkSize < 16) return false;
            std::memcpy(&outData.audioFormat, wavBytes.data() + chunkDataOffset, 2);
            std::memcpy(&outData.numChannels, wavBytes.data() + chunkDataOffset + 2, 2);
            std::memcpy(&outData.sampleRate, wavBytes.data() + chunkDataOffset + 4, 4);
            std::memcpy(&outData.byteRate, wavBytes.data() + chunkDataOffset + 8, 4);
            std::memcpy(&outData.blockAlign, wavBytes.data() + chunkDataOffset + 12, 2);
            std::memcpy(&outData.bitsPerSample, wavBytes.data() + chunkDataOffset + 14, 2);

            if (outData.audioFormat != 1) return false;
            if (outData.numChannels != 1 && outData.numChannels != 2) return false;
            if (outData.sampleRate < 8000 || outData.sampleRate > 192000) return false;
            if (outData.bitsPerSample != 8 && outData.bitsPerSample != 16) return false;

            uint16_t expectedBlockAlign = outData.numChannels * (outData.bitsPerSample / 8);
            if (outData.blockAlign != expectedBlockAlign) return false;

            uint32_t expectedByteRate = outData.sampleRate * outData.blockAlign;
            if (outData.byteRate != expectedByteRate) return false;

            foundFmt = true;
        } else if (std::strcmp(chunkId, "data") == 0) {
            dataOffset = chunkDataOffset;
            dataSize = chunkSize;
            foundData = true;
            break;
        }

        size_t paddedChunkSize = (static_cast<size_t>(chunkSize) + 1U) & ~1U;
        if (chunkDataOffset > wavBytes.size() - paddedChunkSize) break;
        offset = chunkDataOffset + paddedChunkSize;
    }

    if (!foundFmt || !foundData || dataSize < outData.blockAlign) return false;

    outData.pcmSamples.clear();
    const size_t validBytes = std::min(dataSize, wavBytes.size() - dataOffset);
    const size_t validFrames = validBytes / outData.blockAlign;

    if (validFrames == 0) return false;

    outData.pcmSamples.reserve(validFrames);

    if (outData.bitsPerSample == 16) {
        const int16_t* src = reinterpret_cast<const int16_t*>(wavBytes.data() + dataOffset);
        if (outData.numChannels == 1) {
            outData.pcmSamples.assign(src, src + validFrames);
        } else {
            for (size_t i = 0; i < validFrames; ++i) {
                int32_t left = src[i * 2];
                int32_t right = src[i * 2 + 1];
                outData.pcmSamples.push_back(static_cast<int16_t>((left + right) / 2));
            }
        }
    } else if (outData.bitsPerSample == 8) {
        const uint8_t* src = wavBytes.data() + dataOffset;
        if (outData.numChannels == 1) {
            for (size_t i = 0; i < validFrames; ++i) {
                outData.pcmSamples.push_back(static_cast<int16_t>((static_cast<int32_t>(src[i]) - 128) * 256));
            }
        } else {
            for (size_t i = 0; i < validFrames; ++i) {
                int32_t left = (static_cast<int32_t>(src[i * 2]) - 128) * 256;
                int32_t right = (static_cast<int32_t>(src[i * 2 + 1]) - 128) * 256;
                outData.pcmSamples.push_back(static_cast<int16_t>((left + right) / 2));
            }
        }
    }

    return !outData.pcmSamples.empty();
}

std::vector<uint8_t> WavAudioParser::GenerateSyntheticWav(uint32_t sampleRate, uint16_t numChannels, float frequencyHz, float durationSeconds) {
    if (sampleRate == 0 || numChannels == 0 || durationSeconds <= 0.0f) return {};
    uint32_t totalFrames = static_cast<uint32_t>(sampleRate * durationSeconds);
    if (totalFrames == 0) return {};

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
