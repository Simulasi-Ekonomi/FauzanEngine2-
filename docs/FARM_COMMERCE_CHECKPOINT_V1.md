# Farm Commerce Checkpoint V1

## Scope

`FarmCommerceEntitlementLedger` now serializes its bounded accepted provider-receipt identities and audit sequence as a versioned, length-bounded, FNV-1a-checksummed state envelope. Restoration uses candidate vectors and validates the configured player identity, receipt uniqueness, positive entitlement amounts, exact audit sequence continuity, audit kind/error pairing, terminal bytes, and checksum before committing ledger members.

`FarmCommerceCheckpoint` envelopes a serialized `FarmWorldTool` state together with this ledger state. It validates the outer version, lengths, and checksum before mutation. If either world or ledger restoration rejects after the other has changed, it restores both captured pre-load states. This keeps Farm coin state and the receipt replay set coupled for the controlled in-memory restore path.

## Executable Evidence

`farm_commerce_checkpoint_smoke` proves the following in Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1`:

| Case | Required result |
|---|---|
| Accepted receipt then checkpoint | Restored Farm coin total and accepted-receipt count match the captured state. |
| Receipt replay after restore | The original provider receipt is rejected as a duplicate and cannot add coins again. |
| New receipt after restore | A distinct verifier-approved receipt is accepted and reconciliation of the restored receipt succeeds. |
| Trailing/corrupt checkpoint | Restore fails while both the active world bytes and ledger bytes remain unchanged. |
| Player mismatch/corrupt ledger snapshot | Direct ledger restore fails closed and cannot replace active receipt state. |

## Boundary

The envelope is an **in-memory serialization contract**, not a payment-provider protocol or durable storage subsystem. It performs no payment, billing, provider API verification, cryptographic signing, encryption, key management, filesystem write, cloud sync, reconciliation with a real provider, chargeback processing, or Play Store validation. The caller still supplies the verifier at initialization; a restore does not establish trust in a provider receipt by itself.
