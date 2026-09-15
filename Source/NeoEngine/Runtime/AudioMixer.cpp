#include "AudioMixer.h"
#include <algorithm>
#include <cmath>

namespace NeoEngine {

bool AudioMixer::Play(uint32_t id, std::vector<int16_t> mono, uint16_t gainQ8) {
    SpatialVoiceParams params;
    params.id = id;
    params.mono = std::move(mono);
    params.gainQ8 = gainQ8;
    params.pitch = 1.0f;
    params.looping = false;
    params.spatialized = false;
    return PlaySpatial(params);
}

bool AudioMixer::PlaySpatial(const SpatialVoiceParams& params) {
    if (params.mono.empty() || params.mono.size() > kMaxSamplesPerVoice) return false;
    if (params.pitch <= 0.0f) return false;
    for (const auto& v : m_Voices) {
        if (v.id == params.id) return false;
    }
    if (m_Voices.size() >= kMaxVoices) return false;

    Voice v;
    v.id = params.id;
    v.samples = params.mono;
    v.cursor = 0;
    v.cursorSubframe = 0.0;
    v.gain = params.gainQ8;
    v.pitch = params.pitch;
    v.looping = params.looping;
    v.spatialized = params.spatialized;
    v.position = params.position;
    v.attenuation = params.attenuation;

    m_Voices.push_back(std::move(v));
    return true;
}

bool AudioMixer::Stop(uint32_t id) {
    auto it = std::find_if(m_Voices.begin(), m_Voices.end(), [&](const Voice& v) {
        return v.id == id;
    });
    if (it == m_Voices.end()) return false;
    m_Voices.erase(it);
    return true;
}

void AudioMixer::Clear() {
    m_Voices.clear();
}

bool AudioMixer::UpdateVoicePosition(uint32_t id, const AudioVector3& position) {
    for (auto& v : m_Voices) {
        if (v.id == id) {
            v.position = position;
            return true;
        }
    }
    return false;
}

bool AudioMixer::UpdateVoicePitch(uint32_t id, float pitch) {
    if (pitch <= 0.0f) return false;
    for (auto& v : m_Voices) {
        if (v.id == id) {
            v.pitch = pitch;
            return true;
        }
    }
    return false;
}

bool AudioMixer::UpdateVoiceGain(uint32_t id, uint16_t gainQ8) {
    for (auto& v : m_Voices) {
        if (v.id == id) {
            v.gain = gainQ8;
            return true;
        }
    }
    return false;
}

void AudioMixer::Mix(size_t frames, std::vector<int16_t>& out) {
    out.assign(frames * 2, 0);
    if (m_Voices.empty()) return;

    AudioVector3 f = m_Listener.forward;
    AudioVector3 u = m_Listener.up;
    AudioVector3 r{
        f.y * u.z - f.z * u.y,
        f.z * u.x - f.x * u.z,
        f.x * u.y - f.y * u.x
    };
    float rLen = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
    if (rLen > 1e-5f) {
        r.x /= rLen; r.y /= rLen; r.z /= rLen;
    } else {
        r = {1.0f, 0.0f, 0.0f};
    }

    for (size_t fIdx = 0; fIdx < frames; ++fIdx) {
        int64_t sumL = 0;
        int64_t sumR = 0;

        for (auto& v : m_Voices) {
            size_t idx0 = static_cast<size_t>(v.cursorSubframe);
            if (idx0 >= v.samples.size()) {
                if (v.looping && !v.samples.empty()) {
                    v.cursorSubframe = std::fmod(v.cursorSubframe, static_cast<double>(v.samples.size()));
                    idx0 = static_cast<size_t>(v.cursorSubframe);
                } else {
                    continue;
                }
            }

            size_t idx1 = idx0 + 1;
            if (idx1 >= v.samples.size()) {
                idx1 = v.looping ? 0 : idx0;
            }

            double frac = v.cursorSubframe - static_cast<double>(idx0);
            int32_t s0 = v.samples[idx0];
            int32_t s1 = v.samples[idx1];
            int32_t interpolatedSample = static_cast<int32_t>(s0 + frac * (s1 - s0));

            float leftGain = static_cast<float>(v.gain) / 256.0f;
            float rightGain = static_cast<float>(v.gain) / 256.0f;

            if (v.spatialized) {
                float dx = v.position.x - m_Listener.position.x;
                float dy = v.position.y - m_Listener.position.y;
                float dz = v.position.z - m_Listener.position.z;
                float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

                float att = 1.0f;
                if (v.attenuation.model != AudioAttenuationModel::None) {
                    float minD = std::max(0.001f, v.attenuation.minDistance);
                    float maxD = std::max(minD + 0.001f, v.attenuation.maxDistance);
                    if (dist <= minD) {
                        att = 1.0f;
                    } else if (dist >= maxD) {
                        att = v.attenuation.minVolume;
                    } else {
                        float rel = (dist - minD) / (maxD - minD);
                        switch (v.attenuation.model) {
                            case AudioAttenuationModel::Linear:
                                att = 1.0f - rel;
                                break;
                            case AudioAttenuationModel::Logarithmic:
                                att = 1.0f - std::log10(1.0f + 9.0f * rel);
                                break;
                            case AudioAttenuationModel::InverseSquare:
                                att = (minD * minD) / (dist * dist);
                                break;
                            default:
                                break;
                        }
                    }
                    att = std::clamp(att, v.attenuation.minVolume, 1.0f);
                }

                float pan = 0.0f;
                if (dist > 1e-5f) {
                    float dirX = dx / dist;
                    float dirY = dy / dist;
                    float dirZ = dz / dist;
                    pan = dirX * r.x + dirY * r.y + dirZ * r.z;
                }
                pan = std::clamp(pan, -1.0f, 1.0f);

                float leftPan = std::clamp(0.7071f * (1.0f - pan), 0.0f, 1.0f);
                float rightPan = std::clamp(0.7071f * (1.0f + pan), 0.0f, 1.0f);

                leftGain *= att * leftPan;
                rightGain *= att * rightPan;
            }

            sumL += static_cast<int64_t>(std::round(interpolatedSample * leftGain));
            sumR += static_cast<int64_t>(std::round(interpolatedSample * rightGain));

            v.cursorSubframe += v.pitch;
            v.cursor = static_cast<size_t>(v.cursorSubframe);
        }

        sumL = std::clamp<int64_t>(sumL, -32768, 32767);
        sumR = std::clamp<int64_t>(sumR, -32768, 32767);
        out[fIdx * 2] = static_cast<int16_t>(sumL);
        out[fIdx * 2 + 1] = static_cast<int16_t>(sumR);
    }

    m_Voices.erase(
        std::remove_if(m_Voices.begin(), m_Voices.end(), [](const Voice& v) {
            return !v.looping && static_cast<size_t>(v.cursorSubframe) >= v.samples.size();
        }),
        m_Voices.end()
    );
}

} // namespace NeoEngine
