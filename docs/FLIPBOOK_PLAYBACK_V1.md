# Flipbook Playback V1

`FlipbookPlayback` is a bounded caller-driven clock. It accepts a positive finite duration at most 60 seconds and advances only a local elapsed time by a finite delta in `[0, 60]`. In loop mode it wraps with `fmod`; otherwise it clamps at the terminal sample. A caller may pause/resume it; advancing while paused validates the delta but retains elapsed time and sample. It emits only a normalized scalar sample for `FlipbookFrameSelector`.

It owns neither `SceneWorld` nor a sprite, route intent, movement authority, transform, or runtime tick registration. Invalid configuration or delta preserves the last valid clock state and caller sample.

`flipbook_playback_smoke` proves quarter-second selection of atlas frame one, paused time/sample and selector stability, loop wrapping at one second to frame zero, invalid-NaN atomic preservation, and non-loop terminal clamp. It passes in Release and AddressSanitizer with `detect_leaks=1`; the current non-Vulkan broad suites pass 127/127 in both configurations. This is not a skeletal clock, blend/state system, SceneWorld writer, GPU path, or production animation claim.
