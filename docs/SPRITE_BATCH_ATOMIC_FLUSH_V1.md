# Sprite Batch Atomic Flush v1 — Framebuffer Preservation on Failure

## Ruang Lingkup

Sebelum milestone ini, `SpriteBatch::Flush` meraster setiap sprite langsung ke `SoftwareRenderer` pemanggil. Jika sprite berikutnya gagal proyeksi atau raster, frame dapat berisi hasil parsial. Flush sekarang membuat salinan framebuffer kandidat, meraster seluruh batch ke kandidat, lalu mengganti framebuffer pemanggil hanya setelah seluruh sprite berhasil.

| Kondisi | Perilaku |
|---|---|
| Semua sprite valid | Kandidat dipindahkan ke renderer pemanggil. |
| Projection gagal | `ProjectionFailed`; framebuffer pemanggil tidak berubah. |
| Raster gagal | `DrawFailed`; framebuffer pemanggil tidak berubah. |
| Queue invalid | Tetap ditolak sebelum flush dengan error queue lama. |

## Kontrak dan batas

Sorting `layer → order → sequence`, alpha/tint, texture validation, dan count queued sprite tidak berubah. Atomicity hanya mencakup isi `SoftwareRenderer` saat flush; ia tidak membuat SceneWorld, asset staging, atau state gameplay transactional. Copy framebuffer bersifat bounded oleh batas `SoftwareRenderer` aktif (maksimum 16 juta piksel), sehingga tidak ada IO atau authority baru.

## Bukti

`sprite_batch_smoke` menambahkan batch yang memiliki satu sprite valid lalu satu sprite di luar frustum. Flush ditolak dengan `ProjectionFailed`, dan hash framebuffer pemanggil terbukti sama sebelum dan sesudah penolakan. Bukti layer/order serta alpha/tint tetap lulus. Smoke lulus di Release dan ASAN `detect_leaks=1`; suite non-Vulkan penuh mencapai **95/95 Release** dan **95/95 ASAN**.

## Batas

Tidak ada incremental dirty-region copy, render command buffer GPU, texture upload transaction, atau atomicity lintas `MeshRenderer` dan `SpriteBatch`. Milestone ini fokus pada fail-closed surface yang sudah aktif.
