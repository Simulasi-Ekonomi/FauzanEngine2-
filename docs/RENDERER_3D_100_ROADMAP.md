# Renderer 3D 100% — Engineering Roadmap

Branch: `feature/renderer-3d-100`

## Non-negotiable contract

- Existing `SoftwareRenderer`, `MeshRenderer`, camera, scene adapters, texture staging and current smoke contracts remain supported.
- No existing rendering feature is removed or silently changed.
- A milestone is not marked complete from source presence alone; it requires Release + ASAN evidence and, where applicable, hardware/runtime evidence.
- The final 100% claim is blocked until every gate below has executable evidence.

## Gates

| Gate | Target | Status |
|---|---|---|
| R0 | Preserve current CPU renderer and 124/124 regression baseline | baseline available |
| R1 | Vulkan instance/device/queue + SDL surface | existing probe; production integration in progress |
| R2 | Real swapchain, resize, acquire/submit/present and synchronization | implementation in progress |
| R3 | GPU mesh/index/instance buffers and scalable batching | open |
| R4 | Full camera matrices + scene-wide frustum/occlusion culling | open |
| R5 | Depth/stencil, robust clipping and deterministic render ordering | open |
| R6 | PBR metallic/roughness + normal/occlusion/emissive + samplers | open |
| R7 | Directional/point/spot lights + shadow maps/cascades | open |
| R8 | HDR, linear/sRGB correctness, tone mapping | open |
| R9 | Render graph + opaque/transparent/post-process passes | open |
| R10 | Skeletal animation + GPU skinning + validated animation sampling | open |
| R11 | IBL/environment lighting + sky/atmosphere hooks | open |
| R12 | Editor/authoring/asset streaming integration | open |
| R13 | Profiling, GPU timestamps, workload benchmarks and target-device evidence | open |
| R14 | Full regression: Release + ASAN + renderer hardware gates | open |

## Current implementation note

`Vulkan3DRenderer` is being introduced as a dedicated GPU path rather than replacing the bounded CPU path. It is deliberately kept off the canonical CMake source list until the first executable smoke test has been added and the implementation has passed compilation and runtime validation. This prevents an unvalidated GPU increment from destabilizing the existing 124/124 Release and ASAN baseline.

## Definition of 100%

100% means all R1–R14 gates are implemented, integrated, tested and evidenced. It does not mean “looks similar to Unreal”; it means the 3D renderer has a complete production-oriented feature and verification surface for the engine's declared scope.
