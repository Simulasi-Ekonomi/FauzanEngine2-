# Farm Commerce Audit Export Evidence V1

## Scope

`FarmCommerceEntitlementLedger::ExportAuditLog` menyediakan salinan caller-owned dari audit receipt commerce yang sudah versioned dan dibatasi oleh `kMaxAuditReceipts`. Export tidak memberikan akses mutasi terhadap log internal dan gagal secara aman jika ledger belum siap atau batas internal tidak valid.

## Evidence

`farm_commerce_entitlement_smoke` pada Release dan ASAN membuktikan tujuh receipt berurutan mencakup wrong-player rejection, verifier rejection, approved entitlement, duplicate rejection, reversal rejection, reconciliation mismatch, dan approved reconciliation. Smoke juga mengubah copy export dan memastikan `LastAudit` internal tidak berubah.

```text
FARM_COMMERCE_ENTITLEMENT_SMOKE_OK approved=1 duplicate=1 reversal=1 reconcile=1 fraud_report=duplicate,reconcile ban=1 audit_export=7
FARM_COMMERCE_CHECKPOINT_SMOKE_OK restored=1 replay_rejected=1 checksum=1 atomic=1
FARM_AUTHORITATIVE_SERVICE_SMOKE_OK revision=4 harvested=2 replay=1 banned=1
```

## Boundary

Ini adalah bounded in-process audit export. Belum ada provider/payment integration, durable append-only audit store, encrypted export, access-control policy, operational review workflow, refund provider reconciliation, atau store-readiness evidence.
