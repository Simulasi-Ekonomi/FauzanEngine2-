# R10 Security and Privacy Boundary Evidence V1

## Threat model boundary

| Asset | Threat | Canonical boundary | Evidence status |
|---|---|---|---|
| Runtime settings/save payload | Credential or token leakage in local state | `RuntimeSettingsStore` and `RuntimeSaveCodec` reject credential markers, bound keys/values/payloads, and use checksum/schema validation | Release/ASAN smoke passed |
| Telemetry identity/economy fields | Unnecessary player or balance disclosure | `FarmTelemetryPolicy` redacts player ID, direct balances, and event values by default; diagnostics are explicit opt-in | Release/ASAN smoke passed |
| Privileged Farm commands | Client/UI bypass of authority and trust rules | Canonical Farm authority and TrustSafety gates reject invalid subject/session/replay/fraud paths | Release/ASAN authority/trust smokes passed |
| Local save slot/path | Traversal, malformed slot, or corrupt payload | Safe slot validation, bounded payload, checksum/schema rejection, temporary-file write, backup/restore failure preservation | Release/ASAN persistence smokes passed |

## Evidence commands and outputs

The following canonical targets pass in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`:

```text
RUNTIME_PERSISTENCE_SMOKE_OK settings=2 atomic=1 save=1 checksum=1 sensitive=1
FARM_TELEMETRY_SMOKE_OK bytes=911 diagnostic_bytes=1112 events=6 privacy=default-redacted cap=64
ATOMIC_SAVE_FILE_SMOKE_OK write=1 read=1 codec=1 slotValidation=1 backup=1 restore=1 missingRestorePreserved=1
```

These tests prove bounded local rejection and privacy defaults only. They do not prove that the production deployment is secure.

## Explicit exclusions

No credentials, API keys, payment secrets, signing keys, or provider tokens are introduced. No SBOM, dependency vulnerability scan, penetration test, exploit-resistance claim, age/consent legal determination, encrypted-at-rest guarantee, IAM/access-control deployment, incident response SLA, or vulnerability-response operation is fabricated by this increment. Those artifacts require repository/release-owner and deployment evidence.

Therefore **R10 remains Not passed**.
