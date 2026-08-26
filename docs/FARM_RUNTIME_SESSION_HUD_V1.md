# Farm Runtime Session HUD V1

## Scope

`FarmRuntimeSession::DrawHud` is an explicit caller-driven bridge from the last **committed** Farm frame receipt to the existing candidate-rendered `FarmRuntimeHud`. On success it returns a separate `FarmRuntimeHudReceipt` containing the unchanged world receipt hash and the resulting HUD-overlaid framebuffer hash.

| Property | Contract |
|---|---|
| Simulation and input | The method does not tick `FarmWorldTool`, consume input, or mutate Farm state. |
| Frame ownership | It does not increment the session frame count or replace `LastFrameReceipt`. |
| Renderer | `FarmRuntimeHud` uses a candidate framebuffer; HUD rejection preserves the caller framebuffer. |
| Caller output | The output HUD receipt is committed only after a successful HUD draw. |

The method rejects an uninitialized session or one with no committed world frame.

## Evidence

`farm_runtime_session_hud_smoke` proves that an unframed session rejects the request while preserving a sentinel output, blank renderer hash, and zero frame count. It then commits one canonical runtime frame, draws the HUD, and proves a distinct HUD framebuffer hash, receipt telemetry parity, and unchanged session frame/world receipt.

The smoke must pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`, followed by the broad non-Vulkan suite.

## Boundary

This is a finite software HUD overlay seam. It is **not** an interactive UI, input authority layer, persistent host UI, networking screen, APK UI, or production runtime claim.
