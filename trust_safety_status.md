# Trust Safety Runtime Status

`TrustSafetySystem` is a bounded runtime enforcement core for three deterministic signals: duplicate receipt, ledger mismatch, and impossible inventory. Each signal has a fixed weight. A player reaching the threshold of eight is permanently banned in the local authoritative runtime state, and later reports for that player are rejected. Duplicate evidence IDs are rejected globally.

```text
TRUST_SAFETY_SMOKE_OK ban=permanent audit=2
```

Release and AddressSanitizer validation pass. The smoke replay produces the same audit hash, rejects a duplicate receipt ID, records a three-point duplicate-receipt signal and a five-point ledger-mismatch signal, bans at eight, then rejects post-ban reporting.

The system intentionally does not expose network, payment, or agent authority. A trusted server/forwarder must ingest signed audit records, preserve an appeal/governance record, and reconcile ban state across sessions. Thresholds are deterministic code policy and still require operational review before commercial release.

## Farm economy enforcement

FarmSystem can now receive TrustSafetySystem through explicit dependency injection with a player identity. A duplicate accepted top-up is recorded as a duplicate-receipt signal; an attempted sale beyond inventory is recorded as impossible inventory. Once the score reaches the permanent-ban threshold, subsequent Farm economic actions fail with `FarmError::Banned` before receipt verification or balance mutation.

```text
FARM_TRUST_SMOKE_OK banned=1 audit=2
```

Release, Farm regression, telemetry smoke, XPBD regression/determinism, and AddressSanitizer all pass. The local ban does not replace server-side persistence, reconciliation, appeal handling, or policy review.
