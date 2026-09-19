# BRANCH STATUS — p3-editor-android-production-night

## Checkpoint
- Branch: `p3-editor-android-production-night`
- PR: #71 — `WIP: P0-P3 canonical runtime, renderer, physics, Android integration`
- Main baseline: `2b9bd6018fd7d36733602d8bfa0cc4d061e1a4f7`
- Last code checkpoint: `140449a41cbd04c09e74c7294a9d0e27b8264183`
- Latest documentation checkpoint: `112fd47a182c8f6b435509bdffd19a68c23b0b07`
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
- Archetype component migration preserves position/velocity/collider/mesh/rotation/scale data.
- Runtime vertical-slice gate and smoke.
- XPBD total-step wall-clock instrumentation.
- GPU skinning palette buffer foundation and initial Vulkan GLSL skinning shader.
- Required CMake registrations and R2 closure fixes.
- Current CI checkpoint `140449a4` passed:
  - CI Lint & Type Check
  - Renderer 3D Vulkan Smoke
  - PBR Validation
  - Build Android APK
  - R1 Canonical Game Tool
  - R2 canonical Farm loop
  - R3 Farm renderer path
  - R5 Farm authority reconnect
  - R6 Farm fraud trust

## Important unresolved gaps
### 1. ECS mesh/material identity is not yet authoritative
`ArchetypeManager` contains `COMP_MESH` and `meshID`, but `SceneECSBridge` currently creates entities with only `COMP_POSITION | COMP_ROTATION`.

The active Vulkan renderer still obtains mesh geometry/material identity from `SceneMeshAdapter` and only obtains transforms from ECS.

Therefore:
- **Transform:** ECS-authoritative on the active Vulkan path.
- **Mesh/material identity:** SceneMeshAdapter-authoritative.
- **ECS meshID:** existing but not integrated into the active renderer path.

Do not fix this by truncating a 64-bit asset hash into a 32-bit ID or by inventing a second resource registry. First audit the existing AssetRegistry/AssetResourceManager/SceneMeshAdapter contracts and choose an existing stable identity mechanism if one exists.

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
1. Audit `AssetRegistry`, `AssetResourceManager`, `SceneMeshAdapter`, `SceneRenderAdapter`, and existing asset identity fields.
2. Determine whether an existing stable mesh/material identity can safely become ECS-authoritative.
3. If yes, integrate with exact existing contracts and add a focused smoke test.
4. If no, document the boundary and continue P0/P1 without creating a duplicate registry.
5. Re-run CI after any code change.
6. Keep main untouched and do not claim Termux/device verification.

## Room continuation rule
A new room must start from this file, verify the branch HEAD, inspect the exact current files, then continue from **Exact next action**. Do not rely on an older chat summary if repository evidence differs.

## User engineering constraints
- No destructive rewrites.
- No mass stubs.
- No removal of original functions to make compilation easier.
- No downgrade/simplification of intended engine capability.
- No invented paths or APIs.
- No merge to main without explicit user authorization.
