#include "Runtime/WavAudioParser.h"
#include "Runtime/AudioMixer.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace NeoEngine;

    const auto wav = WavAudioParser::GenerateSyntheticWav(48000, 1, 440.0f, 0.01f);
    WavAudioData data{};
    assert(WavAudioParser::Parse(wav, data));
    assert(data.sampleRate == 48000);
    assert(data.channels == 1);
    assert(!data.pcmSamples.empty());

    // Canonical PCM-8 is unsigned on disk; verify it decodes to signed PCM.
    std::vector<uint8_t> pcm8 = {
        'R','I','F','F', 38,0,0,0, 'W','A','V','E',
        'f','m','t',' ', 16,0,0,0, 1,0, 1,0,
        0x40,0x1f, 0,0, 0x40,0x1f, 0,0, 1,0, 8,0,
        'd','a','t','a', 2,0,0,0, 0, 255
    };
    WavAudioData pcm8Data{};
    assert(WavAudioParser::Parse(pcm8, pcm8Data));
    assert(pcm8Data.pcmSamples.size() == 2);
    assert(pcm8Data.pcmSamples[0] == -32768);
    assert(pcm8Data.pcmSamples[1] == 32512);

    AudioMixer mixer;
    assert(mixer.Play(7, {1000, -1000}, 256, true, 2.0f));
    assert(mixer.ActiveVoices() == 1);
    assert(mixer.UpdateVoicePitch(7, 0.5f));
    assert(mixer.UpdateVoiceGain(7, 128));
    const float pos[3] = {2.0f, 0.0f, 0.0f};
    assert(mixer.UpdateVoicePosition(7, pos));
    assert(mixer.Stop(7));
    assert(mixer.ActiveVoices() == 0);
    return 0;
}
