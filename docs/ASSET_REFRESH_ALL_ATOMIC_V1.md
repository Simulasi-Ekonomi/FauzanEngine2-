# Asset Refresh All Atomic V1

E.8b adds a `BuildPlan` overload that accepts both `SceneSpriteAdapter` and `PrefabStagingStore`. For each deterministic dependency ID, it preserves existing resource actions, appends stale sprite actions in immutable adapter insertion order, then appends a stale staged-prefab action when present.

`AssetRefreshExecutor::ExecuteAllAtomic` copies texture, mesh, material, scene-mesh, sprite, and prefab stores. It runs dependency-aware resource actions, then sprite actions, then prefab actions on that shared candidate set. Only total success commits all stores and plan-order receipt vectors. A later malformed prefab therefore rolls back earlier candidate texture and sprite refreshes.

No `EditorSceneSession` is accepted or mutated. This remains CPU/in-memory orchestration, without watchers, automatic re-instantiation, GPU upload, editor UI, asset streaming, or production hot reload.

