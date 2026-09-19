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
10. **No premature merge.** Merge only after applicable review comments are resolved, CI is green, the diff is coherent, and the implementation is actually integrated. If validation is unavailable, leave the PR open and state exactly what remains unverified.

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
- Android lifecycle/input behavior;
- performance contracts.

### Phase C — Implementation

- Make the smallest real change that fixes the defect without removing existing capability.
- Add regression coverage for every repaired defect where practical.
- Register production sources and smoke tests in canonical CMake.
- Prefer existing engine contracts over duplicate parallel APIs.
- If a subsystem is currently CPU/2D-only but its canonical role is 3D, extend it into the real 3D runtime instead of hiding the gap.

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
6. repair asset streaming GPU ownership and resident-budget accounting;
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


## Cross-room continuation and branch percentage (mandatory)

The repository is worked on across multiple ChatGPT/Claude/agent rooms. Chat history is not authoritative; the branch checkpoint is.

Before changing code in any active work branch:
1. Read `AI_HANDOVER_PROTOCOL.md`.
2. Read the branch's `BRANCH_STATUS.md`.
3. Verify the branch HEAD and PR state from GitHub.
4. Inspect the exact current source at that HEAD.
5. Continue from the branch status's `NEXT` item only after confirming it still matches the code.

Every active work branch MUST contain `BRANCH_STATUS.md` with:
- branch name;
- base/main SHA;
- current HEAD;
- PR number;
- overall progress %;
- P0/P1/P2/P3/P4 progress %;
- exact completed work;
- exact verification evidence;
- explicit UNVERIFIED layers;
- concrete unresolved gaps;
- one exact next action.

Percentages are roadmap progress indicators, not claims of production readiness. Code that is implemented but not runtime/device tested remains IMPLEMENTED-UNVERIFIED. Acceptance APIs/tests without target measurements remain CONTRACT-ONLY.

When a room reaches its context/tool limit, the agent must commit completed work to the active branch, update `BRANCH_STATUS.md`, and leave the branch in a directly continuable state. The next room must not reconstruct the task from memory or older chat when repository evidence is newer.

Status labels:
- `VERIFIED-CI`
- `VERIFIED-TERMUX`
- `VERIFIED-BENCH`
- `IMPLEMENTED-UNVERIFIED`
- `CONTRACT-ONLY`
- `BLOCKED`

Current detailed checkpoint and percentages are maintained in `BRANCH_STATUS.md`.


## Current repository checkpoint — 2026-09-19

The active work branch is `p3-editor-android-production-night` at code checkpoint `c2df50300e488936cb777bf9a543f8c988f4af30`, PR #71, based on main `2b9bd6018fd7d36733602d8bfa0cc4d061e1a4f7`. PR merge-ref `1564142` currently has Renderer 3D Vulkan Smoke **FAIL** at native link with unresolved `VulkanGPUBuffer`, `VulkanDescriptorManager`, and `GPUSkinningPaletteBuffer`; the other named workflows are currently reported PASS. Termux/device is **UNVERIFIED — TERMUX REQUIRED**. XPBD 100K/200K/<5 ms is not benchmark-proven. P0–P4 are all mandatory 100% targets; do not mark any phase 100% from source presence or contract-only evidence.
