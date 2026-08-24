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
