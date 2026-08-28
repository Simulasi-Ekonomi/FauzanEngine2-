# Broad Non-Vulkan Smoke Evidence V1

**Increment:** P2.2a  
**Repository:** `Simulasi-Ekonomi/FauzanEngine2-`  
**Canonical checkout:** `/home/ubuntu/work/fauzan_engine/remote_main_next3`  
**Toolchain:** GCC 13.3 (`/usr/bin/g++-13`), CMake, Ninja  
**Execution environment:** Linux sandbox, `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`  
**Scope:** bounded representative smoke matrix; this document does **not** claim whole-engine coverage, Unreal parity, production readiness, Android readiness, or Vulkan presentation readiness.

## Executive result

The representative matrix contains **35 executable smoke targets** selected from the canonical `Source/NeoEngine` build. The same target set built and executed successfully in both configurations:

| Configuration | Build mode | Sanitizer | Leak detection | Result |
|---|---|---|---|---|
| Release | `CMAKE_BUILD_TYPE=Release` | None | Not applicable | **35/35 PASS** |
| ASAN | `CMAKE_BUILD_TYPE=Debug` | AddressSanitizer, frame pointers enabled | `ASAN_OPTIONS=detect_leaks=1` | **35/35 PASS** |

The runner completed all targets in deterministic sorted order and emitted `PASS` for every target in both runs. No AddressSanitizer, LeakSanitizer, or runtime sanitizer failure was reported by the ASAN execution.

## Reproduction commands

The runner is `/tmp/run_p22a_representative.sh`. It configures directly from the canonical `Source/NeoEngine` CMake project, builds only the selected target set with Ninja `-j2`, then executes each target with the headless SDL drivers.

```bash
cd /home/ubuntu/work/fauzan_engine/remote_main_next3
bash /tmp/run_p22a_representative.sh \
  /tmp/fauzanengine_p22a_rep_release Release

ASAN_OPTIONS=detect_leaks=1 \
bash /tmp/run_p22a_representative.sh \
  /tmp/fauzanengine_p22a_rep_asan Debug
```

The runner itself records the resolved target set in `/tmp/p22a_Release_targets.txt` and `/tmp/p22a_Debug_targets.txt`. Both files resolved to the same 35 names listed below.

## Exact target coverage

### Runtime, Farm, and presentation-adjacent paths

| Target | Release | ASAN |
|---|---:|---:|
| `runtime_smoke` | PASS | PASS |
| `neo_runtime_farm_vertical_slice_smoke` | PASS | PASS |
| `farm_runtime_session_smoke` | PASS | PASS |
| `farm_interactive_hud_smoke` | PASS | PASS |
| `farm_interactive_surface_demo_smoke` | PASS | PASS |
| `runtime_input_motion_smoke` | PASS | PASS |
| `runtime_persistence_smoke` | PASS | PASS |
| `kinematic_motion_controller_smoke` | PASS | PASS |
| `movement_authority_smoke` | PASS | PASS |
| `grid_route_follower_smoke` | PASS | PASS |

### Renderer, UI, animation, and authoring

| Target | Release | ASAN |
|---|---:|---:|
| `mesh_renderer_smoke` | PASS | PASS |
| `ui_canvas_renderer_smoke` | PASS | PASS |
| `ui_input_router_smoke` | PASS | PASS |
| `animation_timeline_smoke` | PASS | PASS |
| `world_authoring_smoke` | PASS | PASS |

### Physics and navigation

| Target | Release | ASAN |
|---|---:|---:|
| `gameplay_physics_body_smoke` | PASS | PASS |
| `gameplay_physics_query_smoke` | PASS | PASS |
| `grid_navigation_smoke` | PASS | PASS |
| `xpbd_regression` | PASS | PASS |
| `xpbd_determinism` | PASS | PASS |

### Assets and persistence

| Target | Release | ASAN |
|---|---:|---:|
| `asset_resource_manager_smoke` | PASS | PASS |
| `asset_registry_smoke` | PASS | PASS |
| `asset_reload_diagnostics_smoke` | PASS | PASS |
| `atomic_save_file_smoke` | PASS | PASS |

### Authority, networking, safety, and telemetry

| Target | Release | ASAN |
|---|---:|---:|
| `authoritative_command_gate_smoke` | PASS | PASS |
| `authority_loopback_transport_smoke` | PASS | PASS |
| `authority_wire_protocol_smoke` | PASS | PASS |
| `network_session_smoke` | PASS | PASS |
| `network_replication_policy_smoke` | PASS | PASS |
| `network_transport_smoke` | PASS | PASS |
| `trust_safety_smoke` | PASS | PASS |
| `telemetry_outbox_smoke` | PASS | PASS |
| `farm_telemetry_smoke` | PASS | PASS |
| `farm_authority_checkpoint_file_smoke` | PASS | PASS |
| `farm_commerce_checkpoint_file_smoke` | PASS | PASS |

## Selected output assertions

The following assertions were emitted by the passing executables and are retained as a compact behavioral index. They are evidence for the tested paths only, not a claim that untested paths have equivalent coverage.

| Area | Observed assertion |
|---|---|
| Farm interaction | `FARM_INTERACTIVE_HUD_SMOKE_OK` with Till, Plant, Water, Harvest, pointer and keyboard routing; seed count `32→31`, wheat produce `0→2`. |
| Farm surface | `FARM_INTERACTIVE_SURFACE_DEMO_SMOKE_OK` with 19 presented frames, canonical right movement, all four actions, and deterministic world/HUD hashes. |
| Farm runtime | `FARM_RUNTIME_SESSION_SMOKE_OK` with receipt, telemetry, input/world/render integration, and frame preservation. |
| Renderer | `MESH_RENDERER_SMOKE_OK` with triangles, perspective/oriented camera, double-sided geometry, PPM/BMP texture paths, tint/intensity, depth, and two lights. |
| UI | `UI_CANVAS_RENDERER_SMOKE_OK` with widgets, routing, layering, labels, image, atomic rejection, bounds, and stable hash; `UI_INPUT_ROUTER_SMOKE_OK` with hit testing, capture, focus, keyboard, release, and validation. |
| Physics | `GAMEPLAY_PHYSICS_BODY_SMOKE_OK` with dynamic/static bodies, snapshot, velocity, impulse, batch, ECS, query, and invalid-input rejection; `GAMEPLAY_PHYSICS_QUERY_SMOKE_OK` with static/dynamic hits, sorting, batch, atomicity, and no-step-write behavior. |
| XPBD | `XPBD_DETERMINISM_OK` and `XPBD_REGRESSION_OK`; regression reported contacts, bounded payload, and overlap handling. |
| Assets | `ASSET_REGISTRY_SMOKE_OK` with 514 assets, replacement, summary, deterministic behavior, and stable pointer; `ASSET_RELOAD_DIAGNOSTICS_SMOKE_OK` with changed tile and four affected resources without mutation. |
| Authority/network | Gate, wire protocol, loopback transport, session, replication policy, and transport targets all passed their ordering, replay, validation, interest, bounded-payload, and rejection assertions. |
| Persistence/safety | Atomic save, runtime persistence, authority checkpoint, commerce checkpoint, TrustSafety, telemetry outbox, and Farm telemetry targets passed their respective atomicity, fail-closed, replay, privacy, and bounded-output checks. |

## Environment and external-driver exceptions

The CMake configure phase reported that Vulkan was found but `glslc` was missing while checking Vulkan components. This matrix is explicitly **non-Vulkan**; no Vulkan presentation or GPU-driver capability is counted in the 35-target result. Shader-generation steps still completed during configuration/build, and all selected non-Vulkan executables built and ran successfully.

The SDL-dependent targets were run with dummy video and audio drivers. Therefore, this evidence covers headless lifecycle and deterministic software/test paths, not physical display output, GPU swapchain presentation, hardware audio, hot-plug recovery, mobile device behavior, or Android packaging.

No external network service, private dashboard, credential, payment provider, Play Store account, or production deployment was used by this matrix.

## Boundary of the claim

This result closes the bounded P2.2a evidence requirement for the selected representative matrix. It does not close the broader P2.2 suite, because duplicate-heavy and unselected targets were intentionally excluded. It also does not prove 100% game production readiness, 60% Unreal equivalence, 100k-body performance, multiplayer service capacity, anti-cheat completeness, APK/AAB release capability, or live-operations readiness.

The next engineering action should therefore be selected as a separate bounded increment from the remaining backlog, with its own Release and ASAN evidence. The frozen single-segment movement/route authority contract remains unchanged.
