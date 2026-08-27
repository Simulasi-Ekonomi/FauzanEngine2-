# FauzanEngine Release Readiness Matrix

## Status rule

No FauzanEngine game, game template, client, APK, AAB, website, or control-plane surface may be called **release-ready**, **100% ready**, or **ready to ship** unless every mandatory gate in this matrix has passed with reproducible evidence. A passing unit test, sandbox, CPU frame, or in-process load simulation proves only the specific gate it covers.

The present engine status is **not release-ready**. Existing Farm and RPG sandboxes are backend validation assets. This matrix is a production contract, not a claim that the capabilities below already exist.

## Mandatory release gates

| ID | Gate | Mandatory production evidence | Current evidence | Status |
|---|---|---|---|---|
| R1 | Canonical game tool | A typed, versioned game-tool contract for world, rules, content, save migration, invalid input, and deterministic replay. | Farm canonical tool contract now covers typed FarmWorldTool binding, versioned rules/content, v1→v2 save migration, fail-closed invalid input, and deterministic replay; Release/ASAN evidence is recorded in `docs/FARM_CANONICAL_GAME_TOOL_R1_EVIDENCE_V1.md`. This pass is Farm-specific; other templates still require independent evidence. | Passed for Farm canonical tool scope |
| R2 | Complete game loop | A playable vertical slice implementing onboarding, core loop, progression, failure/recovery, balancing data, content authoring, and player-facing UX. | NeoRuntime Farm slice now integrates authored `AgricultureCurriculum` progression into frame/HUD receipts and topology-preserving checkpoints; Release/ASAN evidence is recorded in `docs/NEO_RUNTIME_FARM_PROGRESSION_EVIDENCE_V1.md`. Energy/economy feedback, complete onboarding UX, executed balancing, persistent production save, accessibility/platform acceptance, and full player package evidence remain incomplete. | Not passed |
| R3 | Asset and renderer path | Production decoders; content limits; texture/mesh/material uploads; camera, lighting, animation; surface/swapchain presentation; resource-loss handling; device evidence. | Byte registry, CPU renderer, and Vulkan offscreen triangle proof. | Not passed |
| R4 | Input, audio, accessibility | Platform touch/controller input, audio output, localization/text scaling, focus handling, offline/error UX, and accessibility acceptance tests. | Bounded input state and PCM mixer only. | Not passed |
| R5 | Authoritative multiplayer | Versioned client commands, authentication/session binding, authoritative simulation, snapshot/delta protocol, idempotency, replay protection, reconciliation, reconnect, and integration/load evidence. | No networked authoritative runtime. | Not passed |
| R6 | Anti-cheat and fraud | Server-side validation of every privileged command, rate limits, tamper-resistant evidence records, receipt verification, ban lifecycle, appeal governance, and adversarial replay/load tests. | Local TrustSafety and Farm anti-inject rules only. | Not passed |
| R7 | Economy and commerce | Ledger invariants, product catalog/receipt authority, refund/reversal handling, duplicate prevention, entitlement reconciliation, audit export, and operational review. | Local Farm transactions and verified-receipt interface only. | Not passed |
| R8 | Persistence and recovery | Authoritative durable store, schema migrations, backups/restores, corruption handling, retention/deletion, privacy boundary, disaster-recovery test, and no credentials in saves. | Versioned local serializations and telemetry outbox only. | Not passed |
| R9 | Live operations | Token-holding trusted ingest host, real telemetry schema, privacy-minimized event policy, alerting, feature/config rollback, incident runbook, and no fabricated player data. | Local outbox and host-forwarder contract only. | Not passed |
| R10 | Security and privacy | Threat model, secret management, abuse controls, dependency/SBOM review, vulnerability response, data-access policy, consent/age handling where required, and penetration/adversarial testing. | Runtime credential exclusion documented. | Not passed |
| R11 | Android delivery | Canonical Android source subset builds on pinned SDK/NDK/Java; debug APK, signed release AAB, emulator/device smoke, crash/ANR evidence, and Play policy/store asset review. | Android preflight blocks; no SDK/NDK or APK/AAB in sandbox. | Not passed |
| R12 | Launch operations | Soft-launch plan, real consented observability, support/appeal workflow, rollback, capacity plan, service SLOs, release checklist, and final owner sign-off. | No soft-launch or operational evidence. | Not passed |

## Template-specific requirement

Each advertised game template must pass R1–R12 independently. Shared engine tooling can satisfy portions of a gate only if the particular game proves its integration. A template catalog entry, executable smoke, or game-state sandbox cannot be promoted to a production template by association with another game.

## Farm-specific release additions

Farm requires authoritative validation of crop/animal timers, building placement, inventories, market prices, top-up entitlements, player-to-player or co-op rules if enabled, and government policy boundaries. NPC and government tools are advisory or simulation components; they must never mint currency, accept receipts, modify bans, or bypass the authoritative service.

## Evidence package requirement

Every final release claim must include the exact build identifier, source revision, dependency lockfile/SBOM, test reports, performance/load reports, supported-device list, package hashes, signing provenance without secret disclosure, operations runbook, and documented known issues. Any missing item leaves the release status as **not passed**.
