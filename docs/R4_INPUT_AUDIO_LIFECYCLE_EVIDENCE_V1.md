# R4 Input and Audio Lifecycle Evidence V1

## Scope

Canonical desktop primitives now have a combined Release/ASAN workflow. `InputState` and `SdlInputBridge` cover bounded input snapshots and SDL translation; `UiInputRouter` covers deterministic hit testing, pointer capture, and keyboard focus traversal; `AudioMixer` and `SdlAudioBridge` cover bounded PCM playback, callback mixing, reset, and reinitialize lifecycle.

## Evidence

The following smokes are run with `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`, and `ASAN_OPTIONS=detect_leaks=1` in the R4 workflow:

```text
SDL_AUDIO_BRIDGE_SMOKE_OK recovery=1 frames_before_reset=<nonzero>
SDL_INPUT_BRIDGE_SMOKE_OK ...
INPUT_STATE_SMOKE_OK ...
UI_INPUT_ROUTER_SMOKE_OK ...
```

The exact counters are runtime-dependent; the pass contract is the target's `*_SMOKE_OK` result. Audio smoke proves initialization rejection, callback frame production, queued voices, reset, play-after-reset rejection, and reinitialization. Input/UI smokes prove bounded translation and deterministic routing/focus behavior.

## Boundary and status

This is desktop/headless lifecycle evidence only. It does not prove physical audio output, Android audio focus, touch/controller device coverage, localization/text scaling, IME, accessibility semantics/screen reader support, or full offline/error UX. Therefore **R4 remains Not passed**.
