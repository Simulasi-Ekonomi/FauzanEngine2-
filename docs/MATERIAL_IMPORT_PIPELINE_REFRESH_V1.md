# Material Import Pipeline Refresh v1

## Tujuan

Fase E.5a menambahkan `MaterialImportPipeline::RefreshMtl` untuk memperbarui bytes MTL dari asset material yang **sudah ada**. Operasi membuat salinan kandidat `AssetRegistry` serta `MaterialStagingStore`, mengganti bytes melalui registry canonical, memanggil `MaterialStagingStore::Refresh` untuk named material yang telah staged, lalu mengganti registry, store, dan receipt caller hanya jika seluruh jalur sukses.

> Refresh v1 adalah transaksi eksplisit in-memory yang dipanggil caller. Ia bukan filesystem watch, hot reload otomatis, atau live scene material rebinding.

## Kontrak kandidat/commit

| Tahap | Perilaku |
|---|---|
| Request | Asset ID, nama material, dan bytes replacement wajib tidak kosong. |
| Registry kandidat | `ReplaceBytes` mengganti bytes asset material yang telah ada. |
| Staging kandidat | `Refresh` memvalidasi kembali named material dari bytes kandidat dan mengganti CPU resource kandidat hanya setelah parse sukses. |
| Commit | Registry, staging store, content hash, RGBA receipt ditugaskan bersama setelah definition/resource cocok. |
| Failure | Asset hilang, request invalid, MTL malformed, atau material staged yang tak dapat disegarkan mempertahankan semua output caller. |

Operasi tidak menyentuh `SceneMeshAdapter` yang sudah copy-on-register. Karena itu refresh staging yang sukses sendiri belum mengubah material yang telah terikat di scene render existing.

## Evidence executable

`material_import_pipeline_smoke` sekarang mengimpor `farm`, lalu melakukan replacement ke diffuse `0.8/0.1/0.2` dengan dissolve `0.75`. Smoke membuktikan hash dan RGBA receipt berubah, resource staged current terhadap registry baru, refresh MTL malformed ditolak sambil mempertahankan receipt/resource current sebelumnya, dan refresh asset hilang ditolak tanpa mutation.

| Gate | Hasil final |
|---|---|
| `material_import_pipeline_smoke` Release | Lulus; replacement sukses dan rollback malformed/missing atomik. |
| `material_import_pipeline_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 120/120 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 120/120 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Tidak ada watcher filesystem, polling path, dependency graph refresh, automatic scene material rebinding, GPU material upload, shader binding, texture map refresh, cache disk, desktop editor UI, APK/AAB, atau release readiness pada increment ini.
