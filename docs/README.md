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
| Fase A renderer baseline | [`PHASE_A_RENDERER_BASELINE_2026-08-25.md`](PHASE_A_RENDERER_BASELINE_2026-08-25.md) | Consolidated renderer evidence, active seams, unfulfilled gates, and non-readiness boundary. |
| Scene mesh binding | [`SCENE_DOCUMENT_MESH_BINDING_V1.md`](SCENE_DOCUMENT_MESH_BINDING_V1.md) | Bounded authoring scene-to-staged mesh/material runtime binding contract and evidence. |
| SceneDocument v2 | [`SCENE_DOCUMENT_V2.md`](SCENE_DOCUMENT_V2.md) | Versioned mesh/material/texture authoring contract, NAB1 compatibility, NAB2 bridge, and current evidence. |
| SceneDocument v3 sprite binding | [`SCENE_DOCUMENT_SPRITE_BINDING_V1.md`](SCENE_DOCUMENT_SPRITE_BINDING_V1.md) | Bounded sprite authoring, NAB3 compatibility, staged CPU texture, and software-rendered 2D quad contract. |
| Farm sprite rendering | [`FARM_SPRITE_RENDERING_V1.md`](FARM_SPRITE_RENDERING_V1.md) | Read-only FarmSystem/FarmWorldTool textured presentation with candidate staging and frame replacement boundary. |
| Farm player input | [`FARM_PLAYER_INPUT_BRIDGE_V1.md`](FARM_PLAYER_INPUT_BRIDGE_V1.md) | Bounded local input-to-move/crop bridge that routes through existing FarmWorld authority APIs. |
| Farm runtime session | [`FARM_RUNTIME_SESSION_V1.md`](FARM_RUNTIME_SESSION_V1.md) | Explicit input → FarmWorld tick → staged sprite frame lifecycle with scoped failure behavior. |
| Software surface presenter | [`SOFTWARE_SURFACE_PRESENTER_V1.md`](SOFTWARE_SURFACE_PRESENTER_V1.md) | Optional SDL upload/present seam from canonical SoftwareRenderer and NeoRuntime Farm. |
| Software surface lifecycle | [`SOFTWARE_SURFACE_LIFECYCLE_V1.md`](SOFTWARE_SURFACE_LIFECYCLE_V1.md) | Bounded SDL event pump, close request, and fail-closed NeoRuntime presentation behavior. |
| Surface resize observation | [`SOFTWARE_SURFACE_RESIZE_OBSERVATION_V1.md`](SOFTWARE_SURFACE_RESIZE_OBSERVATION_V1.md) | Read-only SDL resize-event observation with unchanged renderer/texture dimensions. |
| Farm surface demo | [`FARM_SURFACE_DEMO_V1.md`](FARM_SURFACE_DEMO_V1.md) | Finite native Farm graphical vertical slice with SDL presentation and PPM artifact output. |
| Mesh surface demo | [`MESH_SURFACE_DEMO_V1.md`](MESH_SURFACE_DEMO_V1.md) | Finite static 3D mesh proof through perspective camera, texture/material/light, SDL presentation, and PPM artifact output. |
| Hybrid surface demo | [`HYBRID_SURFACE_DEMO_V1.md`](HYBRID_SURFACE_DEMO_V1.md) | Finite mesh-texture plus camera-billboard sprite proof through SDL presentation and PPM output. |
| Render camera orientation | [`RENDER_CAMERA_ORIENTATION_V1.md`](RENDER_CAMERA_ORIENTATION_V1.md) | Validated orientation basis shared by orthographic sprite/Farm and perspective mesh projection. |
| Camera sphere frustum | [`RENDER_CAMERA_SPHERE_FRUSTUM_V1.md`](RENDER_CAMERA_SPHERE_FRUSTUM_V1.md) | Reusable conservative oriented camera-space sphere-frustum query. |
| Sprite alpha and tint | [`SPRITE_ALPHA_TINT_V1.md`](SPRITE_ALPHA_TINT_V1.md) | Deterministic layer/order-preserving alpha composition and textured RGBA tint for SpriteBatch. |
| Sprite batch atomic flush | [`SPRITE_BATCH_ATOMIC_FLUSH_V1.md`](SPRITE_BATCH_ATOMIC_FLUSH_V1.md) | Candidate-frame flush that preserves the caller framebuffer on sprite projection/raster rejection. |
| Sprite frustum clipping | [`SPRITE_FRUSTUM_CLIPPING_V1.md`](SPRITE_FRUSTUM_CLIPPING_V1.md) | Bounded six-plane quad clipping, normal off-frustum culling, and preserved atomic SpriteBatch behavior. |
| Sprite rotation | [`SPRITE_ROTATION_V1.md`](SPRITE_ROTATION_V1.md) | Finite center-pivot quad rotation before clipping, with preserved tint/alpha/ordering contracts. |
| Sprite camera billboard | [`SPRITE_CAMERA_BILLBOARD_V1.md`](SPRITE_CAMERA_BILLBOARD_V1.md) | Opt-in camera-basis sprite billboard retaining local rotation and renderer contracts. |
| Sprite alpha depth-write | [`SPRITE_ALPHA_DEPTH_WRITE_V1.md`](SPRITE_ALPHA_DEPTH_WRITE_V1.md) | Explicit depth-write policy for alpha SpriteBatch composition. |
| Mesh camera-space | [`MESH_CAMERA_SPACE_V1.md`](MESH_CAMERA_SPACE_V1.md) | Orientation-correct mesh near clipping and perspective depth through RenderCamera camera-space. |
| Mesh basic lighting | [`MESH_BASIC_LIGHTING_V1.md`](MESH_BASIC_LIGHTING_V1.md) | Bounded directional intensity and RGBA material tint across software mesh textures. |
| Mesh renderer atomic draw | [`MESH_RENDERER_ATOMIC_DRAW_V1.md`](MESH_RENDERER_ATOMIC_DRAW_V1.md) | Candidate-frame mesh draw that preserves the caller framebuffer on projection/raster rejection. |
| Mesh frustum clipping | [`MESH_FRUSTUM_CLIPPING_V1.md`](MESH_FRUSTUM_CLIPPING_V1.md) | Bounded six-plane camera-space clipping, normal off-frustum culling, and preserved atomic draw behavior. |
| Mesh double-sided material | [`MESH_DOUBLE_SIDED_MATERIAL_V1.md`](MESH_DOUBLE_SIDED_MATERIAL_V1.md) | Default two-sided raster versus explicit back-face culling contract. |
| Scene mesh oriented culling | [`SCENE_MESH_ORIENTED_CULLING_V1.md`](SCENE_MESH_ORIENTED_CULLING_V1.md) | Conservative instance bounding-sphere culling in oriented camera space. |
| Scene render adapter | [`SCENE_RENDER_ADAPTER_V1.md`](SCENE_RENDER_ADAPTER_V1.md) | Atomic mesh-first plus sprite scene composition entry point. |
| Scene sprite render properties | [`SCENE_SPRITE_RENDER_PROPERTIES_V1.md`](SCENE_SPRITE_RENDER_PROPERTIES_V1.md) | Staged sprite rotation, camera billboard, and depth-write propagation. |
| Editor scene session | [`EDITOR_SCENE_SESSION_V1.md`](EDITOR_SCENE_SESSION_V1.md) | Bounded hierarchy/inspector/save snapshot/viewport foundation over SceneDocument. |
| Editor SceneDocument codec | [`EDITOR_SCENE_DOCUMENT_CODEC_V1.md`](EDITOR_SCENE_DOCUMENT_CODEC_V1.md) | Bounded deterministic in-memory SceneDocument envelope with atomic decode; not filesystem persistence or an asset importer. |
| Animation state machine | [`ANIMATION_STATE_MACHINE_V1.md`](ANIMATION_STATE_MACHINE_V1.md) | Bounded scalar states, explicit transitions, and deterministic linear blend over AnimationTimeline. |
| Animation locomotion bridge | [`ANIMATION_LOCOMOTION_BRIDGE_V1.md`](ANIMATION_LOCOMOTION_BRIDGE_V1.md) | Read-only input-to-state trigger bridge that preserves transform and movement authority ownership. |
| Gameplay physics query | [`GAMEPLAY_PHYSICS_QUERY_V1.md`](GAMEPLAY_PHYSICS_QUERY_V1.md) | Read-only finite-validated XPBD raycast mapped to EntityID. |
| Gameplay physics body | [`GAMEPLAY_PHYSICS_BODY_V1.md`](GAMEPLAY_PHYSICS_BODY_V1.md) | Validated static/dynamic circle-body creation through canonical ECS components. |
| Texture import pipeline | [`TEXTURE_IMPORT_PIPELINE_V1.md`](TEXTURE_IMPORT_PIPELINE_V1.md) | Atomic in-memory PPM/BMP import through registry-ready-to-CPU-staging. |
| Mesh import pipeline | [`MESH_IMPORT_PIPELINE_V1.md`](MESH_IMPORT_PIPELINE_V1.md) | Atomic in-memory OBJ import through registry-ready-to-CPU-mesh staging. |
| Asset import surface demo | [`ASSET_IMPORT_SURFACE_DEMO_V1.md`](ASSET_IMPORT_SURFACE_DEMO_V1.md) | Finite PPM/OBJ import-to-staged-resource-to-software-surface proof with PPM artifact. |
| Motion animation surface demo | [`MOTION_ANIMATION_SURFACE_DEMO_V1.md`](MOTION_ANIMATION_SURFACE_DEMO_V1.md) | Finite single-writer kinematic movement, read-only locomotion animation, and sprite render proof. |
| Kinematic collision preflight | [`KINEMATIC_COLLISION_PREFLIGHT_V1.md`](KINEMATIC_COLLISION_PREFLIGHT_V1.md) | Read-only XPBD raycast gate that blocks or delegates one kinematic transform write. |
| Gameplay trigger tracker | [`GAMEPLAY_TRIGGER_TRACKER_V1.md`](GAMEPLAY_TRIGGER_TRACKER_V1.md) | Read-only XPBD circle-overlap enter/exit deltas with candidate-commit state. |
| Scene physics pose sync | [`SCENE_PHYSICS_POSE_SYNC_V1.md`](SCENE_PHYSICS_POSE_SYNC_V1.md) | One-way candidate-validated SceneWorld X/Z mirror into canonical ECS physics positions. |
| Trigger runtime surface demo | [`TRIGGER_RUNTIME_SURFACE_DEMO_V1.md`](TRIGGER_RUNTIME_SURFACE_DEMO_V1.md) | Finite motion-to-pose-sync-to-XPBD-trigger-to-sprite-render integration proof. |
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
