# BRANCH STATUS — p3-editor-android-production-night

## Checkpoint
- Branch: `p3-editor-android-production-night`
- PR: #71 — `WIP: P0-P3 canonical runtime, renderer, physics, Android integration`
- Main baseline: `2b9bd6018fd7d36733602d8bfa0cc4d061e1a4f7`
- Last code checkpoint: `27fc172c92ec30f8408cde7189385a4da0a01e7d`
- Latest documentation checkpoint: `97041be67e347a02b80754731edbc8731b91281b`
- This status update is the latest handoff checkpoint.
- Main merge: **NOT AUTHORIZED / DO NOT MERGE**
- Termux/device verification: **UNVERIFIED — TERMUX REQUIRED**

## Progress
These percentages are roadmap completion indicators, not production certification.

| Roadmap | Progress | Evidence state |
|---|---:|---|
| P0 Canonical Runtime & ECS | 85% | IMPLEMENTED-UNVERIFIED for runtime/device portions |
| P1 Renderer + Asset + Animation | 25% | mixed CI + IMPLEMENTED-UNVERIFIED |
| P2 Physics + Gameplay + Networking | 20% | physics timing instrumentation; target performance CONTRACT-ONLY |
| P3 Editor + Audio + Android + Services | 15% | mixed existing foundation + CI |
| P4 Production Certification | 0% | not started |
| **Overall branch work** | **45%** | roadmap work in progress |

## What is complete on this branch
- Canonical `NeoRuntime` ECS ownership.
- SceneWorld → SceneECSBridge → ArchetypeManager synchronization.
- Incremental Scene→ECS transform sync with rotation and scale.
- ECS transform sourcing in active Vulkan 3D runtime path.
- ECS-authoritative 64-bit mesh/material asset identity synchronization and Vulkan identity validation.
- Archetype component migration preserves position/velocity/collider/mesh/rotation/scale data.
- Runtime vertical-slice gate and smoke.
- XPBD total-step wall-clock instrumentation.
- GPU skinning palette buffer foundation and initial Vulkan GLSL skinning shader.
- Required CMake registrations and R2 closure fixes.
- Cross-room handover protocol and branch checkpoint documentation are now present.

## CI evidence — exact code checkpoint
Commit `140449a41cbd04c09e74c7294a9d0e27b8264183` passed:
- CI Lint & Type Check
- Renderer 3D Vulkan Smoke
- PBR Validation
- Build Android APK
- R1 Canonical Game Tool
- R2 canonical Farm loop
- R3 Farm renderer path
- R5 Farm authority reconnect
- R6 Farm fraud trust

Documentation commits after that code checkpoint do not change engine behavior.

## Important unresolved gaps
### 1. ECS mesh/material identity is now authoritative at the runtime binding boundary
ECS now carries the existing staging `sourceHash` values as 64-bit mesh/material identity fields. `SceneECSBridge` synchronizes those identities from the existing `SceneMeshAdapter` bindings, and the active Vulkan path rejects an instance when ECS identity does not match the staged resource identity.

The actual geometry/material CPU resource remains owned by the existing staging/adapter path; no duplicate asset registry was introduced and no 64-bit hash was truncated to 32 bits.

Therefore:
- **Transform:** ECS-authoritative on the active Vulkan path.
- **Mesh/material identity:** ECS-authoritative at the runtime binding/validation boundary.
- **Geometry/material payload:** existing staging/SceneMeshAdapter resource owner.

This is implemented but still requires CI/device runtime verification.

### 2. XPBD performance target
Acceptance target remains:
- 100,000 bodies
- 200,000 collision tests
- strictly <5 ms

Timing instrumentation exists. The target is **not proven** until an actual benchmark produces the required workload and measured wall time.

### 3. GPU skinning
Palette buffer and shader foundation exist. Actual descriptor/vertex-input/pipeline/push-constant compatibility and device execution remain unverified.

### 4. Android
CI APK passes, but device matrix/install/run/Vulkan driver/device-loss/suspend-resume evidence remains pending.

## Exact next action
1. Poll CI for code checkpoint `5d917773bf17617c3689deb88e48e11dc2c63c45` and repair any compile/test regression.
2. Verify the new GPU skinning path through CI/device execution. The skinned pipeline now has descriptor-set palette binding, matching vertex inputs, shader generation, and a dedicated renderer draw API; active SceneMeshAdapter character data is still not wired into this draw path.
3. Continue P1 asset/animation pipeline integration without creating duplicate registries.
4. Then advance P2 XPBD proof and P3 Android/device evidence.
5. Keep main untouched and do not claim Termux/device verification.

## Room continuation rule
A new room must start from:
1. `AGENTS.md`
2. `docs/AI_ENGINE_WORK_STANDARD.md`
3. `AI_HANDOVER_PROTOCOL.md`
4. this `BRANCH_STATUS.md)

Then verify the branch HEAD and PR state, inspect the exact current files, and continue from **Exact next action**. Do not rely on an older chat summary if repository evidence differs.

## User engineering constraints
- No destructive rewrites.
- No mass stubs.
- No removal of original functions to make compilation easier.
- No downgrade/simplification of intended engine capability.
- No invented paths or APIs.
- No merge to main without explicit user authorization.
- Workflow: READ → ANALYZE → EDIT → BUILD/CI → TEST → BENCHMARK → REVIEW → APPROVE → COMMIT → PUSH.
