# FauzanEngine AI/Agent Work Standard

**Mandatory reading before changing this repository:** `AGENTS.md` and `docs/AI_ENGINE_WORK_STANDARD.md`.

This repository is a production-oriented 3D game-engine project. Treat the current `main` branch as canonical unless a task explicitly names another branch.

## Non-negotiable rules

1. **Inspect before editing.** Read the relevant current `main` implementation, CMake registration, tests, recent commits, related PRs, and review comments. Never assume a branch still contains work that is absent from `main`.
2. **Preserve functionality.** Do not delete working functions, public APIs, subsystems, or meaningful behavior to make a test pass. Upgrade or repair them. No mass stubs, fake implementations, placeholder success paths, or reduced workloads.
3. **One workstream, one branch.** Never edit `main` directly. Create a focused branch from the current `main` and open a PR when the work is coherent.
4. **Historical work is evidence, not authority.** Closed/merged branches may be stale. Reconstruct their useful intent and port verified defects/fixes onto current `main`; do not blindly merge obsolete heads.
5. **Integration is part of completion.** Code that exists but is not in the canonical CMake target, runtime ownership graph, scene/ECS path, renderer path, or executable smoke gate is not considered fully integrated.
6. **The engine is 3D-first.** Do not preserve a 2D/software-only architecture merely because it is easier to test. Upgrade the canonical runtime toward real Vulkan 3D while retaining software paths only where they have a legitimate fallback/HUD/compatibility role.
7. **No false validation.** Clearly distinguish source inspection, reconstructed local tests, GitHub CI, and actual device/Termux benchmarks. Never claim a build, sanitizer run, Vulkan run, or performance target passed without evidence.
8. **Performance targets are exact contracts.** For the XPBD target, the required workload is 100,000 bodies and at least 200,000 collision tests, with measured step time strictly below 5 ms. Do not lower the workload or weaken the threshold.
9. **Review defects are work items.** Unresolved review comments, CI failures, unsafe ownership, missing CMake registration, dead integration seams, and stale branch defects must be audited and repaired before declaring a workstream complete.
10. **No premature merge.** Merge only after applicable review comments are resolved, CI is green, the diff is coherent, and the implementation is actually integrated.
11. **Async completion is authoritative.** For asynchronous asset/GPU pipelines, command recording or upload submission is not completion. A resource may become Ready only after the authoritative completion mechanism (for example a fence/timeline semaphore or equivalent) has been observed successfully and the owning resource manager has committed the result.
12. **GPU ownership is explicit.** Every Vulkan resource or synchronization primitive must have one unambiguous destruction owner. Borrowed handles must never be destroyed by consumers; owned handles must be destroyed exactly once. Tests must cover duplicate-registration and teardown paths where applicable.
13. **GPU-resource lifetime is explicit.** When an uploader stores an `AssetResourceManager*` or equivalent ownership observer for pending work, the resource manager must outlive every pending upload and the uploader must be destroyed/flushed before the manager. Do not retain raw ownership observers past their owner lifetime.
14. **Residency follows authoritative completion.** A resource upload may increase an in-flight count when submission is recorded, but `gpuResident`/Ready publication can only become true after the authoritative synchronization object reports completion. Multiple uploads for one resource remain non-resident until the final in-flight upload completes.

## Required workflow

### Phase A — Repository reconstruction

- Identify current `main` SHA.
- Enumerate open PRs and historical closed PRs relevant to the work.
- Enumerate active branches and compare their heads with `main`.
- Read PR diffs, review comments, and CI status for relevant work.
- Classify each branch/PR: merged-valid, merged-with-follow-up, stale/superseded, helper/sync, closed-unmerged, or active.
- Reuse real code from history only after checking whether it already exists in `main`.

### Phase B — Defect audit

For each relevant subsystem inspect:

- public API compatibility;
- ownership and lifetime;
- exception/allocation failure paths;
- bounds and integer overflow;
- concurrency and callback lifetime;
- state-machine transitions;
- persistence/serialization compatibility;
- CMake/source registration;
- executable smoke coverage;
- runtime integration and authority ownership;
- Vulkan resource lifetime and shader deployment;
- asynchronous completion ordering and publication semantics;
- Android lifecycle/input behavior;
- performance contracts.

### Phase C — Implementation

- Make the smallest real change that fixes the defect without removing existing capability.
- Add regression coverage for every repaired defect where practical.
- Register production sources and smoke tests in canonical CMake.
- Prefer existing engine contracts over duplicate parallel APIs.
- If a subsystem is currently CPU/2D-only but its canonical role is 3D, extend it into the real 3D runtime instead of hiding the gap.
- For async GPU work, connect submission, completion observation, resource publication, renderer notification/refresh, cancellation/failure, and release/eviction through the real ownership graph; do not create a fake callback merely to advance a state enum.

### Phase D — Validation

Use the strongest available evidence in this order:

1. exact repository CI;
2. exact branch build/smoke;
3. Release + ASan/UBSan where applicable;
4. Vulkan/SDL software-ICD smoke where applicable;
5. Android/device or Termux validation where applicable;
6. performance benchmark on the actual target workload.

Report untested layers explicitly.

### Phase E — Merge discipline

Before merge:

- no unresolved blocking review defect;
- no known regression introduced to existing APIs;
- applicable CI green;
- canonical CMake integration present;
- smoke/regression tests present;
- runtime ownership/integration path verified;
- async completion-to-publication path verified where applicable;
- performance claim backed by measured evidence;
- PR body records scope, evidence, and remaining limitations.

After merge, re-check `main` because another merge may have changed the integration seam.

## Canonical architecture priorities

The target integration graph is:

`ECS ↔ Scene/Gameplay/Physics ↔ Animation/Audio ↔ Renderer/GPU ↔ Vulkan/Android`

`Asset Pipeline → Runtime consumers`

`Editor → Authoring/runtime seams`

`Networking → authoritative ECS/gameplay/physics state`

A feature is not complete merely because its source file exists. It must have a live path through the canonical graph or be explicitly documented as an isolated foundation.

## Current project priorities

Before starting new P0–P3 feature expansion, finish the outstanding repair/integration backlog:

1. close the remaining defects from unmerged/open historical work, especially #44/#48 lineage, #63 P8, and relevant #8 defects;
2. repair Vulkan/P6 shader deployment and ownership defects;
3. connect `SceneWorld`/ECS mesh data to the canonical Vulkan 3D renderer;
4. make `NeoRuntime` 3D-first without deleting legitimate software fallback paths;
5. integrate canonical XPBD V5 into the runtime and authoritative Scene/Gameplay path;
6. repair asset streaming GPU ownership and resident-budget accounting and verify submission→GPU completion→Ready publication→renderer refresh→release/eviction end to end. The current P1 implementation also requires uploader-before-resource-manager teardown ordering and blocks release/eviction/hot-reload while uploads remain in flight;
7. complete animation, audio, networking, and Android runtime seams;
8. only then resume the planned P0–P3 expansion/closure work.

## Forbidden shortcuts

- Do not replace a real subsystem with a stub.
- Do not remove a failing test instead of fixing its cause.
- Do not reduce 100K/200K XPBD workload to pass.
- Do not change `<5 ms` to `<=5 ms`.
- Do not resurrect a stale branch wholesale when the same functionality is already on `main`.
- Do not call a source-only seam "production integrated".
- Do not claim Unreal parity from file count; judge it from working end-to-end capability.
- Do not mark GPU residency from command recording or queue submission alone.
- Do not destroy, reset, or evict a resource manager while an uploader still retains pending resource-tracking tasks.

## Handoff requirement

Every agent that changes the repository must leave enough information for the next agent to continue without reconstructing the task from chat:

- branch and base SHA;
- files changed;
- defects found;
- implementation decisions;
- tests/CI actually run;
- known unverified layers;
- next blocking integration task;
- PR number when applicable.

The authoritative long-form procedure is `docs/AI_ENGINE_WORK_STANDARD.md`.

## Mandatory multi-room governance

Before changing this branch, every room/agent MUST read [`docs/ROOM_WORK_GOVERNANCE_V1.md`](docs/ROOM_WORK_GOVERNANCE_V1.md). It is binding across the six work branches and defines ownership, gap status, canonical CMake, Release/ASAN evidence, handover, and zero-conflict merge rules.
