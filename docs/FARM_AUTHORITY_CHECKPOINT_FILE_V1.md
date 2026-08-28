# Farm Authority Checkpoint File V1

`FarmAuthorityCheckpointFile` is a narrow local-file adapter around the existing atomic `FarmAuthorityCheckpoint` codec. It accepts only a safe `AtomicSaveFile` slot and stores a complete Farm world plus authority-ledger snapshot as one checkpoint payload. The adapter does not serialize a transport endpoint, socket, token, or session credential.

On load, the adapter reads the safe slot before delegating to `FarmAuthorityCheckpoint::Load`; that codec decodes candidates and restores the prior world plus authority ledger if either candidate fails. Missing slots are classified as storage-read rejection, invalid slots as storage-write rejection, and invalid/corrupt checkpoint bytes as checkpoint rejection. In every rejection case, the adapter leaves caller-owned world and ledger state untouched.

`farm_authority_checkpoint_file_smoke`, `farm_authority_checkpoint_smoke`, and `farm_authoritative_session_host_smoke` pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`. The combined evidence proves local save/restore, explicit post-restore session rebind, duplicate-command replay retention, missing-slot rejection, traversal-style slot rejection, corrupt-file rejection, and atomic state preservation.

This is not durable public-server infrastructure. It provides no TLS, authentication token persistence, session-secret storage, file locking, power-loss guarantee, encryption, multi-process coordination, replication, matchmaking, or public multiplayer claim.
