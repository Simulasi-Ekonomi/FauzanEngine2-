#include "Runtime/AudioMixer.h"
#include <cstdio>
#include <limits>

using namespace NeoEngine;

int main() {
    AudioMixer m;
    std::vector<int16_t> out;
    bool ok = m.Play(1, {1000, 1000}) && m.Play(2, {2000, 2000}) &&
              !m.Play(1, {1}) && m.ActiveVoices() == 2;
    m.Mix(2, out);
    ok = ok && out.size() == 4 && out[0] == 3000 && out[1] == 3000 &&
         m.ActiveVoices() == 0 && !m.Stop(1);

    AudioMixer positive, negative;
    ok = ok && positive.Play(1, {32767}, 512) && positive.Play(2, {32767}, 512) &&
         negative.Play(1, {-32768}, 512) && negative.Play(2, {-32768}, 512);
    positive.Mix(1, out);
    ok = ok && out.size() == 2 && out[0] == 32767 && out[1] == 32767;
    negative.Mix(1, out);
    ok = ok && out.size() == 2 && out[0] == -32768 && out[1] == -32768;

    AudioMixer looping;
    ok = ok && looping.Play(7, {1000, 2000}, 256, true);
    looping.Mix(4, out);
    ok = ok && looping.ActiveVoices() == 1 && out.size() == 8 &&
         out[0] == 1000 && out[2] == 2000 && out[4] == 1000 && out[6] == 2000;

    AudioMixer spatial;
    AudioListener listener{};
    listener.forward[0] = 0.0f; listener.forward[1] = 0.0f; listener.forward[2] = 1.0f;
    listener.up[0] = 0.0f; listener.up[1] = 1.0f; listener.up[2] = 0.0f;
    spatial.SetListener(listener);
    SpatialVoiceParams source;
    source.id = 11;
    source.mono = {10000};
    source.spatialized = true;
    source.position[0] = 10.0f;
    source.position[1] = 0.0f;
    source.position[2] = 1.0f;
    source.attenuation.minDistance = 0.1f;
    source.attenuation.maxDistance = 1000.0f;
    ok = ok && spatial.PlaySpatial(source);
    spatial.Mix(1, out);
    ok = ok && out[1] > out[0];

    AudioMixer invalidAttenuation;
    SpatialVoiceParams invalid = source;
    invalid.id = 13;

    invalid.attenuation.minDistance = std::numeric_limits<float>::quiet_NaN();
    ok = ok && !invalidAttenuation.PlaySpatial(invalid);

    invalid.attenuation.minDistance = 0.1f;
    invalid.attenuation.maxDistance = std::numeric_limits<float>::quiet_NaN();
    ok = ok && !invalidAttenuation.PlaySpatial(invalid);

    invalid.attenuation.maxDistance = 1000.0f;
    invalid.attenuation.minVolume = std::numeric_limits<float>::quiet_NaN();
    ok = ok && !invalidAttenuation.PlaySpatial(invalid);

    AudioMixer rotated;
    listener.forward[2] = -1.0f;
    rotated.SetListener(listener);
    source.id = 12;
    ok = ok && rotated.PlaySpatial(source);
    rotated.Mix(1, out);
    ok = ok && out[0] > out[1];

    if (!ok) return 1;
    std::printf("AUDIO_MIXER_SMOKE_OK frames=%zu saturation=1 looping=1 listener_pan=1\n", out.size() / 2);
    return 0;
}
