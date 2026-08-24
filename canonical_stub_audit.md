# Canonical Stub Audit

## Scope

The active CMake target compiles the bounded core introduced in this work: ECS/XPBD/job system, Farm runtime and telemetry adapter, typed agent gateway, template registry/Sudoku, `NeoRuntime`, renderer capability probe, and asset registry. It does **not** compile the broad legacy `Rendering`, `AI`, editor, or Android-native source trees into the Linux canonical target.

## Evidence-based classification

| Category | Files observed | Status and action |
|---|---|---|
| Active canonical runtime | `Runtime/*`, `Systems/Farm*`, `Agents/AgentCommandGateway*`, `Templates/*`, `Physics/V5/*` listed by canonical CMake. | Built and smoke-tested; capability boundaries are explicit. |
| Graphical renderer blocker | `Rendering/RHI/Vulkan/VulkanRHI.cpp`, OpenGL/renderer paths, FrameGraph comments. | Not part of active target. `RendererCapabilityProbe` reports `NotImplemented`; graphical readiness is blocked. |
| Legacy AI generator | `AI/OpenCodeIntegration.cpp` includes generated TODO fallback and localhost calls. | Not part of active target; Coba/Aries use the bounded typed gateway instead. |
| Legacy ECS/editor placeholders | `Core/ECS/*Editor*`, `ChunkedSystems.h`, `ECSGPUBridge.cpp`, `ECS/V4/*`. | Not part of active target. They must be deleted, migrated, or made explicitly unsupported before any broad-source Android build. |
| Legacy streaming/economy placeholder | `Streaming/StreamManager.h`, `Systems/ItemSerialTracker.cpp`. | Not part of active target. Runtime economy authority remains in FarmSystem’s bounded ledger; do not wire legacy code into production. |
| Legacy lifecycle stub | `Core/EngineLoop.cpp`. | Now compiled and fail-closed with an explicit `NOT_IMPLEMENTED` logic error; `NeoRuntime` remains the canonical state lifecycle. |

## Priority order

The next production blockers are a validated renderer backend and asset import/upload pipeline, followed by Android JNI rewiring to the canonical subset and then individual executable templates. The audit deliberately does not mark broad legacy directories as production-ready merely because files are present.
