# Material Refresh Surface Demo v1

## Tujuan

`MaterialRefreshSurfaceDemo` membuktikan bahwa material MTL yang telah staged dapat diperbarui secara eksplisit dari caller sampai output surface berubah. Demo mengimpor OBJ mesh dan material awal, mengikat keduanya ke `SceneMeshAdapter`, merender frame pertama, memanggil `MaterialImportPipeline::RefreshMtl`, lalu secara eksplisit memanggil `SceneMeshAdapter::RefreshStaged` sebelum merender frame berikutnya.

> Tidak ada watcher, polling, callback asset, atau auto-refresh. Perubahan terjadi hanya karena demo memanggil kedua operasi refresh secara berurutan dan keduanya berhasil.

## Kontrak jalur eksplisit

| Langkah | Kontrak |
|---|---|
| Initial bind | Mesh OBJ dan material `plane` staged diregistrasikan copy-on-register pada `SceneMeshAdapter`. |
| Initial frame | Adapter menggambar material awal `FF33CC33`; hash frame sebelum refresh direkam. |
| Replacement | `RefreshMtl` menjalankan registry/staging candidate transaction pada MTL baru. |
| Scene refresh | `RefreshStaged` memeriksa identitas mesh/material staged lalu mengganti instance CPU adapter bila kandidat valid. |
| Final frame | Adapter menggambar material baru `FFCC1A33`; hash frame setelah refresh direkam. |
| Artifact | Hanya frame final yang ditulis ke PPM dari software renderer. |

SceneWorld hanya menyediakan transform identity yang dibaca oleh adapter. Demo tidak memuat system gameplay, movement authority, physics, animation, atau renderer GPU.

## Evidence executable

`material_refresh_surface_demo_smoke` menolak konfigurasi satu frame, lalu memverifikasi 2 frame/2 present, non-background pixels, hash frame awal `4700326750445557667` berbeda dari hash final `1566030339519445923`, hash material sebelum/sesudah berbeda, RGBA `FF33CC33 → FFCC1A33`, dan PPM `P6` artifact. Target lulus pada Release dan AddressSanitizer dengan deteksi kebocoran aktif.

| Gate | Hasil final |
|---|---|
| `material_refresh_surface_demo_smoke` Release | Lulus; pre/post material dan pre/post framebuffer terukur. |
| `material_refresh_surface_demo_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 121/121 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 121/121 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Proof ini tidak menambahkan filesystem watcher, asset dependency graph update, automatic live scene reload, texture replacement binding, GPU resource refresh, shader/material graph, background task, persistent desktop host, APK/AAB, atau release readiness.
