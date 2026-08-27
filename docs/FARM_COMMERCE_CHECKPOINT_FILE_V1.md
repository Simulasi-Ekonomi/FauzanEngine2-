# Farm Commerce Checkpoint File V1

## Scope

`FarmCommerceCheckpointFile` is a narrow adapter over `FarmCommerceCheckpoint` and the existing `AtomicSaveFile` primitive. It serializes a world-plus-commerce checkpoint first; only a valid nonempty checkpoint is passed to the safe-slot file writer. Reading likewise obtains isolated bytes first and delegates restoration to the checkpoint contract, preserving the caller’s world and entitlement ledger whenever storage or checkpoint validation rejects.

The adapter intentionally reports only four outcomes: success, checkpoint rejection before writing, storage-write rejection, or storage-read rejection. It does not reinterpret payment receipts, bypass the caller-supplied receipt verifier, or transform storage errors into a successful restore.

## Executable Evidence

`farm_commerce_checkpoint_file_smoke` proves the following in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`:

| Case | Required result |
|---|---|
| Uninitialized world/ledger | Save is rejected before any persistence attempt. |
| Valid local save and restore | Farm coin state and accepted receipt identity are restored. |
| Replayed receipt | The restored receipt identity is rejected and cannot increase coins. |
| Missing slot / invalid slot | The wrapper reports a storage error while both active world and ledger snapshots remain unchanged. |
| Corrupted stored bytes | The wrapper reports checkpoint rejection while both active world and ledger snapshots remain unchanged. |

## Boundary

This is **local controlled file persistence only**. The underlying primitive uses a safe slot name plus write/flush/rename; it does not provide power-loss `fsync` evidence, locking, journal recovery, encryption, key management, database/cloud replication, access control, provider verification, real-money payment processing, chargeback handling, or store readiness. It must not be presented as a production commerce backend.
