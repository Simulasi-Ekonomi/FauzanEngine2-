# Farm Runtime Session World Checkpoint V1

## Scope

`FarmRuntimeSession` provides a second, explicit caller-driven in-memory checkpoint handoff for the paired Farm and `FarmWorldTool` state. It wraps the canonical `FarmWorldTool::Serialize` payload in `RuntimeSaveCodec` using the exact kind `farm-runtime-world` and a nonzero caller revision.

| API | Success | Rejection boundary |
|---|---|---|
| `SaveWorldCheckpoint(revision, bytes)` | Replaces output bytes with a validated world envelope. | Leaves output bytes, Farm/world state, committed receipt, renderer, and frame count unchanged. |
| `RestoreWorldCheckpoint(bytes, revision)` | Restores the paired Farm/world payload and returns the envelope revision. | Leaves Farm/world payload, output revision, committed receipt, renderer, and frame count unchanged on outer envelope, kind, revision, or inner-world decoding failure. |

The restore path delegates to `FarmWorldTool::Deserialize`, which parses its complete payload before restoring the paired Farm and committing world fields. This checkpoint operation does not tick simulation, accept input, modify rendering, or produce a new receipt.

## Evidence

`farm_runtime_session_world_checkpoint_smoke` commits one frame, snapshots a tilled Farm and a level-3 player world state at revision 11, mutates both Farm and player state, then restores them. It verifies that the committed receipt/frame count and software framebuffer hash are unchanged across restoration. It separately proves preservation for wrong-kind, checksum-corrupted, and syntactically valid but inner-world-corrupt envelopes, plus a zero-revision save request.

The smoke must pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`, followed by the broad non-Vulkan suite.

## Boundary

This is an in-memory, finite, caller-invoked save boundary only. It is **not** a filesystem format, cloud save, networking protocol, authentication mechanism, persistent host loop, recovery service, APK feature, or production persistence system.
