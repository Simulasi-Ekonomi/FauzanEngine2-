# Neo Runtime Farm Render Receipt V1

## Scope

`NeoRuntime` now exposes `LastFarmRenderReceipt()` after a successful `RenderFarm` call. The immutable receipt contains the monotonic runtime render frame, the Farm-world framebuffer hash before HUD composition, the final HUD hash when the optional overlay is enabled, the optional presenter frame count, and a copied `FarmTelemetrySnapshot`.

| Render mode | `worldFramebufferHash` | `hudFramebufferHash` | `presentedFrameCount` |
|---|---|---|
| HUD disabled, no surface | Nonzero canonical Farm world hash. | `0`. | `0`. |
| HUD enabled, no surface | Nonzero pre-overlay Farm world hash. | Nonzero final overlay hash. | `0`. |
| Hidden software surface | Nonzero final candidate hash. | Depends on HUD mode. | Increments only after `Present(candidate)` succeeds. |

The renderer and receipt are committed together only after world rendering, any requested HUD drawing, and optional surface event/present calls succeed. A failed render or presentation leaves the previously committed renderer and receipt available without mutation.

## Evidence

`runtime_farm_hud_smoke` proves the HUD-disabled receipt has frame one, world hash parity, HUD hash zero, no presenter count, and copied simulation tick one. It also proves the HUD-enabled receipt records distinct nonzero world/HUD hashes and advances to frame two; finally, it proves a hidden SDL surface presents one candidate frame before the receipt and renderer commit.

## Boundary

This is in-process render observability only. It does not transport telemetry, accept UI input, write Farm state, synchronize networking, persist to cloud/files, manage a host loop, generate an APK, or establish production runtime readiness.
