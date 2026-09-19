# AI HANDOVER PROTOCOL — FauzanEngine2-

## Purpose
This file is the canonical operating protocol for ChatGPT/Claude/other coding rooms continuing work on this repository. A new room must read this file and `BRANCH_STATUS.md` before editing code.

## Canonical repository
- Repository: `Simulasi-Ekonomi/FauzanEngine2-`
- Canonical source: `Source/NeoEngine`
- Canonical local Termux path: `~/FauzanEngine2-`
- Main is the protected baseline. Do not merge to main unless the user explicitly authorizes it.
- Current active work branch: `p3-editor-android-production-night`
- Current PR: #71 — `WIP: P0-P3 canonical runtime, renderer, physics, Android integration`
- Current code checkpoint: `c2df50300e488936cb777bf9a543f8c988f4af30`
- Main baseline: `2b9bd6018fd7d36733602d8bfa0cc4d061e1a4f7`

## Non-negotiable engineering rules
1. READ before EDIT. Inspect the exact current file/anchor and call sites first.
2. Preserve existing working functionality. Upgrade; do not downgrade.
3. Never remove original functions merely to make a build pass.
4. No mass stubs, fake implementations, placeholder production code, or parallel duplicate architectures.
5. Do not invent paths, APIs, test results, benchmarks, or integration status.
6. Do not overwrite a file blindly. Use the current SHA/content and make an exact change.
7. Keep `main` untouched during branch work.
8. Commit/push only to the active work branch unless the user explicitly says otherwise.
9. CI success is not the same as Termux/device/runtime proof.
10. If Termux/device execution has not happened, state `UNVERIFIED — TERMUX REQUIRED`.
11. Required workflow: READ → ANALYZE → EDIT → BUILD/CI → TEST → BENCHMARK → REVIEW → APPROVE → COMMIT → PUSH.
12. When a user says `Lanjut`, continue the current roadmap autonomously unless a real blocker requires input.
13. Never claim a target is achieved from an acceptance contract alone.
14. For performance targets, record measured evidence separately from API/contract existence.
15. Do not merge just because CI passes; preserve the user's branch/verification gate.

## Architecture rule
Use the existing active architecture before creating anything new.

Canonical direction:
`SceneWorld → SceneECSBridge → ArchetypeManager/ECS → Physics/Animation/Renderer`

For rendering:
`Scene → ECS identity/transform → SceneRenderAdapter → Vulkan/Software renderer`

For mesh/material:
- `SceneMeshAdapter` currently owns CPU mesh/material instance data and source asset identity.
- `ArchetypeManager::meshID` exists but is NOT yet authoritative on the active renderer path.
- Do not invent a mesh-ID registry or replace `SceneMeshAdapter` until the existing asset/resource contracts are audited.
- A gap must be documented as a gap, not hidden by fallback behavior.

## Roadmap
### P0 — Canonical Runtime & ECS
Target: canonical entity/component lifecycle, archetype/chunk storage, Scene↔ECS, ECS↔Physics, ECS↔Animation, ECS↔Renderer, asset handles, frame lifecycle, render extraction, transforms, deterministic sync/ownership.

### P1 — Production Renderer + Asset + Animation
Target: Vulkan production path, GPU-driven rendering, culling, indirect draw, materials/PBR/IBL/shadows/reflections, asset streaming/GPU lifetime, descriptors/render graph/device loss, GPU skinning, animation batching/LOD, production asset pipeline.

### P2 — Physics + Gameplay + Networking
Target: XPBD 100k bodies / 200k collision tests / strictly <5 ms, plus gameplay/networking integration, prediction/reconciliation/interpolation/interest management/reconnect/determinism.

### P3 — Editor + Audio + Android + Production Services
Target: editor/PIE/asset/material/animation/physics authoring, audio production path, Android/device/release pipeline, persistence/telemetry/security/commerce/live ops.

### P4 — Final Production Certification
Target: performance, ASAN/UBSAN/stress/long-run/device-loss/suspend-resume/reconnect, compatibility matrix, reproducible packaging/signing/versioning/migration/recovery.

## Percentage policy
Every active work branch MUST contain `BRANCH_STATUS.md`.

Percentages are engineering progress indicators, not marketing claims:
- `P0/P1/P2/P3/P4` percentages describe completion of the roadmap checklist.
- Overall branch percentage is a weighted work-progress estimate.
- Never increase a percentage merely because code was written. Prefer verified behavior/CI/Termux evidence.
- If a feature is implemented but not runtime-tested, mark it `implemented / unverified`, not fully verified.
- If a target is only an acceptance contract, mark it `contract-only`.

## Room-to-room continuation protocol
When a room reaches a context/tool limit:
1. Commit all completed branch work.
2. Update `BRANCH_STATUS.md` with:
   - branch name
   - HEAD commit
   - PR
   - main baseline
   - overall %
   - P0–P4 %
   - exact files changed
   - what was verified
   - what is unverified
   - current blocker/gap
   - exact next action
3. Update this protocol only when the working method itself changes.
4. Do NOT summarize from memory if the repository contains newer evidence.
5. The next room must re-read the two MD files and then inspect the exact current code before continuing.
6. Never assume an old room's "next step" is still valid if the branch HEAD changed; re-audit the current HEAD.
7. Keep branch status atomic: one current checkpoint, not a long ambiguous narrative.

## Verification labels
- `VERIFIED-CI`: GitHub workflow passed on the stated commit.
- `VERIFIED-TERMUX`: user/device Termux execution passed on the stated commit.
- `VERIFIED-BENCH`: measured benchmark passed with workload and timing recorded.
- `IMPLEMENTED-UNVERIFIED`: code exists but runtime/device evidence is missing.
- `CONTRACT-ONLY`: acceptance API/test exists but target performance/behavior is not proven.
- `BLOCKED`: concrete dependency prevents safe continuation.

## Current known truth at this checkpoint
- P0 canonical Scene↔ECS runtime path is substantially implemented.
- Scene transform, rotation, and scale are ECS-authoritative for the active Vulkan 3D runtime path when ECS is supplied.
- Current Vulkan runtime call supplies `m_ECS.get()` and `&m_SceneECSBridge`.
- XPBD timing instrumentation exists, but the 100k/200k/<5 ms target is not proven.
- GPU skinning palette buffer/shader foundation exists, but full pipeline binding/runtime proof is not complete.
- `ECSGPUBridge::DispatchCompute()` is legacy/non-active; do not turn it into a fake parallel GPU architecture without an active integration point.
- Current CI checkpoint `140449a4` passed the listed repository workflows; Termux/device runtime remains separate evidence.
\n\n## CURRENT P0-P4 STATUS — 2026-09-19\nP0–P4 are all mandatory 100% targets. Current branch remains incomplete. PR #71 merge-ref `1564142` has Renderer 3D Vulkan Smoke FAIL at native link with unresolved VulkanGPUBuffer/VulkanDescriptorManager/GPUSkinningPaletteBuffer. Termux/device is UNVERIFIED — TERMUX REQUIRED. XPBD 100K/200K/<5 ms is not benchmark-proven.\n