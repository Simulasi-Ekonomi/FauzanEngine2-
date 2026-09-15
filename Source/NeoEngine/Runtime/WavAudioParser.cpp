#include "WavAudioParser.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace NeoEngine {
namespace {
uint16_t U16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8)); }
uint32_t U32(const uint8_t* p) { return static_cast<uint32_t>(p[0] | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24)); }
void Put16(std::vector<uint8_t>& b, uint16_t v) { b.push_back(static_cast<uint8_t>(v)); b.push_back(static_cast<uint8_t>(v >> 8)); }
void Put32(std::vector<uint8_t>& b, uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(v >> (i * 8))); }
}

bool WavAudioParser::Parse(const std::vector<uint8_t>& bytes, WavAudioData& out) {
    out = {};
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) return false;
    bool fmtFound = false, dataFound = false;
    uint16_t format = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    size_t dataOffset = 0, dataSize = 0, offset = 12;
    while (offset + 8 <= bytes.size()) {
        const uint8_t* header = bytes.data() + offset;
        const uint32_t chunkSize = U32(header + 4);
        offset += 8;
        if (chunkSize > bytes.size() - offset) return false;
        if (std::memcmp(header, "fmt ", 4) == 0) {
            if (chunkSize < 16) return false;
            const uint8_t* p = bytes.data() + offset;
            format = U16(p); channels = U16(p + 2); rate = U32(p + 4); bits = U16(p + 14);
            fmtFound = true;
        } else if (std::memcmp(header, "data", 4) == 0 && !dataFound) {
            dataOffset = offset; dataSize = chunkSize; dataFound = true;
        }
        offset += chunkSize;
        if ((chunkSize & 1U) != 0) { if (offset == bytes.size()) break; ++offset; }
    }
    if (!fmtFound || !dataFound || format != 1 || channels == 0 || channels > 2 || rate == 0 || bits != 16 || dataSize % (static_cast<size_t>(channels) * 2U) != 0) return false;
    const size_t sampleCount = dataSize / 2U;
    if (sampleCount > std::vector<int16_t>().max_size()) return false;
    out.sampleRate = rate; out.channels = channels; out.pcmSamples.resize(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        const uint8_t* p = bytes.data() + dataOffset + i * 2U;
        const uint16_t raw = U16(p);
        out.pcmSamples[i] = static_cast<int16_t>(raw);
    }
    return true;
}

std::vector<uint8_t> WavAudioParser::GenerateSyntheticWav(uint32_t sampleRate, uint16_t channels, float frequencyHz, float durationSeconds) {
    if (sampleRate == 0 || channels == 0 || channels > 2 || !std::isfinite(frequencyHz) || frequencyHz < 0.0f || !std::isfinite(durationSeconds) || durationSeconds <= 0.0f) return {};
    const double requested = static_cast<double>(sampleRate) * durationSeconds;
    if (requested > static_cast<double>(std::numeric_limits<uint32_t>::max())) return {};
    const uint32_t frames = static_cast<uint32_t>(requested);
    const uint64_t sampleCount = static_cast<uint64_t>(frames) * channels;
    const uint64_t dataBytes = sampleCount * 2U;
    if (dataBytes > std::numeric_limits<uint32_t>::max() - 44U) return {};
    std::vector<uint8_t> b; b.reserve(static_cast<size_t>(44U + dataBytes));
    b.insert(b.end(), {'R','I','F','F'}); Put32(b, static_cast<uint32_t>(36U + dataBytes)); b.insert(b.end(), {'W','A','V','E'});
    b.insert(b.end(), {'f','m','t',' '}); Put32(b, 16); Put16(b, 1); Put16(b, channels); Put32(b, sampleRate); Put32(b, sampleRate * channels * 2U); Put16(b, static_cast<uint16_t>(channels * 2U)); Put16(b, 16);
    b.insert(b.end(), {'d','a','t','a'}); Put32(b, static_cast<uint32_t>(dataBytes));
    for (uint32_t f = 0; f < frames; ++f) { const int16_t s = static_cast<int16_t>(std::sin(2.0 * 3.141592653589793 * frequencyHz * f / sampleRate) * 12000.0); for (uint16_t c = 0; c < channels; ++c) Put16(b, static_cast<uint16_t>(s)); }
    return b;
}
} // namespace NeoEngine
