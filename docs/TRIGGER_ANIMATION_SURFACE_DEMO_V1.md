# Trigger Animation Surface Demo v1

## Tujuan

`TriggerAnimationSurfaceDemo` menggabungkan capability yang sebelumnya terbukti sendiri menjadi satu lintasan finite. Kinematic input memperoleh `MovementAuthority::KinematicRoute`, lalu **hanya** `KinematicMotionController` yang mengubah transform `SceneWorld`. `ScenePhysicsPoseSync` memantulkan posisi X/Z ke ECS physics, XPBD dibaca oleh `GameplayTriggerTracker`, dan delta enter/exit hanya memicu `AnimationStateMachine`. Sample scalar hasil state machine menjadi tint sprite frame-local sebelum `SceneSpriteAdapter` mengisi `SpriteBatch`.

> Trigger delta memengaruhi visual selection saja. Ia tidak dapat menulis transform, menggerakkan actor, memanggil motion controller, memodifikasi RouteIntent, atau melangkahi MovementAuthorityGate.

## Urutan runtime yang dibuktikan

| Urutan | Owner / operasi | Batas authority |
|---|---|---|
| 1 | `MovementAuthorityGate` + `KinematicMotionController` | Satu writer transform SceneWorld. |
| 2 | `ScenePhysicsPoseSync` | Mirror X/Z satu arah SceneWorld → canonical ECS positions. |
| 3 | External `XPBDPhysicsSystem::Step` + trigger tracker | Snapshot physics dan query trigger read-only. |
| 4 | Trigger delta → animation transition | Hanya state selection `idle`/`active`; tidak ada root motion. |
| 5 | Animation sample → tint binding → `QueueTinted` | Output visual frame-local tanpa mutasi SceneWorld. |
| 6 | Software renderer + hidden SDL surface + PPM | Present finite dan artifact, bukan desktop host persisten. |

Script empat frame bergerak dari `x=-0.75` ke `x=0.75`. Trigger circle di `(0,3)` menghasilkan satu enter dan satu exit; enter menyalakan tint active untuk dua frame, exit mengembalikan sample akhir ke idle.

## Evidence executable

`trigger_animation_surface_demo_smoke` menolak konfigurasi invalid lalu memverifikasi 4 frame/4 present, non-background pixels, tepat satu enter dan satu exit, dua active tint frame, final X `0.750`, final animation sample `0.0`, deterministic frame hash `9223671516998279283`, dan PPM `P6`. Target lulus Release dan AddressSanitizer dengan deteksi kebocoran aktif.

| Gate | Hasil final |
|---|---|
| `trigger_animation_surface_demo_smoke` Release | Lulus; authority, enter/exit, tint, frame, dan artifact terukur. |
| `trigger_animation_surface_demo_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 120/120 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 120/120 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Demo tidak menyediakan collision response, Rigidbody generik, NPC AI, blend tree/animation events/skeletal rendering, root motion, 3D physics game loop, multiplayer authority, GPU renderer, persistent desktop host, filesystem projects, APK/AAB, atau release readiness.
