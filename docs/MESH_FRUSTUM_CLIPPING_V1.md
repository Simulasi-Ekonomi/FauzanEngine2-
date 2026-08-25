# Mesh Frustum Clipping V1

## Scope

`MeshRenderer` kini melakukan clipping poligon Sutherland–Hodgman di **camera space** terhadap enam bidang frustum sebelum proyeksi dan raster: kiri, kanan, bawah, atas, near, serta far. Vertex hasil potong menginterpolasi world position, normal, UV, dan kemudian tetap melewati jalur lighting, material, texture, depth, serta candidate framebuffer yang sama.

> Perubahan ini menjadikan geometri di luar viewport sebagai **culling normal yang sukses**, bukan kegagalan `ProjectionFailed`. Ia tidak menambahkan scene-wide frustum culling atau menjadikan semua teknik clipping/raster production-ready.

| Bidang | Kondisi perspektif camera-space |
|---|---|
| Kiri / kanan | `-z·tan(fov/2)·aspect ≤ x ≤ z·tan(fov/2)·aspect` |
| Bawah / atas | `-z·tan(fov/2) ≤ y ≤ z·tan(fov/2)` |
| Near / far | `near ≤ z ≤ far` |
| Orthographic | Batas sisi berasal dari `orthographicHalfHeight` dan `aspect`, dengan near/far yang sama. |

Implementasi membatasi poligon hasil clip pada sembilan vertex. Triangle yang seluruhnya berada di luar satu bidang tidak diraster dan tidak dianggap error. Triangle yang memotong bidang diraster setelah ditriangulasi sebagai fan secara deterministik.

## Kontrak atomik

Validasi input, transform, texture, dan light tetap dilakukan sebelum candidate framebuffer dibuat. Setelah itu semua triangle berjalan pada salinan `SoftwareRenderer`; kegagalan proyeksi atau raster yang masih mungkin terjadi menghentikan `Draw` dan tidak mengganti framebuffer caller. Smoke sekarang membuktikan jalur atomik memakai triangle layar-degenerat yang memicu `RasterFailed`, bukan memakai geometri offscreen yang kini sah dicull.

| Kasus | Hasil kontrak |
|---|---|
| Triangle seluruhnya di kanan atau melewati far plane | `Draw` sukses, hash framebuffer tidak berubah. |
| Triangle melintasi side plane | `Draw` sukses dan menghasilkan pixel terklip. |
| Near-plane crossing sebelumnya | Tetap menghasilkan raster deterministik. |
| Triangle degenerat setelah sebuah triangle valid | `Draw` gagal `RasterFailed`; hash caller dipertahankan. |

## Evidence

`mesh_renderer_smoke` lulus pada Release dan ASAN dengan `ASAN_OPTIONS=detect_leaks=1`. Evidence mencakup right/far culling, side crossing, near clip, depth, culling winding, texture PPM/BMP, tint, directional intensity, oriented camera, dan atomic raster rejection. `mesh_surface_demo_smoke` serta executable `mesh_surface_demo --frames 8` juga lulus pada Release dan ASAN; demo finite sekarang memutar pose piramida secara terbatas per frame dan receipt frame akhir tetap memuat pixel non-latar.

Setelah perubahan ini, suite semua executable smoke root non-Vulkan lulus **96/96 Release** dan **96/96 ASAN** dengan `detect_leaks=1`. `vulkan_*` serta `sdl_audio_bridge_smoke` tetap dikecualikan oleh konvensi evidence yang terdokumentasi.

## Batas yang tetap terbuka

Perubahan ini bukan hierarchical/scene-wide frustum culling, occlusion culling, clipping user plane, guard-band rasterization, PBR, shadow, multi-light, GPU renderer, renderer desktop lengkap, skeletal animation, physics, game playable, APK, ataupun bukti rilis. Batas sembilan vertex adalah batas internal yang cukup untuk satu triangle yang dipotong oleh enam bidang; ia tidak membuat renderer menjadi pipeline clipping umum tanpa batas.
