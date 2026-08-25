# EditorSceneSession Dirty State v1

## Tujuan

`EditorSceneSession` sekarang membedakan session yang telah dimuat atau disimpan dari session yang memiliki mutasi editor in-memory belum tersimpan. Query `HasUnsavedChanges()` membandingkan revision document aktif terhadap revision snapshot terakhir yang berhasil dibuka atau disimpan.

> Dirty state adalah metadata session in-memory. Ia bukan filesystem durability, recovery signal, lock, indikator asset reload, atau bukti bahwa data sudah tersimpan di disk maupun remote.

## Kontrak

| Jalur | Dampak terhadap dirty state |
|---|---|
| `Open` atau `OpenBytes` berhasil | Session menjadi clean setelah seluruh candidate document, SceneWorld, dan binder asset berhasil dibangun. |
| `UpdateTransform`, `ReparentActor`, `AddActor`, atau `DeleteActor` berhasil | Session menjadi dirty; revision document telah meningkat melalui jalur mutation yang existing. |
| `Save` atau `SaveBytes` berhasil | Snapshot in-memory saat ini menjadi clean. `SaveBytes` tetap tidak menulis filesystem. |
| Decode gagal, validasi scene/asset gagal, atau mutasi gagal | Dirty state sebelumnya tetap dipertahankan. Tidak ada commit sebagian. |

Implementasi menggunakan `OpenCandidate(document, assets, markSaved)`. Semua jalur terus membangun `EditorSceneDocumentAdapter`, `SceneWorld`, mesh/sprite binders, serta renderer kandidat sebelum commit. Mutator internal memanggil helper dengan `markSaved=false`; open eksplisit memanggilnya dengan `markSaved=true`. Dengan demikian, perubahan state hanya terjadi setelah candidate session lengkap berhasil.

## Evidence executable

`editor_scene_session_dirty_smoke` membuktikan open bersih, transform mutation yang membuat dirty, invalid transform yang tidak mengubah dirty state, `Save` dan `SaveBytes` yang mengembalikan clean, decode bytes rusak yang mempertahankan clean, delete mutation yang membuat dirty, serta Open document duplicate-actor yang mempertahankan dirty. Smoke tersebut lulus dalam Release dan AddressSanitizer dengan deteksi kebocoran aktif.

| Gate | Hasil final |
|---|---|
| `editor_scene_session_dirty_smoke` Release | Lulus; open clean, mutation dirty, save clean, dan failure rollback. |
| `editor_scene_session_dirty_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 114/114 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 114/114 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Tidak ada undo/redo, change journal, atomic file save, close-prompt desktop UI, filesystem watcher, conflict resolution, collaboration, shared project locking, asset hot reload, editor UI, APK/AAB, atau release readiness pada increment ini. Dirty state juga tidak mengubah authority gameplay, SceneWorld transform ownership, RouteIntent, MovementAuthorityGate, physics, atau agents.
