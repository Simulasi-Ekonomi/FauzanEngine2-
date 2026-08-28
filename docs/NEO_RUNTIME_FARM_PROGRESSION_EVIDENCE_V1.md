# NeoRuntime Farm Progression Evidence V1

## Scope

NeoRuntime kini memiliki opsi `enableFarmCurriculum` yang menginisialisasi graph authored `AgricultureCurriculum` dan mengevaluasinya setelah canonical Farm tick. `RuntimeConfig::farmBalance` memasok profile balance versioned/validated ke FarmSystem; growth, harvest yield, crop price, energy costs, dan energy regeneration dipakai oleh gameplay rules. Receipt progression, energy, next onboarding step, dan failure state disertakan pada frame/HUD tanpa mengambil alih Farm authority atau membuat loop `FarmRuntimeSession` kedua.

## Evidence

`neo_runtime_farm_vertical_slice_smoke` mengaktifkan curriculum dan membuktikan:

| Capability | Evidence |
|---|---|
| Onboarding/progression graph | `agri.orientation` dan `agri.land-investment` menjadi completed setelah canonical Plant/Farm state memenuhi condition. |
| Player-facing feedback | HUD menggambar token lesson `L<n>`, wheat/energy counters, action state, dan bounded next-step/error status (`NEXT TIL`, `NO ENERGY`, dan seterusnya), serta tetap menghasilkan framebuffer berbeda dari world framebuffer. |
| Checkpoint recovery | Curriculum receipt completion count dan revision ikut disimpan pada Farm progress checkpoint, lalu dipulihkan bersama world/time/authority. |
| Corruption handling | Corrupt checkpoint ditolak tanpa mengubah world, render receipt, atau curriculum completion state; `FarmProgressCheckpointFile` juga membuktikan save/load file checkpoint, missing-file rejection, dan caller-state preservation saat runtime restore menolak payload corrupt. |
| Canonical ownership | Evaluasi terjadi pada `NeoRuntime::Tick` setelah FarmWorld tick; tidak ada simulation loop kedua. |

Release dan ASAN menghasilkan receipt yang sama:

```text
NEO_RUNTIME_FARM_VERTICAL_SLICE_SMOKE_OK actions=till,plant,water,harvest hud=1 curriculum=onboarding_progression balance=authored energy=failure-recovery checkpoint=atomic cpu_present=1
NEO_RUNTIME_FARM_PROGRESS_FILE_SMOKE_OK save=1 restore=1 missing_preserved=1 corrupt_preserved=1
```

## Boundary

Increment ini memperkuat R2 pada onboarding next-step state, progression, authored lesson/balance data yang dieksekusi sebagai rules, energy/inventory economy feedback, in-loop failure/recovery, player-facing HUD feedback, dan persistent local production checkpoint. R2 tetap **Not passed** sampai complete player acceptance, longer progression/balancing review, physical platform/accessibility/localization acceptance, and package evidence are independently available; this workflow does not fabricate those external artifacts. See `docs/R2_STRICT_GAP_AUDIT_V1.md`.
