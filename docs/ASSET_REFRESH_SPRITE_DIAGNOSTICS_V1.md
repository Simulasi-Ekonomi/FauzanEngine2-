# Asset Refresh Sprite Diagnostics V1

E.7c adds a bounded CPU-side diagnostic extension for `SceneSpriteAdapter`. `BindingSnapshots()` returns value-owned records containing only an entity, source asset ID, and source hash. The records follow adapter insertion order and do not expose mutable textures, source rectangles, or instance pointers.

`AssetRefreshDiagnostics::BuildPlan` retains its existing mesh-only overload. Its new overload accepts a `SceneSpriteAdapter` and appends one `RefreshSpriteInstance` action for each stale binding of an affected staged texture. Existing texture, mesh, material, and scene-mesh actions retain their prior order; sprite actions are appended in snapshot order after those actions for the same affected asset. A stale staged texture still produces its one existing `RefreshTexture` action before its sprite actions.

`AssetRefreshExecutor::ExecuteSpritesAtomic` accepts the bounded texture-plus-multiple-sprite subset. Duplicate sprite actions are identified by entity, so distinct sprites sharing one texture remain valid. The executor validates and runs against candidate copies, then commits texture staging, sprite bindings, preflight receipts, and execution receipts only after every action succeeds. A later source-rectangle rejection leaves caller-owned stores and prior receipts unchanged.

When the diagnostics overload also emits existing scene-mesh actions, use `AssetRefreshExecutor::ExecuteCombinedAtomic` rather than either subset executor. The combined contract is documented in `ASSET_REFRESH_COMBINED_ATOMIC_V1.md`.

This scope is diagnostics and manual executor invocation only. It does not add a filesystem watcher, automatic registry reload, GPU texture upload, editor UI, project hot reload, or production asset-streaming claim.
