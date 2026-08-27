# Asset Resource Eviction Plan V1

## Scope

`AssetResourceManager` now separates a bounded resource-memory decision into `PlanEviction` and `CommitEviction`. The plan is a fixed-capacity value object containing the manager revision, exact resident bytes before/after, active-resource count, budget, and ascending-slot unleased victims. Planning makes no resource or lease mutation and does not replace the caller’s plan on failure.

Commit recomputes the canonical plan at the recorded budget and requires exact equality before releasing any slot. A manager revision mismatch rejects the plan as stale. This gives an owner a cancellation point between inspection and mutation, while rejecting forged, altered, or outdated plans without releasing a resource.

## Evidence

`asset_resource_manager_smoke` passes in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`. In addition to its existing lease, hot-reload, capacity, and direct-budget cases, it proves these plan/commit properties:

| Case | Required result |
|---|---|
| Budget plan for three unleased resources | Deterministically selects ascending-slot 3-byte then 2-byte victims to leave exactly 2 resident bytes. |
| Altered plan | Commit rejects it with no resident-byte or active-resource mutation. |
| Lease acquired after planning | Commit rejects the former plan as stale and leaves all resources resident. |
| Budget blocked by lease | Planning fails without replacing the caller’s prior plan or mutating resource state. |
| Fresh plan then commit | The exact planned victims are evicted, leaving the planned resident-byte total. |

## Boundary

This is a single-process CPU resource ownership contract. It does not add asynchronous streaming, file watching, I/O cancellation, GPU allocation/release, concurrent synchronization, allocator pressure integration, telemetry, a general memory manager, or production streaming guarantees. The legacy `Streaming/StreamManager.h` remains outside the active canonical CMake source list and is not activated by this increment.
