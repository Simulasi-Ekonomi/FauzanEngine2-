# NeoRuntime Farm HUD Composition V1

When `enableFarmRuntimeHud` is enabled, `NeoRuntime::RenderFarm` derives a read-only `FarmRuntimeFrameReceipt` directly from the existing owned Farm system, Farm world, runtime time, optional Farm player-input bridge, and candidate renderer. It supplies canonical seed/produce inventory, player-input receipt, selected action, and availability computed from the current character tile.

The existing `FarmRuntimeHud` remains a presentation component. It receives no mutable world pointer and does not invoke `FarmWorldTool`; it can only display state and route UI selection through the established action-panel-to-bridge path when a caller exposes that path separately. `NeoRuntime` does not construct or tick a `FarmRuntimeSession`, so it does not create a nested simulation/time loop.

World rendering, HUD draw, and optional presentation remain candidate operations. A HUD failure returns `RuntimeError::HudFailed` before committing the candidate renderer or render receipt. The runtime smoke enables player input and HUD, proves a canonical movement followed by a canonical Till action, then proves a nonzero HUD framebuffer hash after `RenderFarm` in both Release and AddressSanitizer with leak detection.

This is a bounded local CPU vertical-slice seam. It does not provide save/recovery integration, a desktop GPU renderer, platform input, audio output, networking, commerce, Android delivery, or a production readiness claim.
