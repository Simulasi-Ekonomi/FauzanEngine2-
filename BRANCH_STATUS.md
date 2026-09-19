# BRANCH STATUS — p3-editor-android-production-night

## Checkpoint — 2026-09-19
- Branch: `p3-editor-android-production-night`
- PR: #71 — `WIP: P0-P3 canonical runtime, renderer, physics, Android integration`
- Main baseline: `2b9bd6018fd7d36733602d8bfa0cc4d061e1a4f7`
- Current branch HEAD: `c2df50300e488936cb777bf9a543f8c988f4af30`
- Latest branch commit: `fix: link Vulkan GPU buffer into runtime library`
- Main merge: **NOT AUTHORIZED / DO NOT MERGE**
- Termux/device verification: **UNVERIFIED — TERMUX REQUIRED**
- PR CI merge-ref tested: `1564142adff56d98a490c79856830c52010a378b`
- Current CI state: **PARTIAL / BLOCKED** — all listed workflows passed except `Renderer 3D Vulkan Smoke`.

## CI evidence for current checkpoint
For PR #71 merge-ref `1564142`:
- **PASS:** CI - Lint & Type Check
- **PASS:** PBR Validation
- **PASS:** R6 Farm fraud trust
- **PASS:** R5 Farm authority reconnect
- **PASS:** R1 Canonical Game Tool
- **PASS:** R2 canonical Farm loop
- **PASS:** Build Android APK
- **PASS:** R3 Farm renderer path
- **FAIL:** Renderer 3D Vulkan Smoke — native link stage

The failing linker diagnostics include unresolved symbols for:
- `VulkanGPUBuffer`
- `VulkanDescriptorManager`
- `GPUSkinningPaletteBuffer`

Commit `c2df503` adds `Runtime/VulkanGPUBuffer.cpp` to `XPBD_RUNTIME_SOURCES`, but the PR merge-ref still fails to link the complete Vulkan 3D smoke target. This is a real CMake/source-registration defect, not a runtime benchmark result.

## Roadmap progress
Percentages are roadmap completion indicators only; they are not production certification.

| Roadmap | Current | Evidence |
|---|---:|---|
| P0 Canonical Runtime & ECS | 85% | IMPLEMENTED-UNVERIFIED for runtime/device layers |
| P1 Renderer + Asset + Animation | 30% | IMPLEMENTED-UNVERIFIED; Vulkan/animation integration still has CI link gap |
| P2 Physics + Gameplay + Networking | 20% | CONTRACT-ONLY for 100K/200K/<5 ms; broader integration remains |
| P3 Editor + Audio + Android + Services | 15% | Mixed implemented foundation; device/runtime gates remain |
| P4 Final Production Certification | 0% | Not started |
| **Overall branch work** | **46%** | Roadmap work in progress |

**100% target rule:** P0, P1, P2, P3 and P4 each reach 100% only when every applicable implementation, integration, CI, runtime/device, sanitizer, benchmark, packaging, and release gate in the master handover has executable evidence. These percentages must never be raised merely because source files exist.

## Current implemented work
- Canonical `NeoRuntime` ECS ownership.
- SceneWorld → SceneECSBridge → ArchetypeManager synchronization.
- Incremental Scene→ECS transform sync including rotation and scale.
- ECS transform sourcing in the active Vulkan 3D runtime path.
- ECS-authoritative 64-bit mesh/material asset identity synchronization and Vulkan identity validation.
- Archetype component migration preserves position/velocity/collider/mesh/rotation/scale data.
- Runtime vertical-slice gate and smoke.
- XPBD total-step wall-clock instrumentation.
- GPU skinning palette buffer foundation and Vulkan skinning pipeline/shader.
- SceneMeshAdapter skeletal binding with validated four-influence weights and animation controller/palette state.
- NeoRuntime animation advancement before render extraction.
- Active Vulkan adapter path for skinned draw submission.
- Asset streaming GPU ownership and resident-budget repair is present in branch history.
- Required CMake registrations have been progressively repaired; current Vulkan smoke still exposes missing target linkage.

## Important unresolved gaps
### 1. Vulkan 3D build/link closure — BLOCKED
The current PR merge-ref does not link the complete Vulkan renderer target. Repair canonical CMake registration for the unresolved Vulkan/animation implementation objects, then rerun the exact workflow.

### 2. GPU skinning
Foundation and active draw path exist, but full CI/runtime/device proof is not complete. Multi-instance palette batching and animation LOD remain P1 work.

### 3. Asset pipeline
Streaming queue ownership/budget behavior is implemented and smoke-covered in the branch lineage, but the full importer → decoder → staging → GPU upload → dependency/hot-reload → packaging pipeline is not complete.

### 4. XPBD performance
Acceptance target remains:
- 100,000 bodies
- at least 200,000 collision tests
- measured step time **strictly < 5 ms**

Instrumentation exists; **VERIFIED-BENCH is absent**.

### 5. Android
CI APK build passes. Device install/run, Vulkan driver matrix, lifecycle suspend/resume, device-loss recovery, and release artifact certification remain unverified.

### 6. Networking/editor/services
Foundations exist, but production transport, prediction/reconciliation/interpolation, interest management, full editor authoring/PIE, persistence/telemetry/security/commerce/live-ops closure and end-to-end certification remain open.

## Exact next action
1. Repair the Vulkan 3D smoke linker closure on `p3-editor-android-production-night` without deleting or bypassing existing renderer/animation functionality.
2. Rerun PR #71 CI on the new branch commit.
3. Only after CI is green, continue P1 animation batching/LOD and complete asset import/upload/hot-reload integration.
4. Then advance P2 toward the real 100K bodies / 200K collision tests / <5 ms benchmark.
5. Keep main untouched and do not claim Termux/device verification.

## Verification labels
- `VERIFIED-CI`: exact workflow passed on the stated commit.
- `VERIFIED-TERMUX`: user/device Termux execution passed on the stated commit.
- `VERIFIED-BENCH`: measured benchmark passed with required workload and timing.
- `IMPLEMENTED-UNVERIFIED`: source/integration exists but runtime/device evidence is missing.
- `CONTRACT-ONLY`: acceptance contract/test exists but target behavior/performance is not proven.
- `BLOCKED`: concrete dependency prevents safe continuation.

## Room continuation
Read in order:
1. `AGENTS.md`
2. `docs/AI_ENGINE_WORK_STANDARD.md`
3. `AI_HANDOVER_PROTOCOL.md`
4. this file

Then verify branch HEAD, PR #71, exact CI, and the current source before editing.

## User engineering constraints
- No destructive rewrites.
- No mass stubs.
- No removal of original functions to make compilation easier.
- No downgrade/simplification of intended engine capability.
- No invented paths or APIs.
- No merge to main without explicit user authorization.
- Workflow: READ → ANALYZE → EDIT → BUILD/CI → TEST → BENCHMARK → REVIEW → APPROVE → COMMIT → PUSH.
