# Asset Refresh Executor Atomic V1

## Scope

`AssetRefreshExecutor::ExecuteAtomic` applies an existing bounded `AssetRefreshPlanEntry` sequence to candidate copies of `TextureStagingStore`, `MeshStagingStore`, `MaterialStagingStore`, and `SceneMeshAdapter`. Only a fully successful candidate `Execute` replaces the caller stores, scene adapter, preflight receipts, and execution receipts.

| Outcome | Caller-visible state |
|---|---|
| Candidate plan preflights and executes fully | All refreshed staging resources, scene rebinds, and ordered receipts commit together. |
| Candidate plan is stale, invalid, or fails its probe/action | Live staging/scene state and prior preflight/execution receipts remain unchanged; only `LastError()` reports the failure. |

The existing `Execute` method remains the explicit sequential path with its established receipt semantics.

## Evidence

`asset_refresh_executor_smoke` first refreshes a staged red texture and its scene binding to blue using `Execute`, then performs an atomic blue-to-green refresh. It verifies the rendered pixel is green and the committed receipt set has two entries. It then builds a yellow plan but changes registry bytes to malformed PPM before `ExecuteAtomic`; the candidate fails stale-plan validation, while the live scene retains green and its previously committed receipt/preflight vectors are preserved.

## Boundary

This is a single-threaded, in-memory candidate-copy transaction. It does not provide locking, concurrent mutation safety, file watching, filesystem persistence, GPU upload/lifetime, network propagation, a live game host, or production hot-reload readiness.
