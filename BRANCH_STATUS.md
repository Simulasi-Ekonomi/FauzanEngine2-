# BRANCH STATUS — p3-editor-android-production-night

## Checkpoint
- Branch: `p3-editor-android-production-night`
- PR: #71 — `WIP: P0-P3 canonical runtime, renderer, physics, Android integration`
- Main baseline: `2b9bd6018fd7d36733602d8bfa0cc4d061e1a4f7`
- Latest code checkpoint: `a0b41b391da5a075a215a6572b5a3d4cdb2be45f`
- Previous checkpoint: `66bd9aebb35f1618442e61a201bb34ea4f804466`
- Latest CI for previous checkpoint exposed a renderer compile regression; fixed in `a0b41b3`.
- Main merge: **NOT AUTHORIZED / DO NOT MERGE**
- Termux/device verification: **UNVERIFIED — TERMUX REQUIRED**
- Latest CI for `a0b41b3`: workflow runs have not appeared yet; **CI PENDING**.

## Progress
These percentages are roadmap completion indicators, not production certification.

| Roadmap | Progress | Evidence state |
|---|---:|---|
| P0 Canonical Runtime & ECS | 85% | IMPLEMENTED-UNVERIFIED for runtime/device portions |
| P1 Renderer + Asset + Animation | 30% | IMPLEMENTED-UNVERIFIED; active animation wiring is in branch |
| P2 Physics + Gameplay + Networking | 20% | timing instrumentation; target performance CONTRACT-ONLY |
| P3 Editor + Audio + Android + Services | 15% | mixed existing foundation + CI |
| P4 Production Certification | 0% | not started |
| **Overall branch work** | **46%** | roadmap work in progress |

## Latest repair
Commit `a0b41b391da5a075a215a6572b5a3d4cdb2be45f` fixes the compile regression introduced when animation headers exposed the canonical `NeoEngine::Mat4` to `SceneRenderAdapter.cpp`.

The renderer adapter already had a private local matrix type named `Mat4`. That became ambiguous with the canonical animation/runtime `Mat4`. The fix renames the renderer-local type to `RenderMat4` and keeps the animation palette explicitly typed as `NeoEngine::Mat4`.

No renderer behavior or original function was removed.

## Current implemented work
- Canonical `NeoRuntime` ECS ownership.
- SceneWorld → SceneECSBridge → ArchetypeManager synchronization.
- Incremental Scene→ECS transform sync with rotation and scale.
- ECS transform sourcing in active Vulkan 3D runtime path.
- ECS-authoritative 64-bit mesh/material asset identity synchronization and Vulkan identity validation.
- Archetype component migration preserves position/velocity/collider/mesh/rotation/scale data.
- Runtime vertical-slice gate and smoke.
- XPBD total-step wall-clock instrumentation.
- GPU skinning palette buffer foundation and Vulkan skinning pipeline/shader.
- SceneMeshAdapter skeletal binding owns validated four-influence weights plus SkeletalAnimationController/palette state.
- NeoRuntime advances bound animations before render extraction.
- Active Vulkan adapter routes a bound mesh into the skinned draw API.
- Required CMake registrations and Farm/R2 closure fixes.
- Cross-room handover protocol and branch checkpoint documentation.

## CI diagnosis for `66bd9aebb35f1618442e61a201bb34ea4f804466`
PBR, lint/type-check, and Android APK passed.

Renderer 3D Vulkan, R1, R2, R3, R5, and R6 failed during native build because `SceneRenderAdapter.cpp` had an ambiguous local `Mat4` after the canonical `Mat4` was unified through `MathTypes.h`.

The decisive compiler error was:
`reference to ‘Mat4’ is ambiguous`
with candidates:
- `NeoEngine::{anonymous}::Mat4`
- canonical `NeoEngine::Mat4`

This was repaired in `a0b41b3`. The failure was compile-time; it did not establish a runtime rendering defect.

## Important unresolved gaps
### 1. GPU skinning
Implemented through:
`Skeleton/SkeletalAnimationController → SceneMeshAdapter palette/weights → Vulkan skinned draw`.

Still unverified on CI after `a0b41b3`, and unverified on Termux/device. Animation batching and LOD are still future P1 work.

### 2. XPBD performance target
Acceptance target remains:
- 100,000 bodies
- 200,000 collision tests
- strictly <5 ms

Instrumentation exists. The target is **not proven** until an actual benchmark produces the required workload and measured wall time.

### 3. Android
CI APK passes, but device matrix/install/run/Vulkan driver/device-loss/suspend-resume evidence remains pending.

## Exact next action
1. Wait/poll CI for `a0b41b391da5a075a215a6572b5a3d4cdb2be45f`.
2. Repair the next compile/test regression if any.
3. Once CI is green, continue P1 animation production: batching/LOD and production asset/import binding without duplicate registries.
4. Then advance P2 XPBD proof toward the exact 100K bodies / 200K collision tests / <5 ms benchmark.
5. Keep main untouched and do not claim Termux/device verification.

## Room continuation rule
A new room must start from:
1. `AGENTS.md`
2. `docs/AI_ENGINE_WORK_STANDARD.md`
3. `AI_HANDOVER_PROTOCOL.md`
4. this `BRANCH_STATUS.md`

Then verify branch HEAD and PR state, inspect exact current files, and continue from **Exact next action**. Do not rely on older chat summary if repository evidence differs.

## User engineering constraints
- No destructive rewrites.
- No mass stubs.
- No removal of original functions to make compilation easier.
- No downgrade/simplification of intended engine capability.
- No invented paths or APIs.
- No merge to main without explicit user authorization.
- Workflow: READ → ANALYZE → EDIT → BUILD/CI → TEST → BENCHMARK → REVIEW → APPROVE → COMMIT → PUSH.
