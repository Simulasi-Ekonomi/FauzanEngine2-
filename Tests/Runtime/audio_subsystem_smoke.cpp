#include "Runtime/AudioMixer.h"
#include "Runtime/AudioComponent.h"
#include "Runtime/WavAudioParser.h"
#include "Runtime/SdlAudioBridge.h"

#include <cstdio>
#include <cmath>
#include <vector>

int main() {
    using namespace NeoEngine;

    auto syntheticWav = WavAudioParser::GenerateSyntheticWav(44100, 1, 440.0f, 0.1f);
    if (syntheticWav.empty()) {
        std::printf("ERR: Synthetic WAV generation failed\n");
        return 1;
    }

    WavAudioData wavData;
    if (!WavAudioParser::Parse(syntheticWav, wavData) || wavData.pcmSamples.empty() || wavData.sampleRate != 44100) {
        std::printf("ERR: WAV parsing failed\n");
        return 1;
    }

    AudioMixer mixer;
    AudioListener listener;
    listener.position = {0.0f, 0.0f, 0.0f};
    listener.forward = {0.0f, 0.0f, 1.0f};
    listener.up = {0.0f, 1.0f, 0.0f};
    mixer.SetListener(listener);

    SpatialVoiceParams leftParams;
    leftParams.id = 101;
    leftParams.mono = wavData.pcmSamples;
    leftParams.spatialized = true;
    leftParams.position = {-10.0f, 0.0f, 5.0f};
    leftParams.attenuation.minDistance = 1.0f;
    leftParams.attenuation.maxDistance = 50.0f;
    if (!mixer.PlaySpatial(leftParams)) {
        std::printf("ERR: PlaySpatial left failed\n");
        return 1;
    }

    std::vector<int16_t> leftStereo;
    mixer.Mix(10, leftStereo);
    if (leftStereo.size() != 20) {
        std::printf("ERR: Left mix size invalid\n");
        return 1;
    }

    int64_t sumLeftL = 0, sumLeftR = 0;
    for (size_t i = 0; i < 10; ++i) {
        sumLeftL += std::abs(leftStereo[i * 2]);
        sumLeftR += std::abs(leftStereo[i * 2 + 1]);
    }
    if (sumLeftL <= sumLeftR) {
        std::printf("ERR: Left spatial panning assertion failed: Left=%lld, Right=%lld\n", static_cast<long long>(sumLeftL), static_cast<long long>(sumLeftR));
        return 1;
    }

    mixer.Clear();

    SpatialVoiceParams rightParams;
    rightParams.id = 102;
    rightParams.mono = wavData.pcmSamples;
    rightParams.spatialized = true;
    rightParams.position = {10.0f, 0.0f, 5.0f};
    rightParams.attenuation.minDistance = 1.0f;
    rightParams.attenuation.maxDistance = 50.0f;
    if (!mixer.PlaySpatial(rightParams)) {
        std::printf("ERR: PlaySpatial right failed\n");
        return 1;
    }

    std::vector<int16_t> rightStereo;
    mixer.Mix(10, rightStereo);
    int64_t sumRightL = 0, sumRightR = 0;
    for (size_t i = 0; i < 10; ++i) {
        sumRightL += std::abs(rightStereo[i * 2]);
        sumRightR += std::abs(rightStereo[i * 2 + 1]);
    }
    if (sumRightR <= sumRightL) {
        std::printf("ERR: Right spatial panning assertion failed: Left=%lld, Right=%lld\n", static_cast<long long>(sumRightL), static_cast<long long>(sumRightR));
        return 1;
    }

    mixer.Clear();

    AudioComponent audioComp;
    audioComp.SetSound(201, wavData.pcmSamples);
    audioComp.SetPosition({0.0f, 0.0f, 2.0f});
    audioComp.SetPitch(1.2f);
    audioComp.SetLooping(true);

    if (!audioComp.Play(mixer) || !audioComp.IsPlaying()) {
        std::printf("ERR: AudioComponent Play failed\n");
        return 1;
    }

    audioComp.Update(mixer, {5.0f, 0.0f, 2.0f});
    std::vector<int16_t> compStereo;
    mixer.Mix(50, compStereo);

    if (compStereo.size() != 100 || !audioComp.Stop(mixer) || audioComp.IsPlaying()) {
        std::printf("ERR: AudioComponent update/stop failed\n");
        return 1;
    }

    std::printf("AUDIO_SUBSYSTEM_SMOKE_OK wav_samples=%zu active_voices=%zu\n", wavData.pcmSamples.size(), mixer.ActiveVoices());
    return 0;
}
