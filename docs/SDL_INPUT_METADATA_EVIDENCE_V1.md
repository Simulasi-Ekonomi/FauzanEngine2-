# SDL Input Metadata Evidence V1

**Increment:** P1.6b  
**Scope:** bounded local input normalization for touch/controller metadata and lifecycle fail-closed behavior. This evidence does **not** claim mobile-device certification, accessibility semantics, remapping persistence, physical controller coverage, or production platform readiness.

## Result

The new `sdl_input_metadata_smoke` target and the existing input regression targets passed in both configurations under the SDL dummy drivers.

| Configuration | Build | Sanitizer | Result |
|---|---|---|---|
| Release | `CMAKE_BUILD_TYPE=Release` | None | PASS |
| ASAN | `CMAKE_BUILD_TYPE=Debug` | AddressSanitizer with `detect_leaks=1` | PASS |

The focused smoke outputs were:

```text
SDL_INPUT_METADATA_SMOKE_OK touch=1 motion=1 axis=1 device_reset=1 focus_release=1 invalid_atomic=1 quit=1 lifecycle=1
SDL_INPUT_BRIDGE_SMOKE_OK keyboard=1 mouse=1 touch=1 gamepad=1 quit=1
INPUT_STATE_SMOKE_OK keyboard=1 touch=1 gamepad=1 rebind=1 summary=1 deterministic=1
```

The targeted regressions also passed in Release and ASAN:

```text
RUNTIME_INPUT_MOTION_SMOKE_OK fixedTick=1 diagonal=1 heading=1 pause=1 x=0.707107 z=0.707107
FARM_RUNTIME_SESSION_SMOKE_OK frames=3 receipt=1 telemetry=1 inputWorldRender=1 framePreservation=1 hash=13107158792956737171
FARM_INTERACTIVE_HUD_SMOKE_OK frame=18 hud=1 pointer=1 keyboard=1 actions=till,plant,water,harvest canonicalInteract=1 wheatSeeds=31 wheatProduce=2
```

## Verified behavior

The smoke covers normalized touch coordinates for finger down, motion, and release; controller left-axis normalization from SDL signed 16-bit values into the bounded `[-1, 1]` interval; controller connected/disconnected transitions; focus-loss release of held actions; explicit quit propagation; atomic rejection of non-finite/out-of-range metadata; and `PumpFrame` rejection after bridge reset with `NotInitialized`.

The existing keyboard, mouse, touch, and gamepad action bindings remain green. Focus-loss behavior is fail-closed: held actions are released and expose `justReleased` rather than remaining stuck. Metadata is cleared at the beginning of each SDL frame, while persistent controller connection/axis state is retained until a new device event changes it.

## Changed paths

| Path | Change |
|---|---|
| `Source/NeoEngine/Runtime/InputState.h` | Added bounded frame metadata structures and validated metadata lifecycle methods while preserving the existing action API. |
| `Source/NeoEngine/Runtime/InputState.cpp` | Implemented atomic coordinate/axis validation, pending release behavior, metadata reset, and controller lifecycle updates. |
| `Source/NeoEngine/Runtime/SdlInputBridge.h` | Added an explicit metadata-rejection error. |
| `Source/NeoEngine/Runtime/SdlInputBridge.cpp` | Normalized touch/controller events and propagated focus-loss/quit signals. |
| `Tests/Runtime/sdl_input_metadata_smoke.cpp` | Added the focused Release/ASAN smoke coverage. |
| `Source/NeoEngine/CMakeLists.txt` | Registered `sdl_input_metadata_smoke`. |

## Boundary

This is a local SDL event bridge proof only. It does not prove event delivery on a physical Android/iOS device, controller hot-plug behavior across every backend, touch gesture semantics, localization/text scaling, accessibility tree semantics, audio device recovery, persistence of input mappings, or release packaging.
