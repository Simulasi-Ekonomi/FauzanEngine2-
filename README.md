# FauzanEngine — Canonical NeoEngine Baseline

This repository currently documents and tests a **limited C++ engine/backend foundation** for lightweight-to-medium game foundations, with Farm/economy simulation as a principal future use case. The canonical C++ source is `Source/NeoEngine`; canonical executable evidence is in `Tests`.

> **Status: NOT production-ready and NOT AAA-ready.** The project intentionally makes no claim of shipped-game, renderer, editor, Android, agent autonomy, multiplayer, anti-cheat, payments, or deployment readiness.

## Verified baseline

The current scope includes fail-closed CPU-side skeleton hierarchy/bind/inverse-bind/palette evaluation, position-and-normal CPU skinning, typed TRS clips and player ownership, skeletal root-motion application, route intent/receipt, a single transform-writer adapter, movement authority gating, and a deliberately constrained opt-in NeoRuntime skeletal route.

| Area | Boundary |
|---|---|
| Skeletal route | One straight, two-cell, clamp-only segment; default off and exclusive with static kinematic route mode. |
| Transform writes | Guarded skeletal root motion is the sole writer on the skeletal-route path. |
| Metadata | Cardinal one-cell clip validation and a snapshot registry of at most four clips; no multi-segment transition runtime. |
| Verification | Direct smoke executables in Release and AddressSanitizer; broad non-Vulkan suite excludes `vulkan_*` and `sdl_audio_bridge_smoke`. |
| Readiness | [`production_backend_readiness.md`](production_backend_readiness.md) remains **NOT PASSED**. |

## Repository guide

| Path | Purpose |
|---|---|
| `Source/NeoEngine/` | Canonical C++23/CMake engine source. |
| `Tests/` | Canonical executable smoke evidence. |
| `Source/NeoEngine/CMakeLists.txt` | Canonical target and smoke registration. |
| `docs/README.md` | Documentation map plus reproducible build/test commands. |
| `GITHUB_PREPARATION.md` | Storage checklist and staging boundary. |
| `todo.md` | Historical work log and deferred work. |
| `*_contract.md` | Explicit contracts that prevent unsafe feature expansion. |

## Build and test

The maintained local build directories are `/home/ubuntu/work/fauzan_engine/build/neoengine` for Release and `/home/ubuntu/work/fauzan_engine/build/neoengine_asan` for AddressSanitizer. Build and direct-smoke commands are maintained in [`docs/README.md`](docs/README.md).

## Explicitly deferred

No feature expansion is authorized in this stabilization checkpoint. Multi-segment routing, root rotation, kinematic fallback/delta blending, NPC locomotion, steering, collision/physics coupling, renderer binding, GPU skinning, prediction, multiplayer, Android packaging, deployment, and production readiness remain deferred.
