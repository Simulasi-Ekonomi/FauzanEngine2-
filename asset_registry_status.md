# Asset Registry Status

`AssetRegistry` is the active bounded metadata registry for canonical runtime assets. It validates safe IDs, rejects duplicate declarations, requires dependency declaration before a dependent asset can exist, and permits `Ready` state only after all direct dependencies are ready.

```text
ASSET_REGISTRY_SMOKE_OK assets=2 deterministic=1
```

Release and AddressSanitizer smoke runs pass. This registry does not pretend to import meshes, textures, prefabs, scenes, or audio bytes. Import, hashing, source/dependency scanning, hot reload, and GPU upload remain separate work items and must integrate with a validated renderer backend.
