# Animation Playback Control Binding V1

`AnimationPlaybackControlBinding` accepts exactly two bounded, nonempty state identifiers: one pause state and one resume state. A caller supplies the active state identifier, typically the result of a canonical state-machine decision, and a `FlipbookPlayback`. The binding only invokes `SetPaused`; it does not own a timeline, state machine, SceneWorld, sprite, route, movement authority, transform, or runtime tick.

Configuration and unmapped-state failures retain the pre-existing playback pause state. The smoke proves `idle` pauses a running playback, the elapsed sample is stable while paused, an unmapped state preserves pause, and `move` resumes time advancement to sample `0.50`. It passes in Release and AddressSanitizer with `detect_leaks=1`; the current non-Vulkan broad suites pass 128/128 in both configurations. This is a bounded control seam, not skeletal animation, root motion, NPC behavior, GPU animation, or production readiness.
