# FauzanEngine — AI / GPT / Agent Engineering Standard

**Status:** Mandatory repository standard  
**Audience:** ChatGPT/GPT, Claude, Gemini, Jules, Devin, Codex, local Termux agents, human maintainers  
**Canonical branch:** `main`  
**Canonical source:** `Source/NeoEngine/`  
**Project type:** production-oriented 3D game engine, Vulkan/Android capable

This document exists so a new AI agent can enter the repository without relying on previous chat history. It is the operational contract for repository inspection, implementation, integration, validation, and handoff.

## 1. Mission

Build FauzanEngine into a genuinely functioning, production-oriented **3D game engine**, not a collection of disconnected demos or source files.

The standard is end-to-end capability:

`ECS ↔ Scene ↔ Gameplay ↔ Physics ↔ Animation ↔ Audio ↔ Renderer/GPU ↔ Vulkan ↔ Android`

with:

`Asset Pipeline → all runtime consumers`  
`Editor → authoring/runtime seams`  
`Networking → authoritative ECS/gameplay/physics state`

A subsystem is not considered complete when its classes merely compile. It is complete when the canonical runtime can use it through the intended integration graph and executable evidence exists.

## 2. Absolute engineering rules

### 2.1 Preserve, then upgrade

- Never delete an existing real function simply because it is inconvenient.
- Never replace a real subsystem with a stub.
- Never remove a meaningful test to hide a failure.
- Never weaken a workload or acceptance threshold to obtain green CI.
- Existing APIs should remain compatible unless a justified additive evolution is required.
- When a design is insufficient for the 3D target, upgrade the design rather than downgrading the target.

### 2.2 Main is protected

- Never make direct implementation edits on `main`.
- Every workstream gets its own branch based on the latest relevant `main`.
- One coherent workstream per branch/PR.
- Re-check `main` after another PR merges before continuing dependent work.

### 2.3 Historical branches are not automatically authoritative

A historical branch may be:

- already fully represented in `main`;
- partially represented;
- superseded;
- stale;
- a helper/synchronization branch;
- an evaluation-only artifact;
- a closed/unmerged source of still-valid defects.

Therefore:

> **Inspect the implementation and history first; do not blindly merge an old branch.**

If #44 or another old branch contains useful functionality that is already present through a repair lineage, repair the current `main` implementation rather than duplicating stale commits.

## 3. Required first action for every new agent

Before changing code:

1. Read `AGENTS.md`.
2. Read this document.
3. Determine the current `main` SHA.
4. Inspect the relevant current source and CMake registration.
5. Search open and closed PRs for the subsystem.
6. Read relevant PR diffs and review comments.
7. Check CI status for the relevant commits.
8. Compare relevant branches with `main`.
9. Identify whether the requested functionality already exists on `main`.
10. Write a short internal work plan before editing.

Never begin from a remembered chat summary when the repository itself can answer the question.

## 4. Branch and PR audit procedure

For every outstanding historical branch/PR, classify it:

| Class | Meaning | Action |
|---|---|---|
| Merged-valid | Functionality is on `main` and no known defect remains | Do not duplicate |
| Merged-follow-up | Functionality is on `main` but review/CI exposed defects | Repair current `main` |
| Stale/superseded | Old architecture or far behind `main` | Extract valid ideas/defects only |
| Helper/sync | Rebase/synchronization only | Do not treat as product feature |
| Closed-unmerged | Valuable work never merged | Port verified functionality or defects to current `main` |
| Open | Active implementation | Review, repair, validate, then merge |

The goal is **zero unresolved product-relevant work**, not zero historical branch names.

## 5. Defect audit checklist

Every relevant implementation must be checked for:

### Correctness
- invalid input;
- invalid enum/state;
- integer overflow/underflow;
- float NaN/Inf;
- bounds validation;
- stale handles/generations;
- partial mutation;
- transactional failure semantics;
- serialization corruption/truncation;
- compatibility with existing data.

### Memory and lifetime
- ownership is explicit;
- RAII is used where applicable;
- no double free;
- no use-after-free;
- no callback racing object destruction;
- pending GPU upload tasks must not outlive the `AssetResourceManager` they reference;
- uploader teardown must occur before destruction/reset of the referenced resource manager;
- release/eviction/hot-reload must be blocked while an upload pin is active;
- GPU resources are released at the correct lifetime;
- failed allocations leave state unchanged where the API promises failure safety.

### Concurrency
- producer/consumer publication is synchronized;
- counters are updated only after successful insertion;
- shutdown drains or safely invalidates in-flight work;
- callbacks cannot access destroyed state;
- global state is not accidentally shared across independent actors/players.

### Build/integration
- production source is in canonical CMake;
- smoke executable is registered;
- required shader/assets are generated or deployed;
- runtime ownership is correct;
- no accidental dependency on a legacy path;
- public APIs remain connected to their actual implementation;
- Vulkan helper functions must fail closed on command recording/submission/transition errors;

## 6. 3D-first renderer rule

FauzanEngine is intended to become a real 3D engine.

If the canonical runtime currently routes scene rendering primarily through `SoftwareRenderer`, that is an integration gap, not a reason to declare the 3D renderer complete.

The intended evolution is:

`SceneWorld/ECS → SceneMeshAdapter → Vulkan3DRenderer/PBR/GPU path → Vulkan presentation`

while retaining software rendering only when it has a legitimate role such as:

- deterministic fallback;
- headless/test path;
- UI/HUD compatibility;
- environments without Vulkan.

Do not delete the software path solely to make the architecture look cleaner. Instead establish a real Vulkan 3D primary path and make fallback selection explicit.

## 7. PBR and Vulkan requirements

The repository already contains substantial PBR/Vulkan work. Agents must treat it as real code requiring integration, not as decorative source.

When touching Vulkan:

- validate descriptor-set contracts;
- validate shader interface compatibility;
- validate shader runtime deployment paths;
- validate image layout transitions;
- reject unsupported image-layout transition pairs rather than executing an implicit generic barrier;
- propagate Vulkan command-buffer, queue-submit, and queue-idle failures instead of returning synthetic success;
- validate resource ownership and cleanup;
- preserve persistent `VulkanContext` lifetime where required;
- validate SDL ownership so renderer teardown cannot unexpectedly terminate audio/input;
- validate software-ICD/Xvfb smoke where CI supports it.

For HDR/IBL environment resources, preserve the intended 6-layer cube-compatible image and mip-chain semantics. Do not downgrade formats or remove mip levels simply to make a test pass.

## 8. XPBD performance contract

The performance goal is an engineering acceptance contract:

- **100,000 bodies**;
- **at least 200,000 collision tests**;
- measured physics step **strictly `< 5 ms`**.

A benchmark is invalid if it uses a smaller workload to pass.

A gate is invalid if exactly `5,000 µs` passes.

Do not replace the real solver with a benchmark-only fake path. The benchmark must exercise the production XPBD implementation.

Canonical integration must eventually connect:

`ECS/Gameplay → authoritative XPBD state → SceneWorld transform/pose → Renderer`

without creating competing transform authorities.

## 9. Asset pipeline requirements

Asset streaming is not complete when metadata moves through a queue, when a command buffer is merely recorded/submitted, or when a state enum is advanced by a synthetic callback. The current P1 contract requires a resource upload pin, exact resource-handle association with the pending GPU task, authoritative fence/timeline observation, and only then resource-manager residency publication. File-stream completion callbacks must not mutate those canonical runtime stores from worker threads; a synchronized runtime-thread handoff must perform the state-machine transitions.

Validate:

- bounded queue behavior;
- Pending → Uploading → Ready/Failed lifecycle;
- resident-byte accounting;
- GPU allocation ownership;
- synchronization-handle ownership and duplicate-registration safety;
- eviction cleanup;
- LRU correctness;
- total resident budget enforcement;
- hot reload and dependency closure;
- renderer/asset-manager ownership boundaries.

Known historical concern: eviction and resident-budget logic must not lose GPU ownership or erase metadata while GPU resources remain alive.

## 10. ECS / Scene / Gameplay authority

Avoid duplicate authoritative state.

The desired direction is:

- ECS owns authoritative gameplay component state;
- SceneWorld provides scene/entity transform representation;
- physics owns authoritative physical state for physics-controlled bodies;
- animation modifies only the explicitly authorized animation/pose path;
- renderer consumes state and does not become an accidental gameplay authority.

When adding a bridge, identify exactly which direction data flows and who owns the final write.

## 11. Animation

Animation work must progress beyond isolated clip/state utilities.

Eventually validate:

- animation state machine;
- clip/blend evaluation;
- skeleton/pose evaluation;
- root motion authority;
- Scene/ECS transform integration;
- renderer skinning path;
- GPU skinning where performance requires it.

Do not claim animation production readiness from CPU-only utility coverage.

## 12. Audio

Audio work must preserve:

- non-spatial stereo behavior;
- spatial attenuation;
- listener-relative panning;
- looping;
- callback lifetime safety;
- deterministic failure behavior.

SDL audio stream teardown must not race a callback or terminate unrelated SDL subsystems.

## 13. Android

Android work must be treated as a production platform path, not merely a header abstraction.

Validate:

- lifecycle state transitions;
- native window lifetime;
- input queue preservation;
- AInputQueue/Looper glue where the final application path requires it;
- rendering suspension/resume;
- audio behavior across pause/stop/destroy;
- frame scheduling;
- packaging/deployment.

A lossless internal injection/polling seam is useful but is not by itself proof of complete NDK input integration.

## 14. Networking

Networking must ultimately feed authoritative ECS/gameplay/physics state.

Validate:

- sequence/idempotency;
- bounded command admission;
- malformed payload rejection;
- deterministic authority;
- replay/conflict handling;
- state publication only after successful mutation;
- disconnect/reconnect behavior.

## 15. Editor and AI-agent integration

Editor functionality must be useful against the actual runtime scene model.

Validate:

- actor creation/mutation/duplication;
- naming;
- selection;
- serialization compatibility;
- schema validation;
- safe property mutation;
- AI-agent JSON scene automation against the same canonical scene representation.

Do not maintain a second disconnected scene format merely for the editor agent.

## 16. Test hierarchy

Use the strongest applicable evidence:

1. GitHub Actions on the exact PR commit;
2. exact branch Release build + executable smoke;
3. ASan + UBSan + leak checks;
4. Vulkan software-ICD/Xvfb smoke;
5. Android/device smoke;
6. real performance benchmark.

A source review is useful but is not equivalent to runtime execution.

When a layer cannot be tested in the current environment, say so explicitly.

For the active P1 asset-uploader path, the minimum executable evidence is a real Vulkan smoke that proves: upload remains non-resident before the fence, submission is not completion, completion publishes residency, the exact resource handle is preserved, and borrowed synchronization handles remain owned by the caller.

## 17. Review and merge gate

Do not merge merely because the PR is mergeable.

Before merge verify:

- implementation is on the current branch base;
- no unresolved blocking review defect;
- CI applicable to the work is green;
- tests exercise the real implementation;
- async GPU completion is observed through the authoritative synchronization mechanism before Ready publication;
- no existing capability was removed;
- CMake integration is present;
- runtime integration is present;
- PR description accurately states validation and limitations.

If any of these are unknown, keep the PR open.

For asynchronous asset work, also verify that the resource manager rejects release/eviction/hot-reload while an upload pin is active, permits release after the final successful completion, and clears the pin on cancellation/failure.

## 18. Documentation and handoff

Every agent must leave a durable record:

- branch name;
- base SHA;
- current head SHA;
- PR number;
- files changed;
- defect list;
- architectural decision;
- tests actually executed;
- CI evidence;
- known limitations;
- explicit owner/lifetime order for asynchronous observers or resource managers;
- next blocking task.

Do not force the next agent to recover this information from chat history.

## 19. Current pre-P0/P1/P2/P3 gate

Before new P0–P3 expansion, the repository must first be brought to a coherent integrated state.

### Outstanding repair/integration sequence

1. Finish PR #64 and any remaining #44/#48/#63/#8 defect lineage.
2. Audit and repair the P6 Vulkan shader deployment/runtime path.
3. Audit SDL ownership and teardown across renderer/audio/input.
4. Upgrade the canonical Scene/ECS render route from software-primary to real Vulkan 3D primary with explicit fallback.
5. Connect SceneWorld mesh extraction to Vulkan3DRenderer/PBR/GPU rendering.
6. Integrate XPBD V5 into the canonical runtime and authoritative gameplay/scene path.
7. Repair AssetStreamingQueue GPU ownership and total resident-budget accounting.
8. Complete animation-to-scene/render integration.
9. Complete networking-to-authoritative-ECS/physics integration.
10. Complete Android native input/render lifecycle integration.
11. Re-audit all active branches and open PRs; no product-relevant branch may remain unexplained.
12. Only after the above is coherent should the planned P0–P3 work continue.

## 20. Definition of done

A feature is **DONE** only when all applicable statements are true:

- real implementation exists;
- existing functionality was preserved or deliberately upgraded;
- canonical CMake includes it;
- runtime ownership/integration is established;
- regression/smoke coverage exists;
- applicable CI is green;
- sanitizer evidence exists where relevant;
- platform/Vulkan evidence exists where relevant;
- performance evidence exists where required;
- review defects are resolved;
- documentation/handoff is updated.

A feature may be marked **PARTIAL** when the source is real but one or more integration/validation layers remain. Do not convert PARTIAL into DONE by wording.

## 21. Agent command style

Agents should communicate status in this compact format:

```text
WORKSTREAM: <name>
BRANCH: <branch>
BASE: <sha>
HEAD: <sha>
STATUS: <AUDIT | IMPLEMENTING | VALIDATING | BLOCKED | READY_TO_MERGE | MERGED>
DEFECTS: <count/list>
CHANGES: <files/areas>
TESTS: <actual evidence only>
CI: <actual evidence only>
UNVERIFIED: <explicit gaps>
NEXT: <single next blocking action>
```

This format is intentionally factual. Do not substitute confidence or optimism for evidence.

## 22. Final principle

> **The objective is not to make the repository look complete. The objective is to make the engine actually work.**

When a test conflicts with the intended production capability, repair the implementation or the test contract correctly. Never lower the engine's target just to obtain a green check.
