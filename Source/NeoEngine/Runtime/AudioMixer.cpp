#include "AudioMixer.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace NeoEngine {
namespace {

float Dot3(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float Length3(const float v[3]) {
    return std::sqrt(Dot3(v, v));
}

bool Normalize3(float v[3]) {
    const float length = Length3(v);
    if (!std::isfinite(length) || length <= 1.0e-5f) return false;
    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
    return true;
}

} // namespace

bool AudioMixer::Play(uint32_t id, std::vector<int16_t> samples, uint16_t gainQ8, bool looping, float pitch) {
    if (id == 0 || samples.empty() || samples.size() > kMaxSamplesPerVoice || gainQ8 == 0 || !std::isfinite(pitch) || pitch <= 0.001f || pitch > 8.0f) return false;
    for (const auto& voice : m_Voices) if (voice.id == id) return false;
    if (m_Voices.size() >= kMaxVoices) return false;

    Voice voice;
    voice.id = id;
    voice.samples = std::move(samples);
    voice.gain = gainQ8;
    voice.looping = looping;
    voice.pitch = pitch;
    voice.spatialized = false;
    m_Voices.push_back(std::move(voice));
    return true;
}

bool AudioMixer::PlaySpatial(const SpatialVoiceParams& params) {
    if (params.id == 0 || params.mono.empty() || params.mono.size() > kMaxSamplesPerVoice || params.gainQ8 == 0) return false;
    for (const auto& voice : m_Voices) if (voice.id == params.id) return false;
    if (m_Voices.size() >= kMaxVoices) return false;
    for (float x : params.position) if (!std::isfinite(x)) return false;

    const float dx = params.position[0] - m_Listener.position[0];
    const float dy = params.position[1] - m_Listener.position[1];
    const float dz = params.position[2] - m_Listener.position[2];
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!std::isfinite(distance)) return false;

    float attenuation = 1.0f;
    if (!std::isfinite(params.attenuation.minDistance) ||
        !std::isfinite(params.attenuation.maxDistance) ||
        !std::isfinite(params.attenuation.minVolume)) {
        return false;
    }
    const float minD = std::max(0.001f, params.attenuation.minDistance);
    const float maxD = std::max(minD + 0.001f, params.attenuation.maxDistance);
    const float minVolume = std::clamp(params.attenuation.minVolume, 0.0f, 1.0f);
    if (params.spatialized) {
        if (distance >= maxD) {
            attenuation = minVolume;
        } else if (distance > minD) {
            const float t = (distance - minD) / (maxD - minD);
            attenuation = params.attenuation.model == AudioAttenuationModel::Linear
                ? 1.0f - t
                : 1.0f / (1.0f + t * t * (maxD / minD));
            attenuation = std::max(minVolume, attenuation);
        }
    }

    float pan = 0.0f;
    if (params.spatialized && distance > 0.001f) {
        float forward[3]{m_Listener.forward[0], m_Listener.forward[1], m_Listener.forward[2]};
        float up[3]{m_Listener.up[0], m_Listener.up[1], m_Listener.up[2]};
        if (Normalize3(forward) && Normalize3(up)) {
            const float forwardDotUp = Dot3(forward, up);
            up[0] -= forward[0] * forwardDotUp;
            up[1] -= forward[1] * forwardDotUp;
            up[2] -= forward[2] * forwardDotUp;
            if (Normalize3(up)) {
                const float right[3]{
                    up[1] * forward[2] - up[2] * forward[1],
                    up[2] * forward[0] - up[0] * forward[2],
                    up[0] * forward[1] - up[1] * forward[0]
                };
                const float source[3]{dx / distance, dy / distance, dz / distance};
                pan = std::clamp(Dot3(source, right), -1.0f, 1.0f);
            }
        }
    }

    Voice voice;
    voice.id = params.id;
    voice.samples = params.mono;
    voice.gain = params.gainQ8;
    voice.pan = pan;
    voice.pitch = params.pitch;
    voice.position[0] = params.position[0];
    voice.position[1] = params.position[1];
    voice.position[2] = params.position[2];
    voice.attenuation = params.attenuation;
    voice.looping = params.looping;
    voice.spatialized = params.spatialized;
    m_Voices.push_back(std::move(voice));
    return true;
}

bool AudioMixer::UpdateVoicePosition(uint32_t id, const float position[3]) {
    if (id == 0 || position == nullptr) return false;
    for (float value : {position[0], position[1], position[2]}) if (!std::isfinite(value)) return false;
    for (auto& voice : m_Voices) {
        if (voice.id == id) {
            voice.position[0] = position[0];
            voice.position[1] = position[1];
            voice.position[2] = position[2];
            return true;
        }
    }
    return false;
}

bool AudioMixer::UpdateVoicePitch(uint32_t id, float pitch) {
    if (id == 0 || !std::isfinite(pitch) || pitch <= 0.001f || pitch > 8.0f) return false;
    for (auto& voice : m_Voices) if (voice.id == id) { voice.pitch = pitch; return true; }
    return false;
}

bool AudioMixer::UpdateVoiceGain(uint32_t id, uint16_t gainQ8) {
    if (id == 0 || gainQ8 == 0) return false;
    for (auto& voice : m_Voices) if (voice.id == id) { voice.gain = gainQ8; return true; }
    return false;
}

bool AudioMixer::Stop(uint32_t id) {
    if (id == 0) return false;
    auto it = std::find_if(m_Voices.begin(), m_Voices.end(), [&](const auto& voice) { return voice.id == id; });
    if (it == m_Voices.end()) return false;
    m_Voices.erase(it);
    return true;
}

void AudioMixer::Clear() { m_Voices.clear(); }

void AudioMixer::Mix(size_t frames, std::vector<int16_t>& out) {
    out.assign(frames * 2U, 0);
    for (size_t f = 0; f < frames; ++f) {
        int64_t left = 0;
        int64_t right = 0;
        for (auto& voice : m_Voices) {
            if (voice.samples.empty()) continue;
            if (voice.cursorSubframe >= static_cast<double>(voice.samples.size())) {
                if (voice.looping) voice.cursorSubframe = std::fmod(voice.cursorSubframe, static_cast<double>(voice.samples.size()));
                else continue;
            }

            const double position = voice.cursorSubframe;
            const size_t idx0 = static_cast<size_t>(position);
            const size_t idx1 = idx0 + 1U < voice.samples.size() ? idx0 + 1U : (voice.looping ? 0U : idx0);
            const double frac = position - static_cast<double>(idx0);
            const int32_t s0 = voice.samples[idx0];
            const int32_t s1 = voice.samples[idx1];
            const int32_t interpolated = static_cast<int32_t>(std::llround(static_cast<double>(s0) + frac * static_cast<double>(s1 - s0)));
            float dynamicGain = static_cast<float>(voice.gain) / 256.0f;
            float dynamicPan = voice.pan;
            if (voice.spatialized) {
                const float dx = voice.position[0] - m_Listener.position[0];
                const float dy = voice.position[1] - m_Listener.position[1];
                const float dz = voice.position[2] - m_Listener.position[2];
                const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                const float minD = std::max(0.001f, voice.attenuation.minDistance);
                const float maxD = std::max(minD + 0.001f, voice.attenuation.maxDistance);
                const float minVolume = std::clamp(voice.attenuation.minVolume, 0.0f, 1.0f);
                float attenuation = 1.0f;
                if (distance >= maxD) attenuation = minVolume;
                else if (distance > minD) {
                    const float t = (distance - minD) / (maxD - minD);
                    if (voice.attenuation.model == AudioAttenuationModel::Linear) attenuation = 1.0f - t;
                    else if (voice.attenuation.model == AudioAttenuationModel::Logarithmic) attenuation = 1.0f - std::log10(1.0f + 9.0f * t);
                    else attenuation = 1.0f / (1.0f + t * t * (maxD / minD));
                    attenuation = std::max(minVolume, attenuation);
                }
                dynamicGain *= attenuation;
                if (distance > 0.001f) {
                    float forward[3]{m_Listener.forward[0], m_Listener.forward[1], m_Listener.forward[2]};
                    float up[3]{m_Listener.up[0], m_Listener.up[1], m_Listener.up[2]};
                    if (Normalize3(forward) && Normalize3(up)) {
                        const float fu = Dot3(forward, up);
                        up[0] -= forward[0] * fu; up[1] -= forward[1] * fu; up[2] -= forward[2] * fu;
                        if (Normalize3(up)) {
                            const float rightAxis[3]{up[1]*forward[2]-up[2]*forward[1], up[2]*forward[0]-up[0]*forward[2], up[0]*forward[1]-up[1]*forward[0]};
                            dynamicPan = std::clamp((dx*rightAxis[0]+dy*rightAxis[1]+dz*rightAxis[2])/distance, -1.0f, 1.0f);
                        }
                    }
                }
            }
            const int64_t sample = static_cast<int64_t>(std::llround(static_cast<double>(interpolated) * dynamicGain));
            voice.cursorSubframe += voice.pitch;
            voice.cursor = static_cast<size_t>(voice.cursorSubframe);
            if (!voice.spatialized) {
                left += sample;
                right += sample;
            } else {
                const float pan = std::clamp(dynamicPan, -1.0f, 1.0f);
                constexpr float kHalfPi = 1.57079632679489661923f;
                const float angle = (pan + 1.0f) * 0.5f * kHalfPi;
                const float leftGain = std::cos(angle);
                const float rightGain = std::sin(angle);
                left += static_cast<int64_t>(std::llround(static_cast<double>(sample) * leftGain));
                right += static_cast<int64_t>(std::llround(static_cast<double>(sample) * rightGain));
            }
        }
        out[f * 2U] = static_cast<int16_t>(std::clamp<int64_t>(left, -32768, 32767));
        out[f * 2U + 1U] = static_cast<int16_t>(std::clamp<int64_t>(right, -32768, 32767));
    }
    m_Voices.erase(std::remove_if(m_Voices.begin(), m_Voices.end(), [](const auto& voice) {
        return !voice.looping && voice.cursorSubframe >= static_cast<double>(voice.samples.size());
    }), m_Voices.end());
}

} // namespace NeoEngine
