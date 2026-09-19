# FauzanEngine2- — Production Execution, Integration & CI Rules

**Scope:** canonical engine development from subsystem audit through production certification.  
**Branch policy:** this document may be updated on an active work/production branch, but it does **not** authorize merge to `main`.  
**Current canonical source:** `Source/NeoEngine`.

## 1. Purpose

This document defines how FauzanEngine2- is evaluated and advanced from isolated capability proofs to a production-integrated engine.

A subsystem is **not production-ready merely because**:
- its source exists;
- CMake includes it;
- a unit/smoke test passes;
- a CPU/offscreen/localhost proof passes;
- an Android cross-compile succeeds;
- a benchmark exists.

The required transition is:

`implementation -> regression evidence -> canonical ownership -> runtime integration -> Release/ASAN -> CI -> platform/device evidence -> production certification`

## 2. Non-negotiable engineering rules

1. **Upgrade only.** Do not delete original functionality to make a build or test pass.
2. **No simplification/downgrade.** Performance, architecture, validation, determinism, and feature scope may not be weakened merely to pass CI.
3. **No fake completion.** Stubs, placeholder APIs, empty implementations, fabricated benchmark results, fabricated device evidence, or documentation-only completion do not count.
4. **Read before edit.** Inspect the canonical implementation, callers, CMake registration, tests, and relevant CI workflow before changing code.
5. **Preserve existing behavior.** New behavior must be additive or a deliberate compatible upgrade.
6. **Release correctness matters.** Expressions with side effects must never be placed only inside `assert(...)` or a debug-only macro.
7. **Failure must be explicit.** Production paths must fail closed or return a real error; they must not rely on debug assertions for essential runtime safety.
8. **One canonical path.** Do not repair obsolete duplicate trees unless audit proves they are active build inputs.
9. **No premature merge.** PRs remain unmerged until their required integration and evidence gates are satisfied.
10. **Evidence is scoped.** A Farm/local/CPU/Vulkan-offscreen proof proves only the exact contract it tests.
11. **P0–P4 are mandatory.** Production certification requires 100% completion of all required P0, P1, P2, P3 and P4 criteria.
12. **No percentage inflation.** If evidence is incomplete, report the gap rather than inventing a readiness percentage.

## 3. Canonical workflow

### Step 1 — Inventory

Identify:
- canonical source path;
- active branch and base SHA;
- CMake targets;
- runtime owners;
- existing tests;
- CI workflows;
- documentation claiming readiness;
- duplicate/legacy paths.

### Step 2 — Audit

For every capability record:

| Field | Required |
|---|---|
| Implementation | exact file/class/function |
| Owner | canonical runtime owner |
| Inputs | source/data contract |
| Outputs | runtime/render/network/persistence effect |
| Integration | actual caller chain |
| Failure behavior | explicit failure path |
| Release test | Release executable evidence |
| Sanitizer | ASAN/other required evidence |
| CI | workflow and trigger |
| Platform | desktop/Android/device evidence where required |
| Remaining gap | concrete missing production behavior |

### Step 3 — Integrate

An isolated subsystem becomes production-integrated only when the canonical runtime owns it and the real frame/game lifecycle exercises it.

Example:

`Input -> Simulation -> Physics -> Scene snapshot -> Animation -> Render commands -> Audio -> Persistence/telemetry`

A subsystem with only a standalone smoke remains **isolated**.

### Step 4 — Validate

Required validation depends on the subsystem, but production work normally requires:
- Release build;
- Release runtime smoke;
- ASAN regression;
- deterministic/replay checks where applicable;
- failure-path tests;
- CI evidence;
- platform/device evidence where applicable;
- performance evidence where the gate specifies a budget.

### Step 5 — Review

Before promotion:
- compare changed files against canonical ownership;
- check for duplicate implementations;
- check CMake/source registration;
- check Release semantics;
- inspect test quality;
- inspect CI trigger and final gate logic;
- update evidence documentation.

## 4. P0–P4 execution model

### P0 — Canonical Runtime & ECS

Required chain:

`NeoRuntime -> frame contract -> ECS/SceneWorld -> gameplay state -> lifecycle/persistence -> deterministic frame completion`

Audit targets:
- runtime initialization/shutdown;
- frame token/revision;
- ECS ownership;
- SceneWorld lifecycle and transforms;
- gameplay actor ownership;
- save/restore boundary;
- pause/time-scale/failure lifecycle;
- Release-safe error handling.

**Current major gap:** several gameplay/physics/animation systems have proofs but are not yet all owned by the canonical runtime.

### P1 — Renderer, Assets & Animation

Required chain:

`asset bytes -> registry -> importer -> content validation -> material/texture/mesh resources -> GPU resources -> descriptors/shaders -> scene draw`

Plus:

`animation asset -> pose evaluation -> skin palette -> skinned GPU draw`

Required production concerns:
- texture/mesh/material ownership;
- live references and dependency validation;
- GPU lifetime;
- reload/refresh;
- camera;
- lighting;
- animation;
- resize/surface lifecycle;
- device-loss/recreation;
- physical GPU evidence.

**Current major gap:** material/texture/PBR and asset-to-GPU ownership remain incomplete even though bounded CPU/Vulkan proofs exist.

### P2 — Physics, Gameplay & Networking

Required chain:

`input/network command -> authoritative gameplay -> XPBD/collision -> transform/state -> replication -> reconciliation`

Required:
- XPBD owned by canonical runtime;
- collision/material response connected to gameplay;
- character/NPC/building/item/quest state;
- authoritative multiplayer transport;
- authentication/session binding;
- prediction/reconciliation;
- reconnect;
- multi-client/load evidence.

**Current major gap:** XPBD has real isolated evidence but is not yet canonical `NeoRuntime` gameplay physics.

### P3 — Editor, Audio, Android & Production Services

Required:
- editor authoring -> canonical content/runtime;
- audio ownership in runtime lifecycle;
- real mobile input/UI/audio/storage lifecycle;
- Android APK/AAB packaging;
- emulator and physical-device evidence;
- telemetry/trust/economy/persistence service boundaries.

**Current major gap:** Android is still primarily cross-compile/debug proof; production device and signed release evidence remain open.

### P4 — Final Production Certification

P4 is not another feature bucket. It is the final evidence gate.

Required:
- all P0–P3 gates complete;
- clean Release build;
- sanitizer evidence;
- performance/load evidence;
- supported-platform matrix;
- package hashes;
- signing provenance;
- security/privacy evidence;
- persistence/recovery evidence;
- regression suite;
- known-issues review;
- reproducible release record.

No P4 pass while any mandatory P0–P3 integration is missing.

## 5. Status vocabulary

Use only these statuses:

- **Implemented** — code exists and has direct regression evidence.
- **Integrated** — canonical runtime owns and exercises it.
- **Partial** — meaningful implementation exists but required production behavior is missing.
- **Isolated** — subsystem/test works independently but is not canonical-runtime integrated.
- **Documentation-only** — planned/claimed but no sufficient executable evidence.
- **Missing** — required implementation does not exist.
- **Passed (scoped)** — exact gate scope has complete evidence.
- **Not passed** — mandatory production evidence is incomplete.

A scoped Farm pass must never be rewritten as a universal engine pass.

## 6. CI rules

### 6.1 CI is a gate, not the implementation

CI must validate real behavior. It must not be modified to conceal failures.

Allowed:
- trigger a workflow on the correct production/work branch;
- split Release and ASAN stages;
- collect diagnostics after a failure;
- fix stale dependency installation;
- fix incorrect final-gate logic;
- change test ordering so diagnostics continue.

Not allowed:
- remove a failing test;
- weaken an assertion;
- replace runtime validation with a no-op;
- mark a failed smoke as success;
- use `continue-on-error` without a later gate that checks the actual exit status;
- remove Release validation because ASAN passes.

### 6.2 Release test rule

Every runtime smoke that matters to release must execute its operations outside assertions.

Bad:

`assert(runtime.Initialize(config));`

In a Release build with `NDEBUG`, the initialization call can disappear.

Correct pattern:

`if (!runtime.Initialize(config)) { return failure; }`

Then assertions/checks may validate pure conditions.

### 6.3 ASAN rule

ASAN is complementary to Release, not a replacement.

For critical runtime paths:
1. build Release;
2. run Release smoke;
3. if Release fails, capture diagnostics;
4. run ASAN unless the environment itself prevents it;
5. final gate checks actual Release build/runtime exit codes **and** ASAN result.

### 6.4 Workflow trigger rule

A workflow that protects a production branch must actually trigger for changes on that branch.

When adding a production work branch trigger:
- preserve main trigger;
- preserve relevant path filters;
- do not broaden unrelated workflows unnecessarily;
- verify the workflow run after the change.

### 6.5 CMake rule

Any new production source/test must be:
1. physically present in the canonical tree;
2. registered in the correct CMake target;
3. built by the relevant workflow;
4. executed by a real smoke/regression where appropriate.

A source file existing in Git without target registration does not count as integrated.

### 6.6 CI dependency rule

Pin or explicitly control critical dependencies where reproducibility matters.

CI must use the same canonical source layout and supported dependency family as the production target. Stale SDL2/SDL3, JSON, Vulkan, shader, or target names must be corrected rather than worked around with alternate fake targets.

### 6.7 CI evidence rule

A green workflow proves only the tests that workflow actually executed.

Record:
- workflow;
- run ID;
- commit SHA;
- build configuration;
- test target;
- runtime environment;
- Release result;
- ASAN result;
- artifacts/logs where required.

## 6.8 Current dependency baseline — mandatory

The repository CI baseline is **Node.js 24.x LTS + SDL3 3.4.16**.

All active workflows/tests consuming these dependencies must use the same baseline. Node 20 and SDL 3.4.14 are retired from the active CI baseline and must not be restored as isolated fixes.

See `docs/CI_RUNTIME_DEPENDENCY_BASELINE_2026-09.md` for the complete handoff record.

A future dependency migration requires a complete active-workflow/test-surface update and an explicit update of that baseline document. Never downgrade a single workflow to hide a compatibility failure.

## 7. Current critical CI precedent

The R3 Vulkan workflow demonstrates the intended pattern:

`Release build -> Release runtime smoke -> diagnostic GDB if needed -> ASAN build -> ASAN runtime smoke -> final gate`

The final R3 gate must inspect the actual exported Release statuses:

- build status;
- Farm smoke status;
- NeoRuntime Vulkan smoke status;
- ASAN result.

A failed Release smoke therefore remains a failed gate even if the diagnostic step succeeds.

## 8. Test-harness safety rules

Audit all tests for:

- side-effectful `assert` expressions;
- debug-only macros around required operations;
- tests that pass because setup never executed in Release;
- tests that validate only object construction rather than behavior;
- missing negative/failure-path checks;
- Release/ASAN asymmetry.

The same rule applies to engine macros such as `NEO_ASSERT`: required runtime operations must not disappear from non-debug builds.

## 9. Evidence hierarchy

From weakest to strongest:

1. source exists;
2. compile-only;
3. unit test;
4. Release smoke;
5. ASAN/regression;
6. canonical runtime integration;
7. end-to-end package/runtime test;
8. emulator/device evidence;
9. production-like load/recovery/security evidence;
10. reproducible release certification.

Do not promote a lower level into a higher level by wording.

## 10. Documentation maintenance

When implementation changes:
- update the relevant evidence document;
- update `release_readiness_matrix.md` only when the scoped evidence actually changes;
- update the detailed release gate document;
- update this execution-rules document only when the engineering/CI contract changes.

Documentation must describe the repository's actual state, including remaining gaps.

## 11. Merge/release policy

### Work branch

Implementation and CI repair may proceed on the designated work branch.

### PR

A PR may contain:
- feature implementation;
- integration;
- regression tests;
- CI corrections;
- evidence documentation.

A PR is not automatically production-ready because all its CI checks are green.

### Main / release

Do not merge/promote when:
- any mandatory P0–P4 requirement is incomplete;
- required runtime integration is isolated;
- Release evidence is missing;
- platform evidence is missing where mandatory;
- performance/load budget is unproven;
- security/privacy/recovery evidence is incomplete.

## 12. Current priority queue

The current repository audit identifies these as high-impact integration gaps:

1. **P0 runtime ownership:** connect gameplay subsystems into the canonical frame lifecycle.
2. **P2 physics:** connect `XPBDPhysicsSystem` to `NeoRuntime` gameplay/scene state.
3. **P1 asset/render:** complete asset -> material/texture -> GPU -> scene ownership and binding.
4. **P1 animation:** connect animation evaluation to runtime ticks and GPU skinning.
5. **P3 Android:** move from cross-compile/debug proof to actual APK/AAB + emulator/device lifecycle evidence.
6. **P2 networking:** move from local/loopback proofs to authenticated authoritative transport and multi-client evidence.
7. **P3/P4 persistence/security:** durable storage, migration, backup/recovery, security/privacy operations.
8. **Test correctness:** remove side-effectful Release hazards from assertions/macros across the active canonical test/runtime surface.
9. **P4 certification:** assemble reproducible evidence only after P0–P3 integration is complete.

## 13. Golden rule

> **Green CI means the executed checks passed. It does not mean the engine is 100% complete.**
>
> **100% production readiness means every required capability is implemented, canonically integrated, regression-tested, CI-validated, platform-validated where required, and backed by reproducible evidence.**
