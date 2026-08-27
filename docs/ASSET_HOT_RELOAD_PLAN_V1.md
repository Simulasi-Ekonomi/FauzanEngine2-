# Asset Hot Reload Plan V1

`AssetResourceManager` now exposes a bounded candidate/commit seam for hot reload of an already resident, ready asset. `PlanHotReload` records the manager revision, root slot/generation, and up to 64 unleased affected resource targets. Each target binds the current resource generation, pre-reload hash, current registry hash, and reload generation. The affected set contains the resident root and any resident resource root that records it in its acquired dependency closure.

`CommitHotReload` is fail-closed. It first rejects a manager-revision mismatch, malformed root, empty or oversized plan, forged target, leased resource, and any change in resource/reload generation. It then recomputes the plan from current ready registry definitions and requires exact equality before a single resource is updated. A rejected plan does not change resource hashes or generations; a successful commit updates only the validated resource records and advances the manager revision once.

`asset_resource_manager_smoke` passes in Release and in AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`. The smoke proves the two-target texture/dependent-root plan, forged-plan rejection with receipt preservation, stale-plan rejection after lease activity, exact fresh commit, and invalid-input plan-output preservation.

This is CPU-resident, caller-driven resource metadata only. It does not run a file watcher, import bytes, schedule asynchronous work, cancel I/O, migrate GPU objects, notify renderers, define cross-process cache coherence, or claim production hot reload.
