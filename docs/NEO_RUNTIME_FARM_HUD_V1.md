# Neo Runtime Farm HUD V1

## Scope

`RuntimeConfig::enableFarmRuntimeHud` optionally composes `FarmRuntimeHud` after canonical `FarmRenderAdapter::RenderWorld` inside `NeoRuntime::RenderFarm`. The runtime creates a candidate renderer, renders the Farm world, constructs a frame-local telemetry receipt, and draws the HUD into that candidate. The canonical renderer is replaced only after world rendering and HUD drawing both succeed.

| Condition | Result |
|---|---|
| HUD disabled | Existing Farm render path remains unchanged. |
| HUD enabled with at least 64×48 renderer | World render plus read-only frame/coins/tick overlay commits. |
| HUD enabled below 64×48 | Initialization rejects the configuration. |
| World or HUD operation fails | Renderer remains at its previous committed frame and `RenderFarm` fails. |

The HUD receipt uses a runtime-owned render frame counter and `FarmSystem::Snapshot`; it does not tick the Farm, alter user input, or claim any session persistence semantics.

## Evidence

`runtime_farm_hud_smoke` rejects a 63×48 HUD configuration, then compares identical core and overlay runs after one Farm tick. It proves both hashes are nonzero and that the overlay hash differs from the world-only hash. A second HUD render also succeeds before shutdown.

## Boundary

This is a finite software-rendered, read-only HUD overlay. It is not an interactive UI, input authority, host loop, audio device, networking layer, APK, or production runtime readiness claim.
