# FauzanEngine2- — P0–P4 Master Roadmap

**Status date:** 2026-09-19  
**Branch:** `p3-editor-android-production-night`  
**PR:** #71  
**Code:** `c2df50300e488936cb777bf9a543f8c988f4af30`  
**Main:** `2b9bd6018fd7d36733602d8bfa0cc4d061e1a4f7`

> Historical R1–R6 plans are historical records, not current completion certificates.

## Master rule
**P0, P1, P2, P3 and P4 must all reach 100%.**

100% requires real implementation, canonical integration, CMake/build closure, regression tests, CI, runtime/device validation, sanitizer evidence, benchmark evidence where applicable, packaging and release gates.

## Current progress
| Phase | Current | Evidence |
|---|---:|---|
| P0 — Canonical Runtime & ECS | 85% | IMPLEMENTED-UNVERIFIED for runtime/device layers |
| P1 — Renderer + Asset + Animation | 30% | IMPLEMENTED-UNVERIFIED; current Vulkan link closure blocks full smoke |
| P2 — Physics + Gameplay + Networking | 20% | CONTRACT-ONLY for 100K/200K/<5 ms |
| P3 — Editor + Audio + Android + Services | 15% | Mixed foundation; device/release gates open |
| P4 — Final Production Certification | 0% | Not started |

## P0 — 100% target
Entity/component lifecycle; archetype/chunk storage; deterministic Scene↔ECS; authoritative ECS↔Physics; ECS↔Animation; ECS↔Renderer; asset/resource lifetime; frame scheduling/shutdown/failure; regression evidence.

## P1 — 100% target
Production Vulkan 3D primary path; GPU-driven visibility/culling/indirect rendering; PBR/IBL/HDR/lighting/shadows/reflections; descriptor/render-graph synchronization; swapchain/device-loss recovery; importer→decoder→staging→GPU upload→LOD/cooking→packaging; GPU ownership/eviction/full async hot reload; multi-instance GPU skinning/palette batching/animation LOD; end-to-end Scene→GPU proof.

## P2 — 100% target
Production XPBD correctness/determinism; **100,000 bodies + at least 200,000 collision tests + measured step strictly <5 ms**; authoritative gameplay; replication; prediction/reconciliation/interpolation; interest management; snapshot compression; reconnect; authority recovery; stress/capacity/security.

## P3 — 100% target
Canonical editor/PIE/authoring; asset/material/animation/physics authoring; production audio; Android input/render/audio lifecycle and device matrix; reproducible APK/AAB packaging/signing; persistence; crash reporting; telemetry; security; commerce; live ops.

## P4 — 100% target
Release/ASAN/UBSAN/leak/stress/long-run; Vulkan device/driver matrix; device loss/recovery; suspend/resume; reconnect; GPU/asset failure recovery; reproducible builds/signing/versioning/save migration; crash recovery/rollback; final performance/capacity/security/accessibility/release gates; zero blocking review defects.

## Current CI gate
PR #71 merge-ref `1564142`:
- 8 named workflows PASS.
- `Renderer 3D Vulkan Smoke` FAILS during native link.
- Unresolved symbols: `VulkanGPUBuffer`, `VulkanDescriptorManager`, `GPUSkinningPaletteBuffer`.
- `c2df503` added `Runtime/VulkanGPUBuffer.cpp`, but complete Vulkan/animation linkage remains unresolved.
- Termux/device: **UNVERIFIED — TERMUX REQUIRED**.
- XPBD target: **not benchmark-proven**.

## Execution order
1. Repair Vulkan CMake/link closure.
2. Rerun exact CI.
3. Complete P1 animation batching/LOD and asset import/upload/hot-reload.
4. Close P0/P2 authoritative integration and obtain the real XPBD benchmark.
5. Close P3 editor/audio/Android/services and device/release evidence.
6. Execute P4 certification.
7. Mark P0–P4 100% only after applicable gates pass.

`main` remains protected.
