# Farm Runtime Session Checkpoint V1

## Scope

`FarmRuntimeSession` now provides an explicit in-memory checkpoint handoff over the canonical `FarmRuntimeSaveCodec`.

| API | Preconditions | Successful effect | Failure boundary |
|---|---|---|---|
| `SaveCheckpoint(revision, bytes)` | Initialized session; nonzero revision; ready Farm | Replaces `bytes` with a validated `farm-world` envelope. | Rejects without replacing caller bytes, frame count, receipt, renderer, or Farm state. |
| `RestoreCheckpoint(bytes, revision)` | Initialized session; canonical valid envelope | Restores the referenced Farm and returns its envelope revision. | Preserves Farm, output revision, frame count, receipt, and renderer on envelope or Farm decode failure. |

The session does not own Farm simulation, `FarmWorldTool`, renderer, asset registry, input, or persistence location. The checkpoint methods neither tick the world nor render a frame; therefore the last committed frame receipt and frame count are unchanged across a successful restore as well as rejected restore attempts.

## Evidence

`farm_runtime_session_checkpoint_smoke` initializes the canonical Farm session, commits one frame, snapshots a tilled Farm state at revision 7, mutates Farm state, and restores the snapshot. It verifies that the Farm payload is restored while the session frame count, committed receipt, and renderer hash remain unchanged. It also verifies preservation under a wrong-kind envelope, checksum corruption, and a zero-revision save request.

The smoke is required to pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`, followed by the broad non-Vulkan smoke suite.

## Boundary

This is a finite caller-driven, in-memory handoff only. It does **not** implement a filesystem save, cloud synchronization, networking, authentication, a long-running host loop, recovery service, APK integration, or production persistence guarantee.
