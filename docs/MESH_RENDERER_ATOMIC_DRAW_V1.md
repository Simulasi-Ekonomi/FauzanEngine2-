# Mesh Renderer Atomic Draw v1 — Framebuffer Preservation on Mesh Failure

## Ruang Lingkup

`MeshRenderer::Draw` dapat meraster beberapa triangle. Sebelum milestone ini, triangle awal dapat ditulis ke framebuffer sebelum triangle berikutnya gagal projection atau raster. Draw sekarang memakai salinan `SoftwareRenderer` kandidat dan hanya memindahkan kandidat ke renderer pemanggil setelah seluruh mesh lolos.

| Kondisi | Perilaku |
|---|---|
| Semua triangle valid | Kandidat menjadi framebuffer baru. |
| Projection/clipping gagal pada triangle berikutnya | `ProjectionFailed`; framebuffer pemanggil tetap. |
| Raster gagal | `RasterFailed`; framebuffer pemanggil tetap. |
| Input/material/light invalid | Ditolak sebelum kandidat diraster. |

## Kontrak

Atomicity mencakup pixel dan depth buffer `SoftwareRenderer` untuk satu panggilan `MeshRenderer::Draw`. Transform, normal, camera-space clipping, staged material/texture, material tint, intensity lighting, culling, dan SceneWorld tidak dimutasi oleh mekanisme kandidat ini. Ukuran copy dibatasi oleh kontrak `SoftwareRenderer` yang telah ada.

## Bukti

`mesh_renderer_smoke` membangun mesh dengan triangle valid lalu triangle kedua di luar frustum. Panggilan ditolak sebagai `ProjectionFailed` dan hash framebuffer pemanggil terbukti tidak berubah. Bukti texture PPM/BMP, tint material, intensity lighting, culling, depth, near clipping, serta oriented camera tetap lulus. Smoke lulus pada Release dan ASAN `detect_leaks=1`; suite non-Vulkan penuh mencapai **95/95 Release** dan **95/95 ASAN**.

## Batas

Tidak ada atomicity lintas beberapa pemanggilan draw, sprite + mesh composition, asset staging, scene mutation, GPU command buffer, atau submission Vulkan. Copy framebuffer kandidat adalah pilihan correctness bounded, bukan optimasi high-performance GPU.
