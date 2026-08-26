# Neo Runtime Farm Render Receipt V1

## Scope

`NeoRuntime` now exposes `LastFarmRenderReceipt()` after a successful `RenderFarm` call. The immutable receipt contains the monotonic runtime render frame, the Farm-world framebuffer hash before HUD composition, the final HUD hash when the optional overlay is enabled, and a copied `FarmTelemetrySnapshot`.

| Render mode | `worldFramebufferHash` | `hudFramebufferHash` |
|---|---|---|
| HUD disabled | Nonzero canonical Farm world hash. | `0`. |
| HUD enabled | Nonzero pre-overlay Farm world hash. | Nonzero final overlay hash. |

The renderer and receipt are committed together only after world rendering and any requested HUD drawing succeed. A failed render leaves the previously committed renderer and receipt available without mutation.

## Evidence

`runtime_farm_hud_smoke` proves the HUD-disabled receipt has frame one, world hash parity, HUD hash zero, and copied simulation tick one. It also proves the HUD-enabled receipt records distinct nonzero world/HUD hashes and advances to frame two after a second successful render.

## Boundary

This is in-process render observability only. It does not transport telemetry, accept UI input, write Farm state, synchronize networking, persist to cloud/files, manage a host loop, generate an APK, or establish production runtime readiness.
