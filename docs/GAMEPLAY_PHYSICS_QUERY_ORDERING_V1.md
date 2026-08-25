# Gameplay Physics Query Ordering v1

## Tujuan

Fase D.5a membuat output `GameplayPhysicsQuery::OverlapCircle` canonical setelah collider XPBD yang overlap berhasil dipetakan ke `EntityID`. Mapping tetap dilakukan ke vector kandidat lokal. Hanya setelah seluruh mapping sukses, EntityID diurutkan ascending dan output caller diganti.

> Ordering ini adalah kontrak query read-only. Ia tidak mengubah flattened collider snapshot, tidak memanggil `XPBDPhysicsSystem::Step`, tidak mengubah layer/constraint, dan tidak menulis ECS atau SceneWorld.

## Kontrak

| Kondisi | Perilaku |
|---|---|
| Shape + mask valid, seluruh mapping valid | EntityID hasil overlap dikembalikan ascending. |
| Tidak ada overlap | Output valid berupa vector kosong. |
| Shape/mask invalid, capacity melampaui limit, atau mapping entity gagal | Output caller sebelumnya dipertahankan karena commit belum dilakukan. |
| Raycast | Tidak berubah; tetap mengembalikan hit tunggal XPBD terdekat. |

Sorting dilakukan sesudah mapping, bukan pada internal collider index. Jadi pemanggil gameplay menerima ordering yang tidak bergantung pada urutan collider flattened dari solver untuk query overlap yang sama.

## Evidence executable

`gameplay_physics_query_smoke` sekarang menambahkan static body kedua pada overlap yang sama, lalu memverifikasi dua `EntityID` diurutkan ascending. Smoke tetap membuktikan raycast static/dynamic filtering, invalid/non-finite input rollback, empty overlap, dan no-step-write boundary.

| Gate | Hasil final |
|---|---|
| `gameplay_physics_query_smoke` Release | Lulus; static, dynamic, sorted overlap, atomic failure, no solver step/write. |
| `gameplay_physics_query_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 119/119 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 119/119 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Increment ini bukan collision response, generic Rigidbody API, continuous collision detection, 3D query system, query batching, spatial query cache, navmesh, physics replication, gameplay transform writer, atau production readiness.
