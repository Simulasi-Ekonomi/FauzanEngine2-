# Mesh Import Set V1

## Scope

`MeshImportPipeline::ImportObjSet` imports from one to 32 caller-supplied in-memory OBJ requests into candidate copies of `AssetRegistry` and `MeshStagingStore`. Each request follows the existing OBJ registry import, ready-state, CPU mesh staging, and receipt validation path. Registry, mesh staging, and ordered receipts commit only after all requests succeed.

| Condition | Registry/staging/receipts |
|---|---|
| All bounded OBJ requests stage successfully | All meshes and ordered receipts commit together. |
| Empty set or more than 32 requests | Rejects without mutation. |
| Any duplicate, dependency, ready-state, or OBJ-stage failure | Existing registry, mesh staging, and caller receipt vector remain unchanged. |

The existing single-OBJ `ImportObj` contract remains unchanged.

## Evidence

The extended `mesh_import_pipeline_smoke` proves a two-OBJ commit with ordered receipts, followed by rollback when the second OBJ is malformed. It also verifies an empty set preserves the prior receipt vector.

## Boundary

This is a bounded in-memory CPU OBJ transaction. It does not read files, parse MTL, watch paths, hot reload live renderers, upload GPU meshes, download external assets, or establish production asset-streaming readiness.
