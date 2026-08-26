# Texture Import Set V1

## Scope

`TextureImportPipeline::ImportSet` imports from one to 32 caller-supplied in-memory texture requests into candidate copies of `AssetRegistry` and `TextureStagingStore`. Each request follows the existing registry-import, ready-state, decoder, and CPU staging path. Only when every request succeeds do the registry, staging store, and receipt vector commit together.

| Condition | Registry/staging/receipts |
|---|---|
| All bounded requests decode and stage | All requested textures and ordered receipts commit together. |
| Empty set or more than 32 requests | Rejects before mutation. |
| Any duplicate, dependency, registry-ready, or decode/stage failure | All caller state and prior receipt vector remain unchanged. |

The existing single-request `Import` behavior is unchanged.

## Evidence

The extended `texture_import_pipeline_smoke` imports a two-texture PPM set and proves two ordered receipts plus two registry/staging additions. It then attempts a two-request set whose second PPM is malformed and verifies asset count, staged count, and prior receipt vector are preserved; an empty set is also rejected without receipt mutation.

## Boundary

This is a bounded in-memory CPU import transaction. It does not read files, watch paths, hot reload live renderers, upload GPU textures, download external assets, or establish production asset-streaming readiness.
