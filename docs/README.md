# Canonical Documentation Map

This directory provides a stable entry point for the **canonical C++ NeoEngine baseline** rooted at `Source/NeoEngine`. It does not claim production, AAA, shipping, Android, editor, agent, or deployment readiness.

| Area | Canonical document | Current boundary |
|---|---|---|
| Task backlog | [`../todo.md`](../todo.md) | Historical completed work and explicitly deferred work. |
| Readiness | [`../production_backend_readiness.md`](../production_backend_readiness.md) | Authoritative **NOT PASSED** readiness statement. |
| Readiness audit | [`READINESS_AUDIT_2026-08-24.md`](READINESS_AUDIT_2026-08-24.md) | Evidence-based active-source, smoke, integration, legacy-boundary, and priority audit. |
| Source consolidation | [`SOURCE_CONSOLIDATION_MANIFEST.md`](SOURCE_CONSOLIDATION_MANIFEST.md) | Preserve/adapt/replace-stub map for canonical runtime, legacy source, editor, backend, agents, and Android adapters. |
| Editor/runtime scene contract | [`SCENE_DOCUMENT_V1.md`](SCENE_DOCUMENT_V1.md) | Bounded authoring envelope and atomic candidate-scene loading boundary; not multiplayer or game-state authority. |
| Local authoring bridge | [`LOCAL_AUTHORING_BRIDGE_V1.md`](LOCAL_AUTHORING_BRIDGE_V1.md) | Approval-gated local handoff from versioned authoring document to C++ candidate scene; no public runtime-control endpoint. |
| Archive completeness | [`GITHUB_ARCHIVE_COMPLETENESS_AUDIT_2026-08-24.md`](GITHUB_ARCHIVE_COMPLETENESS_AUDIT_2026-08-24.md) | Local workspace versus GitHub tree comparison and documented source/artefact decisions. |
| Hermes reconciliation | [`HERMES_ARCHIVE_RECONCILIATION_2026-08-24.md`](HERMES_ARCHIVE_RECONCILIATION_2026-08-24.md) | Reconciliation of the obsolete `1281664` clone scan against current GitHub `main`. |
| Vault and APK readiness | [`VAULT_APK_ENGINE_READINESS_ASSESSMENT_2026-08-24.md`](VAULT_APK_ENGINE_READINESS_ASSESSMENT_2026-08-24.md) | Evidence-based vault exclusion, APK dependency, and engine readiness assessment. |
| Phase 1 codebase audit | [`PHASE_1_ENGINE_CODEBASE_AUDIT.md`](PHASE_1_ENGINE_CODEBASE_AUDIT.md) | Active-source, legacy, placeholder, agent, and capability-gap inventory for the game-builder program. |
| Scene mesh binding | [`SCENE_DOCUMENT_MESH_BINDING_V1.md`](SCENE_DOCUMENT_MESH_BINDING_V1.md) | Bounded authoring scene-to-staged mesh/material runtime binding contract and evidence. |
| SceneDocument v2 | [`SCENE_DOCUMENT_V2.md`](SCENE_DOCUMENT_V2.md) | Versioned mesh/material/texture authoring contract, NAB1 compatibility, NAB2 bridge, and current evidence. |
| SceneDocument v3 sprite binding | [`SCENE_DOCUMENT_SPRITE_BINDING_V1.md`](SCENE_DOCUMENT_SPRITE_BINDING_V1.md) | Bounded sprite authoring, NAB3 compatibility, staged CPU texture, and software-rendered 2D quad contract. |
| Farm sprite rendering | [`FARM_SPRITE_RENDERING_V1.md`](FARM_SPRITE_RENDERING_V1.md) | Read-only FarmSystem/FarmWorldTool textured presentation with candidate staging and frame replacement boundary. |
| Farm player input | [`FARM_PLAYER_INPUT_BRIDGE_V1.md`](FARM_PLAYER_INPUT_BRIDGE_V1.md) | Bounded local input-to-move/crop bridge that routes through existing FarmWorld authority APIs. |
| Farm runtime session | [`FARM_RUNTIME_SESSION_V1.md`](FARM_RUNTIME_SESSION_V1.md) | Explicit input → FarmWorld tick → staged sprite frame lifecycle with scoped failure behavior. |
| Software surface presenter | [`SOFTWARE_SURFACE_PRESENTER_V1.md`](SOFTWARE_SURFACE_PRESENTER_V1.md) | Optional SDL upload/present seam from canonical SoftwareRenderer and NeoRuntime Farm. |
| Gameplay authoring visual binding | [`AUTHORING_CATALOG_VISUAL_BINDING_V1.md`](AUTHORING_CATALOG_VISUAL_BINDING_V1.md) | Building and actor placement binding to canonical staged mesh/material/texture runtime entities. |
| Engine audit | [`../engine_capability_audit_2026-08-23.md`](../engine_capability_audit_2026-08-23.md) | Capability audit and known gaps. |
| CPU skinning | [`../skinning_cpu_evaluation.md`](../skinning_cpu_evaluation.md) | Limited CPU skeleton, palette, clip, and skinning evidence. |
| Route ownership | [`../route_intent_single_writer_contract.md`](../route_intent_single_writer_contract.md) | Intent/receipt and one-transform-writer contract. |
| Skeletal-route lifecycle | [`../route_root_motion_runtime_lifecycle_contract.md`](../route_root_motion_runtime_lifecycle_contract.md) | Opt-in, one straight clamp segment runtime boundary. |
| Cardinal metadata | [`../skeletal_locomotion_metadata_contract.md`](../skeletal_locomotion_metadata_contract.md) | Isolated metadata/registry preflight; no multi-segment runtime. |

## Build and smoke evidence

The canonical C++ CMake project is `Source/NeoEngine`. Use a generated build directory outside the source tree when possible. The maintained local configurations are shown below; CTest is not the evidence runner for this repository.

```bash
# Release
ninja -C /home/ubuntu/work/fauzan_engine/build/neoengine -j2

# AddressSanitizer debug configuration
ninja -C /home/ubuntu/work/fauzan_engine/build/neoengine_asan -j2
```

Run smoke executables directly. The broad suite is every root `*_smoke` executable except `vulkan_*` and `sdl_audio_bridge_smoke`; the latter is excluded because ALSA is unavailable in the sandbox.

```bash
set -e
build=/home/ubuntu/work/fauzan_engine/build/neoengine
find "$build" -maxdepth 1 -type f -executable -printf '%f\n' | grep '_smoke$' | sort | while read -r smoke; do
  case "$smoke" in vulkan_*|sdl_audio_bridge_smoke) continue ;; esac
  "$build/$smoke"
done

set -e
build=/home/ubuntu/work/fauzan_engine/build/neoengine_asan
find "$build" -maxdepth 1 -type f -executable -printf '%f\n' | grep '_smoke$' | sort | while read -r smoke; do
  case "$smoke" in vulkan_*|sdl_audio_bridge_smoke) continue ;; esac
  ASAN_OPTIONS=detect_leaks=1 "$build/$smoke"
done
```

Before storing work, run a **scoped** whitespace check for canonical source, tests, CMake, contracts, docs, and TODO. The repository contains unrelated legacy/untracked material, so a global `git diff --check` is not a reliable gate.

```bash
cd /home/ubuntu/work/fauzan_engine/src/FauzanEngine
git diff --check -- Source/NeoEngine Tests todo.md docs \
  production_backend_readiness.md engine_capability_audit_2026-08-23.md \
  skinning_cpu_evaluation.md route_intent_single_writer_contract.md \
  route_root_motion_runtime_lifecycle_contract.md skeletal_locomotion_metadata_contract.md
```
