#include "WavAudioParser.h"

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
    if (bytes.size() > 64U * 1024U * 1024U) return false;
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) return false;
    bool fmtFound = false, dataFound = false;
    uint16_t format = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    size_t dataOffset = 0, dataSize = 0, offset = 12;
    while (offset + 8 <= bytes.size()) {
        const uint8_t* header = bytes.data() + offset;
        const uint32_t chunkSize = U32(header + 4);
        if (chunkSize > bytes.size() - offset) return false;
        offset += 8;
        if (chunkSize > bytes.size() - offset) return false;
        if (std::memcmp(header, "fmt ", 4) == 0) {
            if (fmtFound || chunkSize < 16U) return false;
            const uint8_t* p = bytes.data() + offset;
            format = U16(p); channels = U16(p + 2); rate = U32(p + 4); bits = U16(p + 14);
            fmtFound = true;
        } else if (std::memcmp(header, "data", 4) == 0 && !dataFound) {
            dataOffset = offset; dataSize = chunkSize; dataFound = true;
        }
        offset += chunkSize;
        if ((chunkSize & 1U) != 0U) {
            if (offset >= bytes.size()) return false;
            ++offset;
        }
    }
    if (offset != bytes.size() || !fmtFound || !dataFound || format != 1 || channels == 0 || channels > 2 || rate == 0 ||
        (bits != 8 && bits != 16) || dataSize % (static_cast<size_t>(channels) * (bits / 8U)) != 0) return false;
    const size_t bytesPerSample = bits / 8U;
    const size_t frameBytes = static_cast<size_t>(channels) * bytesPerSample;
    if (frameBytes == 0U || dataSize < frameBytes) return false;
    const size_t frameCount = dataSize / frameBytes;
    if (frameCount == 0U || frameCount > 48000U * 60U * 60U) return false;
    if (frameCount > std::numeric_limits<size_t>::max() / static_cast<size_t>(channels)) return false;
    const size_t sampleCount = frameCount * channels;
    if (sampleCount > std::vector<int16_t>().max_size() || sampleCount > 48000ULL * 60ULL * 60ULL * 2ULL) return false;
    if (dataOffset > bytes.size() || dataSize > bytes.size() - dataOffset) return false;
    out.sampleRate = rate; out.channels = channels;
    try { out.pcmSamples.resize(frameCount); } catch (...) { out = {}; return false; }
    if (bits == 16) {
        for (size_t frame = 0; frame < frameCount; ++frame) {
            int32_t sum = 0;
            for (uint16_t channel = 0; channel < channels; ++channel)
                sum += static_cast<int16_t>(U16(bytes.data() + dataOffset + frame * frameBytes + channel * 2U));
            out.pcmSamples[frame] = static_cast<int16_t>(sum / static_cast<int32_t>(channels));
        }
    } else {
        for (size_t frame = 0; frame < frameCount; ++frame) {
            int32_t sum = 0;
            for (uint16_t channel = 0; channel < channels; ++channel) {
                const uint8_t sample = bytes[dataOffset + frame * frameBytes + channel];
                sum += (static_cast<int32_t>(sample) - 128) * 256;
            }
            out.pcmSamples[frame] = static_cast<int16_t>(sum / static_cast<int32_t>(channels));
        }
    }
    return true;
}

std::vector<uint8_t> WavAudioParser::GenerateSyntheticWav(uint32_t sampleRate, uint16_t channels, float frequencyHz, float durationSeconds) {
    if (sampleRate == 0 || channels == 0 || channels > 2 || !std::isfinite(frequencyHz) || frequencyHz < 0.0f || !std::isfinite(durationSeconds) || durationSeconds <= 0.0f) return {};
    const double requested = static_cast<double>(sampleRate) * static_cast<double>(durationSeconds);
    if (requested > static_cast<double>(std::numeric_limits<uint32_t>::max()) || requested < 1.0) return {};
    const uint32_t frames = static_cast<uint32_t>(std::llround(requested));
    if (frames == 0) return {};
    if (static_cast<uint64_t>(frames) > std::numeric_limits<uint64_t>::max() / channels) return {};
    const uint64_t sampleCount = static_cast<uint64_t>(frames) * channels;
    if (sampleCount > std::numeric_limits<uint64_t>::max() / 2U) return {};
    const uint64_t dataBytes = sampleCount * 2U;
    const uint64_t byteRate = static_cast<uint64_t>(sampleRate) * channels * 2U;
    if (dataBytes > std::numeric_limits<uint32_t>::max() - 36U || byteRate > std::numeric_limits<uint32_t>::max()) return {};
    const uint64_t outputBytes = 44U + dataBytes;
    if (outputBytes > std::vector<uint8_t>().max_size()) return {};
    std::vector<uint8_t> b; b.reserve(static_cast<size_t>(outputBytes));
    b.insert(b.end(), {'R','I','F','F'}); Put32(b, static_cast<uint32_t>(36U + dataBytes)); b.insert(b.end(), {'W','A','V','E'});
    b.insert(b.end(), {'f','m','t',' '}); Put32(b, 16); Put16(b, 1); Put16(b, channels); Put32(b, sampleRate); Put32(b, static_cast<uint32_t>(byteRate)); Put16(b, static_cast<uint16_t>(channels * 2U)); Put16(b, 16);
    b.insert(b.end(), {'d','a','t','a'}); Put32(b, static_cast<uint32_t>(dataBytes));
    for (uint32_t f = 0; f < frames; ++f) { const int16_t s = static_cast<int16_t>(std::sin(2.0 * 3.141592653589793 * frequencyHz * f / sampleRate) * 12000.0); for (uint16_t c = 0; c < channels; ++c) Put16(b, static_cast<uint16_t>(s)); }
    return b;
}
} // namespace NeoEngine
