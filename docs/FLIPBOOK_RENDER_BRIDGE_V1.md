# Flipbook Render Bridge V1

`FlipbookRenderBridge::AdvanceQueue` combines a caller-supplied `FlipbookPlayback`, an initialized `FlipbookFrameSelector`, a read-only `SceneWorld`, a staged `SceneSpriteAdapter`, and one `SpriteBatch`. It first copies playback, batch, and output rectangle; it advances/selects/queues only in those candidates; then commits all three candidates only when every step succeeds.

This bridge has no transform, route, or movement authority. `SceneSpriteAdapter::QueueFrame` still reads transforms only. `flipbook_render_bridge_smoke` proves a half-second frame selection/queue, invalid-NaN playback rollback, missing-world-transform queue rollback, unchanged batch/time/rect on failure, and unchanged actor transform. It passes in Release and AddressSanitizer with `detect_leaks=1`; the current non-Vulkan broad suites pass 127/127 in both configurations. This is not a skeletal, GPU, or production animation path.
