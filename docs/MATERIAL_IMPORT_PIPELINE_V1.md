# Material Import Pipeline v1

## Tujuan

`MaterialImportPipeline` melengkapi seam texture dan mesh existing dengan transaksi MTL material **in-memory** yang bounded. Ia menerima bytes MTL dan nama material, memasukkan bytes sebagai `AssetKind::Material` pada `AssetRegistry` kandidat, menandainya ready, lalu memanggil `MaterialStagingStore::StageMtl` pada kandidat tersebut. Registry, store, dan receipt caller diganti hanya setelah seluruh langkah berhasil.

> Pipeline ini menggunakan parser dan staging material canonical. Ia bukan importer filesystem, resolver `mtllib`, hot reload, cache disk, resource GPU, atau material production pipeline.

## Kontrak kandidat/commit

| Langkah | Perilaku |
|---|---|
| Request | `assetId`, `materialName`, dan bytes MTL wajib tidak kosong. |
| Registry kandidat | Bytes diimpor sebagai material lalu wajib mencapai state `Ready`. |
| Staging kandidat | Satu named material diparse melalui `MaterialStagingStore::StageMtl`. |
| Receipt | Memuat asset ID, material name, content hash registry, dan RGBA CPU staged. |
| Commit | Registry, store, dan receipt caller diganti bersama hanya setelah definition serta resource kandidat ditemukan. |
| Failure | Invalid request, duplicate asset, malformed/out-of-range MTL, atau stage failure mempertahankan state caller sebelumnya. |

Tidak ada nama material kedua atau dependency material yang disintesis. Dependencies caller hanya diteruskan ke `AssetRegistry` canonical dan tetap melewati validasi registry-nya.

## Evidence executable

`material_import_pipeline_smoke` mengimpor material `farm` dari MTL dengan diffuse `0.2/0.4/0.6` dan dissolve `0.5`, lalu memverifikasi material registry ready, content hash linkage, staged resource, dan RGBA receipt. Smoke juga membuktikan MTL dengan `Kd 2 0 0` ditolak pada staging tanpa menambah registry/store atau mengganti receipt, duplicate asset ditolak pada registry, dan request ID/nama kosong ditolak.

| Gate | Hasil final |
|---|---|
| `material_import_pipeline_smoke` Release | Lulus; 1 asset, 1 material, receipt hash, dan rollback atomik. |
| `material_import_pipeline_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 118/118 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 118/118 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Pipeline ini belum mendukung filesystem import, `mtllib` dependency resolution, multiple named material transaction, texture map/PBR map, cache disk, watcher/hot reload, live mesh material rebinding, GPU upload/lifetime, shader binding, editor UI, APK/AAB, atau release readiness.
