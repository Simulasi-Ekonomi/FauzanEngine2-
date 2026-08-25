# Kinematic Preflight Surface Demo v1

## Tujuan

`KinematicPreflightSurfaceDemo` memperlihatkan jalur gameplay collision preflight secara visual dan terukur. Actor sprite berada di `SceneWorld`, sementara obstacle circle statis berada pada snapshot XPBD. Sebelum setiap request move, `KinematicCollisionPreflight` melakukan raycast read-only ke snapshot tersebut. Bila clear, ia mendelegasikan write transform hanya ke `KinematicMotionController`; bila hit ditemukan, ia mengembalikan `Blocked` dan tidak memodifikasi actor.

> Preflight adalah gate query. Ia tidak melakukan collision response, tidak memanggil XPBD step, dan tidak menulis transform sendiri.

## Script finite

| Frame | Input | Hasil preflight | Transform actor |
|---|---|---|---|
| 0 | Gerak +X | Clear; controller didelegasikan | `x: -0.75 → -0.25` |
| 1 | Gerak +X | Obstacle snapshot terdeteksi; `Blocked` | Tetap `x=-0.25` |
| 2 | Input nol | Controller menerima no-op | Tetap `x=-0.25` |

Actor tetap memperoleh `MovementAuthority::KinematicRoute` tiap frame. Setelah preflight, `SceneWorld::UpdateTransforms` dan `SceneSpriteAdapter` hanya membaca state itu untuk render software/SDL/PPM.

## Evidence executable

`kinematic_preflight_surface_demo_smoke` menolak konfigurasi kurang dari tiga frame lalu membuktikan 3 frame/3 present, dua delegated move (termasuk no-op), satu blocked move, final X `-0.250`, pixel non-background, deterministic hash `14176517661472077123`, dan artifact PPM `P6`.

| Gate | Hasil final |
|---|---|
| `kinematic_preflight_surface_demo_smoke` Release | Lulus; delegated/blocking semantics dan artifact terukur. |
| `kinematic_preflight_surface_demo_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 122/122 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 122/122 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Proof tidak memberikan collision response, sliding, penetration solve, generic Rigidbody, dynamic obstacle update, CCD, 3D shape casting, navmesh, physics replication, GPU rendering, desktop host persisten, APK/AAB, atau release readiness.
