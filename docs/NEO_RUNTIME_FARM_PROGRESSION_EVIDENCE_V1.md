# NeoRuntime Farm Progression Evidence V1

## Scope

NeoRuntime kini memiliki opsi `enableFarmCurriculum` yang menginisialisasi graph authored `AgricultureCurriculum` dan mengevaluasinya setelah canonical Farm tick. Receipt progression disertakan pada `NeoRuntimeFrameReceipt` dan `FarmRuntimeFrameReceipt`, sehingga HUD dapat menampilkan indikator lesson completion tanpa mengambil alih Farm authority atau membuat loop `FarmRuntimeSession` kedua.

## Evidence

`neo_runtime_farm_vertical_slice_smoke` mengaktifkan curriculum dan membuktikan:

| Capability | Evidence |
|---|---|
| Onboarding/progression graph | `agri.orientation` dan `agri.land-investment` menjadi completed setelah canonical Plant/Farm state memenuhi condition. |
| Player-facing feedback | HUD menggambar token lesson `L<n>` bersama inventory/coin/tick/action state dan tetap menghasilkan framebuffer berbeda dari world framebuffer. |
| Checkpoint recovery | Curriculum receipt completion count dan revision ikut disimpan pada Farm progress checkpoint, lalu dipulihkan bersama world/time/authority. |
| Corruption handling | Corrupt checkpoint ditolak tanpa mengubah world, render receipt, atau curriculum completion state. |
| Canonical ownership | Evaluasi terjadi pada `NeoRuntime::Tick` setelah FarmWorld tick; tidak ada simulation loop kedua. |

Release dan ASAN menghasilkan receipt yang sama:

```text
NEO_RUNTIME_FARM_VERTICAL_SLICE_SMOKE_OK actions=till,plant,water,harvest hud=1 curriculum=onboarding_progression checkpoint=atomic cpu_present=1
```

## Boundary

Increment ini memperkuat R2 pada onboarding, progression, authored lesson data, player-facing HUD feedback, dan local recovery. R2 tetap **Not passed** karena belum memiliki complete player onboarding UX, energy/inventory economy contract yang lengkap, authored balancing data yang dieksekusi sebagai gameplay balance, persistent production save, physical platform acceptance, accessibility/localization, atau release package evidence.
