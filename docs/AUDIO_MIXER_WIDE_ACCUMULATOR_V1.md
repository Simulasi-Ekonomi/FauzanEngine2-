# Audio Mixer Wide Accumulator V1

## Scope

`AudioMixer::Mix` now accumulates each stereo frame in a signed 64-bit intermediate before deterministic saturation to signed 16-bit PCM. The existing voice ordering, gain arithmetic, mono-to-stereo duplication, cursor advancement, and voice retirement behavior are unchanged.

| Input mix | Output |
|---|---|
| Summed scaled samples within signed-16 range | Exact PCM value on both stereo channels. |
| Sum above `32767` | Saturated `32767` on both stereo channels. |
| Sum below `-32768` | Saturated `-32768` on both stereo channels. |

The wide intermediate removes dependence on narrower accumulation as voice-count and gain constraints evolve. It does not change device behavior or establish a runtime audio lifecycle.

## Evidence

`audio_mixer_smoke` continues to prove normal two-voice mixing and adds two-voice positive and negative saturation at Q8 gain 512. The target is required to pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`, then in the broad non-Vulkan suite.

## Boundary

This is a software PCM mixing hardening only. It is not runtime audio integration, audio-device delivery, streaming, spatial audio, a persistent host, APK audio integration, or production audio readiness.
