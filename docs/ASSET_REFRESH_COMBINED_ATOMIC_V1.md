# Asset Refresh Combined Atomic V1

E.7d adds `AssetRefreshExecutor::ExecuteCombinedAtomic` for a bounded diagnostics plan containing `RefreshTexture`, `RefreshMesh`, `RefreshMaterial`, `RebindSceneInstance`, and `RefreshSpriteInstance`. It preserves `ExecuteAtomic` and `ExecuteSpritesAtomic` as separate compatibility APIs.

The combined executor copies `TextureStagingStore`, `MeshStagingStore`, `MaterialStagingStore`, `SceneMeshAdapter`, and `SceneSpriteAdapter` before work begins. It validates and executes non-sprite actions as one dependency-aware resource group, then validates and executes sprite actions against the same candidate texture store. This keeps an existing `RefreshTexture` action visible to its following scene-mesh rebind during preflight, while allowing multiple sprite bindings to consume the refreshed candidate texture.

Only after both groups succeed are all five caller-owned stores plus complete plan-order preflight/execution receipts committed. A failure in the later sprite group leaves caller stores and prior receipts unchanged. The extended smoke covers a texture refresh followed by one scene-mesh rebind and two sprite refreshes, then a second fixture in which a later source rectangle rejects the replacement and proves full rollback.

This is bounded CPU/in-memory refresh orchestration. It does not add file watching, automatic registry reload, GPU resource upload, editor UI, project management, asset streaming, concurrency guarantees, or production hot reload.
