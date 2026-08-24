# Farm Runtime Verification Status

## Scope recovered in the canonical source tree

The active `Source/NeoEngine` tree now contains a bounded `FarmSystem` runtime and a `farm_smoke` executable. The recovery was necessary because the canonical CMake project only exposed XPBD targets and the former headless Farm implementation was not present in the active tree.

| Capability | Runtime contract | Verification |
|---|---|---|
| Bounded terrain | Constructor accepts at most 1,000 tiles and rejects invalid dimensions. | `FarmSystem(20, 50)` creates the 1,000-tile smoke world. |
| Crop loop | Till, plant, water, deterministic tick progression, and harvest are state-checked. | Five wheat plots are harvested in the smoke scenario. |
| Inventory and economy | Seed/produce/egg inventory is bounded; sales use idempotent ledger IDs. | First sale succeeds; duplicate sale ID is rejected. |
| Top-up protection | Runtime accepts top-ups only after an injected authority verifier approves a receipt and only once per receipt ID. | Invalid receipt is rejected; approved receipt succeeds; duplicate receipt is rejected. |
| Animal output | Hen production is tick-based and emits eggs deterministically. | One hen produces an egg after 12 ticks. |
| Quest state | Harvest progress completes a bounded quest at five harvest actions. | Smoke scenario reaches the harvest quest completion condition. |
| Telemetry | Snapshot includes tick, revision, event sequence, coins, tile distribution, animal count, quest state, and last error. | Smoke runtime emits a non-empty event stream and snapshot. |
| Persistence | Versioned binary state encodes tiles, inventory, animals, quest state, and ordered ledger IDs; invalid data fails closed. | Serialize/deserialize round trip preserves coins, egg inventory, and quest completion. |

## Latest evidence

```text
FARM_SMOKE_OK tiles=0 harvestable=0 coins=242 events=24 bytes=7108
XPBD_REGRESSION_OK contacts=1 bytes=196668 overlaps=2
XPBD_DETERMINISM_OK bytes=108
```

The Farm smoke executable also passed under AddressSanitizer with leak detection enabled. The current core is headless by design: it is ready for a presentation layer, but it does not yet include a renderer, input loop, Android package, or network authority client. Top-up verification remains an injected server-authority contract; no secret or payment validation is embedded in the game runtime.

## Executable player loop

`farm_demo` is a playable terminal vertical slice backed exclusively by `FarmSystem`. It accepts `till`, `plant`, `water`, `tick`, `harvest`, `animal`, `sell`, and `status` commands, returns explicit errors such as `duplicate_transaction`, and supports `--scripted` mode for reproducible testing. A verified scripted flow completed crop growth, a harvest, a sale, a rejected duplicate sale, and a final status read without bypassing the runtime authority rules.

This executable is deliberately **not** presented as a graphical FarmVille client. A graphical UI and input/render binding remain dependent on the active renderer path, which is still outside this recovered Farm core.

## Runtime-to-control-plane telemetry contract

`FarmTelemetryAdapter` now converts the authoritative `FarmSystem` snapshot and bounded event history into the exact JSON shape accepted by `POST /api/runtime/farm`. It emits source identity, game/version/player references, a deterministic snapshot reference, game-economy counters, and `farm.*` events. The runtime adapter contains **no bearer token, HTTP client, payment secret, or decision authority**.

`farm_telemetry_smoke` generated an envelope containing a 1,000-tile world, one accepted authority receipt, one rejected duplicate sale, and bounded Farm events. The control-plane Vitest schema contract accepted this form and rejected malformed event names and negative balances. Release and AddressSanitizer Farm smoke tests passed.

The remaining connection step is intentionally outside the game loop: a trusted runtime host/forwarder must take this envelope and attach `ENGINE_TELEMETRY_INGEST_TOKEN` when calling the control-plane endpoint. Until that host delivers an envelope, the dashboard correctly remains in its empty **awaiting-runtime** state rather than showing invented game data.
