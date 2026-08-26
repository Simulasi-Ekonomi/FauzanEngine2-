# Editor Scene Mesh Binder Atomic V1

## Scope

`EditorSceneMeshBinder::Bind` and `BindDocumentAssets` now stage mesh, named MTL material, and optional PPM/BMP texture resources into candidate stores. The resulting `SceneMeshAdapter` is also candidate-only. The caller's stores and target are replaced only after every eligible authoring mesh actor has staged and bound successfully.

| Result | Caller state |
|---|---|
| All document mesh actors stage and bind | Mesh, material, texture stores and target adapter commit together. |
| A later actor lacks/invalidates a required staged resource | No earlier candidate staging reaches the caller and the target adapter remains unchanged. |

The existing document, registry-ready checks, capacity rules, and actor/entity mapping stay authoritative.

## Evidence

`editor_scene_mesh_binder_smoke` adds a two-mesh document. The first actor has valid mesh/material/texture data, while the second references a ready but malformed MTL material. The binder rejects the second actor with `MaterialStageFailed`; all three caller staging stores remain empty and the pre-existing target keeps its original textured instance.

## Boundary

This is an in-memory authoring-to-CPU-scene binding transaction. It does not import from paths, persist documents, create editor UI, own runtime world lifecycle, upload GPU resources, or establish production scene-loading readiness.
