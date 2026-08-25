# EditorSceneSession Revert v1

## Tujuan

Fase B.4b menambahkan `RevertToSaved(const AssetRegistry&)` pada `EditorSceneSession`. Operasi ini mengembalikan session ke **snapshot document in-memory terakhir yang berhasil dibuka atau disimpan**. Ia menggunakan jalur candidate Open yang sama seperti load biasa, bukan dengan menyalin sebagian transform atau state runtime secara langsung.

> Revert v1 hanya memiliki satu baseline snapshot. Ini bukan undo/redo history, project persistence, autosave, atau recovery filesystem.

## Kontrak

| Kondisi | Perilaku |
|---|---|
| `Open` / `OpenBytes` berhasil | Document yang berhasil dibangun menjadi snapshot clean baru. |
| `Save` / `SaveBytes` berhasil | Document aktif menjadi snapshot clean baru setelah output caller berhasil ditulis/di-encode. |
| `RevertToSaved` berhasil | Snapshot clean dibangun ulang melalui `OpenCandidate(..., true)`; session kembali clean. |
| Revert gagal | Session aktif, dirty status, saved snapshot, SceneWorld, binders, dan renderer sebelumnya tetap utuh. |
| Session belum terbuka | Revert ditolak dengan `InvalidDocument`. |

Revert tetap melakukan asset validation penuh ketika kandidat dibangun. Karena itu asset yang pernah siap tetapi kemudian tidak dapat dibind dapat membuat revert gagal secara fail-closed tanpa mengubah session yang sedang diedit.

## Evidence executable

`editor_scene_session_revert_smoke` membuktikan transform mutation dapat dikembalikan ke snapshot open, snapshot yang diperbarui oleh `Save` dapat memulihkan penghapusan actor berikutnya, dan replacemen texture bytes yang rusak membuat reversion snapshot sprite gagal dengan `SpriteBindFailed` sambil mempertahankan transform dirty aktif. Smoke juga menguji reversion session kosong ditolak.

| Gate | Hasil final |
|---|---|
| `editor_scene_session_revert_smoke` Release | Lulus; restore open snapshot, save snapshot, dan failure atomic dibuktikan. |
| `editor_scene_session_revert_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 115/115 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 115/115 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Tidak ada multi-level undo/redo, command journal, selective revert, filesystem save, transaction log, UI close prompt, collaboration conflict resolution, editor desktop UI, asset hot reload, GPU runtime, APK/AAB, atau release readiness pada increment ini. Revert tidak mengubah authority gameplay, physics, RouteIntent, MovementAuthorityGate, atau agents.
