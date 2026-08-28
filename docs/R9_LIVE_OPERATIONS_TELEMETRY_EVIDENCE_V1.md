# R9 Live Operations Telemetry Evidence V1

## Scope

`FarmTelemetryAdapter` kini memiliki `FarmTelemetryPolicy` bounded. Mode default memancarkan schema version, host/game/version reference, aggregate Farm counters, event type/reference, dan timestamp; `playerId`, `goldBalance`, `observedFarmCoins`, serta nilai/detail event tidak dipancarkan secara default. Jumlah event dibatasi maksimum 64 dan caller dapat memilih cap lebih rendah. Field diagnostik yang lebih sensitif hanya tersedia melalui policy opt-in.

`TelemetryOutbox` tetap menjadi bounded pending envelope queue dengan serialization/acknowledgement.

## Release and ASAN evidence

```text
FARM_TELEMETRY_SMOKE_OK bytes=911 diagnostic_bytes=1112 events=6 privacy=default-redacted cap=64
TELEMETRY_OUTBOX_SMOKE_OK bytes=39
```

Kedua smoke lulus pada Release dan AddressSanitizer dengan `ASAN_OPTIONS=detect_leaks=1`. Smoke membuktikan schemaVersion, redaction default, opt-in player/economic/event diagnostics, invalid cap rejection, serta outbox regression.

## Boundary and status

Ini adalah in-process serialization/privacy policy evidence. Belum ada trusted token-holding ingest host, authenticated transport, consent/retention enforcement, PII classification review, durable event sink, dashboards, alerting, SLO, incident runbook, feature rollback, or production privacy/legal review. Karena itu **R9 tetap Not passed**.
