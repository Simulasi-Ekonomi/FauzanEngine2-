# R2 Strict Gap Audit V1

## Contract

R2 hanya dapat dinyatakan **Passed** apabila satu jalur canonical game loop memiliki evidence reproducible untuk onboarding, core loop, progression, failure/recovery, executed balancing data, content authoring, dan player-facing UX. Evidence R2 tidak boleh digantikan oleh kemajuan R3–R12.

| Requirement | Current canonical evidence | Remaining gate condition |
|---|---|---|
| Onboarding | `AgricultureCurriculum` completion plus `FarmOnboardingReceipt` next-step state (`NEXT TIL`, `NEXT PLN`, `NEXT WAT`, `NEXT HAR`) rendered by HUD | Formal player acceptance of the complete guided flow and restart/skip behavior |
| Core loop | NeoRuntime keyboard flow Till → Plant → Water → Tick → Harvest, inventory, authored sale price, and energy budget | Broader player-facing content and acceptance outside the bounded Farm scenario |
| Progression | Authored curriculum graph, lesson receipt, checkpointed lesson progress, and action-state next step | Longer progression arc and balance review across authored lessons |
| Failure/recovery | Insufficient energy rejects without tile/inventory mutation; Tick regenerates energy; missing/corrupt file restore preserves caller state | Player acceptance of recovery/error UX and broader recovery scenarios |
| Balancing data | Validated `FarmBalanceProfile` drives all three crop growth durations, yields, sale prices, energy costs, and regeneration; dedicated three-crop smoke passes | Product-owner balance tuning/review and evidence over the full intended economy/content set |
| Content authoring | `RuntimeConfig::farmBalance` and authored curriculum are consumed by canonical runtime initialization | Authoring workflow acceptance for the complete intended content catalog |
| Player-facing UX | HUD renders counters, lesson token, energy, next-step/error feedback, action availability, CPU frame/presentation | Physical platform, accessibility, localization, text scaling, and package/player acceptance |
| Persistent production save | `FarmProgressCheckpointFile` saves/loads NeoRuntime checkpoint bytes through `AtomicSaveFile`, with missing/corrupt preservation | Deployment-grade storage/DR is outside this local R2 proof and must not be fabricated |

## Evidence commands

Release and AddressSanitizer with `ASAN_OPTIONS=detect_leaks=1` pass:

```text
NEO_RUNTIME_FARM_VERTICAL_SLICE_SMOKE_OK actions=till,plant,water,harvest hud=1 curriculum=onboarding_progression balance=authored energy=failure-recovery checkpoint=atomic cpu_present=1
NEO_RUNTIME_FARM_PROGRESS_FILE_SMOKE_OK save=1 restore=1 missing_preserved=1 corrupt_preserved=1
FARM_BALANCE_PROFILE_SMOKE_OK invalid_rejected=1 crops=3 growth=authored yield=authored price=authored energy=88
```

## Strict status

The functional R2 Farm slice is substantially covered by canonical local evidence. R2 remains **Not passed** until the remaining acceptance conditions are independently supplied and reviewed: physical platform/player acceptance, accessibility/localization/text scaling, complete content-authoring acceptance, longer balance review, and release package evidence where required by the project matrix. No R3 work is authorized by this document.
