# Farm Commerce Fraud Trust Evidence V1

## Scope

`FarmCommerceEntitlementLedger` kini dapat menerima binding opsional ke `TrustSafetySystem`. Rejection yang memiliki sinyal fraud kuat dipromosikan secara deterministik ke trust ledger untuk player yang terkait, sementara wrong-player, reversed, dan verifier-rejected input tetap dicatat sebagai commerce rejection tetapi tidak otomatis dianggap fraud.

## Evidence

`farm_commerce_entitlement_smoke` membuktikan:

| Signal | Result |
|---|---|
| Duplicate provider receipt | Ledger rejects duplicate, does not add coins, reports `DuplicateReceipt`, and raises player score to 3. |
| Reconciliation mismatch | Ledger rejects mismatch, does not mutate entitlement, reports `LedgerMismatch`, and raises score to 8 so the player is banned. |
| Ban enforcement | Subsequent authoritative Farm command remains rejected by the existing authority gate for the banned player. |
| False-positive boundary | Wrong-player, verifier-rejected, and reversed receipt paths remain commerce audit rejections without automatic fraud promotion. |
| Existing behavior | Commerce checkpoint and authoritative service smokes continue to pass, including restore, duplicate prevention, and banned-player rejection. |

Release and ASAN output:

```text
FARM_COMMERCE_ENTITLEMENT_SMOKE_OK approved=1 duplicate=1 reversal=1 reconcile=1 fraud_report=duplicate,reconcile ban=1
FARM_COMMERCE_CHECKPOINT_SMOKE_OK restored=1 replay_rejected=1 checksum=1 atomic=1
FARM_AUTHORITATIVE_SERVICE_SMOKE_OK revision=4 harvested=2 replay=1 banned=1
```

## Boundary

This is canonical in-process fraud-signal integration. It does not claim provider-backed fraud detection, server deployment, tamper-resistant audit storage, rate-limit operations, ban appeal governance, identity verification, adversarial load, or production financial/compliance readiness.
