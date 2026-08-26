# Prefab Asset Refresh V1

E.8a extends `AssetRefreshDiagnostics` with `RefreshPrefab`. Its prefab overload receives a `PrefabStagingStore` and appends one action when an affected, staged prefab is no longer current against the ready `AssetRegistry` definition. Existing mesh-only and sprite overloads remain unchanged.

`AssetRefreshExecutor::ExecutePrefabsAtomic` accepts only the bounded `RefreshPrefab` subset. It copies `PrefabStagingStore`, validates the expected ready-asset hash and `CanRefresh` decode/capacity probe, performs refreshes on the candidate, then commits the staged store and receipts only after every action succeeds. On a malformed replacement, the caller store and its prior receipt vectors are retained.

The executor refreshes staged prefab data only. It does not instantiate a prefab, alter a live `EditorSceneSession`, replace existing actor subtrees, observe files, or reload a project. A caller must explicitly invoke `EditorSceneSession::InstantiateStagedPrefab` after confirming the staged resource is current.

