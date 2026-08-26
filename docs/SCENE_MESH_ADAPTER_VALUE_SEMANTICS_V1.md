# SceneMeshAdapter Value Semantics V1

## Scope

`SceneMeshAdapter` stores a copy-on-register `CpuTextureResource` for every textured `SceneMeshInstance`; `MeshMaterial::texture` is therefore an internal pointer that must address that instance's embedded copy. Copy construction, copy assignment, move construction, and move assignment now rebind every textured instance to its own embedded resource after transfer.

| Transfer case | Required result |
|---|---|
| Copy or move a textured adapter | Its material pointer is rebound to its own copied texture. |
| Refresh the source adapter afterward | The transferred adapter retains its prior texture snapshot. |
| Refresh the source adapter itself | It displays only the newly staged texture. |

The change preserves existing source asset/hash checks, staging validation, culling, and CPU renderer behavior. It adds no live sharing or resource-streaming ownership model.

## Evidence

`scene_mesh_adapter_smoke` builds a red textured instance, refreshes its source adapter to blue, then exercises copy construction, move construction, copy assignment, and move assignment. After the original is refreshed to green, the moved and move-assigned adapters still render blue while the source renders green. The same smoke retains its hierarchy, culling, staging, paired-material, rebind, and rejection checks.

## Boundary

This only makes the adapter's internal CPU texture pointers safe across local C++ value transfer. It does not provide GPU resource lifetime management, shared streaming, file watching, hot reload, editor UI, or production renderer readiness.
