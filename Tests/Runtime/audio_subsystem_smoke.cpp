#include "Runtime/AudioMixer.h"
#include "Runtime/AudioComponent.h"
#include "Runtime/WavAudioParser.h"
#include "Runtime/SdlAudioBridge.h"

#include <cstdio>
#include <cmath>
#include <vector>

int main() {
    using namespace NeoEngine;

    auto wavMono16 = WavAudioParser::GenerateSyntheticWav(44100, 1, 440.0f, 0.1f);
    WavAudioData dataMono16;
    if (!WavAudioParser::Parse(wavMono16, dataMono16) || dataMono16.pcmSamples.empty() || dataMono16.sampleRate != 44100) {
        std::printf("ERR: Mono 16-bit WAV parsing failed\n");
        return 1;
    }

    auto wavStereo16 = WavAudioParser::GenerateSyntheticWav(48000, 2, 880.0f, 0.05f);
    WavAudioData dataStereo16;
    if (!WavAudioParser::Parse(wavStereo16, dataStereo16) || dataStereo16.pcmSamples.empty() || dataStereo16.sampleRate != 48000) {
        std::printf("ERR: Stereo 16-bit WAV parsing failed\n");
        return 1;
    }

    WavAudioData dummy;
    std::vector<uint8_t> truncatedHeader = { 'R', 'I', 'F', 'F', 0, 0 };
    if (WavAudioParser::Parse(truncatedHeader, dummy)) {
        std::printf("ERR: Truncated header should be rejected\n");
        return 1;
    }

    std::vector<uint8_t> invalidMagic = wavMono16;
    invalidMagic[0] = 'X';
    if (WavAudioParser::Parse(invalidMagic, dummy)) {
        std::printf("ERR: Invalid magic RIFF should be rejected\n");
        return 1;
    }

    AudioMixer mixer;
    AudioListener listener;
    listener.position = {0.0f, 0.0f, 0.0f};
    listener.forward = {0.0f, 0.0f, 1.0f};
    listener.up = {0.0f, 1.0f, 0.0f};
    mixer.SetListener(listener);

    if (mixer.Play(0, dataMono16.pcmSamples)) {
        std::printf("ERR: Voice ID 0 should be rejected\n");
        return 1;
    }

    if (!mixer.Play(10, dataMono16.pcmSamples) || mixer.Play(10, dataMono16.pcmSamples)) {
        std::printf("ERR: Duplicate voice ID should be rejected\n");
        return 1;
    }
    mixer.Stop(10);

    SpatialVoiceParams linearParams;
    linearParams.id = 1;
    linearParams.mono = dataMono16.pcmSamples;
    linearParams.spatialized = true;
    linearParams.position = {0.0f, 0.0f, 10.0f};
    linearParams.attenuation.model = AudioAttenuationModel::Linear;
    linearParams.attenuation.minDistance = 2.0f;
    linearParams.attenuation.maxDistance = 20.0f;
    linearParams.attenuation.minVolume = 0.0f;
    if (!mixer.PlaySpatial(linearParams)) {
        std::printf("ERR: PlaySpatial Linear failed\n");
        return 1;
    }

    SpatialVoiceParams invParams = linearParams;
    invParams.id = 2;
    invParams.attenuation.model = AudioAttenuationModel::InverseSquare;
    if (!mixer.PlaySpatial(invParams)) {
        std::printf("ERR: PlaySpatial InverseSquare failed\n");
        return 1;
    }

    std::vector<int16_t> mixed;
    mixer.Mix(100, mixed);
    if (mixed.size() != 200) {
        std::printf("ERR: Mixed size invalid\n");
        return 1;
    }

    AudioComponent component;
    component.SetSound(50, dataMono16.pcmSamples);
    component.SetPosition({-5.0f, 0.0f, 2.0f});
    component.SetPitch(1.5f);
    if (!component.Play(mixer) || !component.IsPlaying()) {
        std::printf("ERR: Component play failed\n");
        return 1;
    }

    component.Update(mixer, {5.0f, 0.0f, 2.0f});
    mixer.Mix(50, mixed);

    if (!component.Stop(mixer) || component.IsPlaying()) {
        std::printf("ERR: Component stop failed\n");
        return 1;
    }

    std::printf("AUDIO_SUBSYSTEM_SMOKE_OK mono_samples=%zu stereo_samples=%zu active_voices=%zu\n",
                dataMono16.pcmSamples.size(), dataStereo16.pcmSamples.size(), mixer.ActiveVoices());
    return 0;
}
