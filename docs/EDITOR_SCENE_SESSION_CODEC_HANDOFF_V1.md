# EditorSceneSession Codec Handoff v1

## Tujuan

Fase B.3b menghubungkan envelope `EditorSceneDocumentCodec` yang sudah bounded dengan `EditorSceneSession` tanpa membuat jalur scene atau asset baru. `OpenBytes` mendekodekan bytes ke `EditorSceneDocument` kandidat lokal dan kemudian memanggil `Open` yang existing. `SaveBytes` menyandikan snapshot document yang telah dimuat melalui codec canonical, seluruhnya di memori.

> Keberhasilan decode tidak menyatakan asset siap. `OpenBytes` baru sukses setelah `Open` tetap melewati `EditorSceneDocumentAdapter`, mesh binder, sprite binder, dan `AssetRegistry` readiness yang berlaku.

## Kontrak operasi

| Operasi | Perilaku yang dibuktikan | Atomicity |
|---|---|---|
| `OpenBytes(bytes, assets)` | Decode SceneDocument kandidat lalu delegasi ke `Open`. | Bytes malformed tidak mengubah session. Dokumen valid dengan asset yang tidak ready/hilang tetap gagal melalui `DocumentLoadFailed` tanpa mengganti session. |
| `SaveBytes(bytes)` | Menyandikan snapshot session yang terbuka dengan codec v1. | Bila session belum terbuka, gagal sebagai `InvalidDocument` dan bytes caller tetap utuh. |
| Error codec | Decode bytes gagal menjadi `CodecDecodeFailed`. | Tidak menyalurkan transform atau state baru ke `SceneWorld`, binders, atau renderer. |
| Error validasi scene/asset | Setelah decode berhasil, error tetap berasal dari jalur `Open`. | Tetap memakai candidate-build/commit session existing. |

`OpenBytes` tidak menulis `SceneWorld` sendiri. Ownership transform tetap berada pada jalur SceneWorld/session yang telah ada; handoff byte ini tidak menambah writer gameplay, physics, RouteIntent, atau MovementAuthorityGate.

## Evidence executable

`editor_scene_session_codec_smoke` membangun AssetRegistry texture siap yang nyata, membuka SceneDocument sprite, menyimpan bytes, lalu membuka kembali session dari bytes dan merender viewport. Smoke memverifikasi deterministic frame hash, rollback setelah magic rusak, rollback setelah document yang valid tetapi mereferensikan asset hilang, serta preservation bytes saat `SaveBytes` dipanggil pada session kosong.

| Gate | Hasil final |
|---|---|
| `editor_scene_session_codec_smoke` Release | Lulus; `bytes=191`, round-trip, malformed rollback, dan asset-open rollback. |
| `editor_scene_session_codec_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 113/113 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 113/113 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Increment ini bukan filesystem persistence, project browser, file locking, migration framework, undo/redo, gizmo, desktop editor UI, collaboration, network replication, asset importer, hot reload, GPU upload, APK/AAB, atau release readiness. Ia hanya menyediakan seam in-memory yang dapat dibangun di atasnya secara bounded pada increment berikutnya.
