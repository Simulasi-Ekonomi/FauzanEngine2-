# Runtime Telemetry Outbox Status

`TelemetryOutbox` buffers bounded Farm/control-plane JSON envelopes in runtime state. It validates opaque envelope IDs, rejects duplicates, caps entry count and payload size, persists the queue through a versioned binary format, and removes an entry only after a host acknowledgement.

```text
TELEMETRY_OUTBOX_SMOKE_OK bytes=39
```

Release and AddressSanitizer smoke runs pass. The outbox has **no HTTP implementation, bearer token, payment secret, or control-plane authority**. A trusted runtime host must read pending envelopes, attach `ENGINE_TELEMETRY_INGEST_TOKEN` outside the game process, send only genuine runtime envelopes to `/api/runtime/farm`, and acknowledge an envelope only after a successful server response. The smoke payload was not posted to the control-plane database, so the dashboard remains correctly free of test data.
