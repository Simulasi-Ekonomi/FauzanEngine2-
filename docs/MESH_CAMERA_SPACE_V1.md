# Mesh Camera-Space v1 — Orientation-Correct Clipping and Perspective Depth

## Ruang Lingkup

Setelah `RenderCamera` memperoleh orientasi `forward`/`up`, `MeshRenderer` masih menghitung near clipping dan reciprocal depth dari sumbu Z dunia. Itu benar hanya ketika kamera menghadap +Z. Milestone ini membuat renderer mesh memakai `RenderCamera::WorldToCamera` untuk near-plane clipping dan depth perspektif.

| Tahap mesh | Sebelum | Sesudah |
|---|---|---|
| Near clipping | `world.z >= camera.position.z + near`. | `cameraSpace.z >= near`. |
| Reciprocal depth | Berdasarkan selisih Z dunia. | Berdasarkan kedalaman camera-space. |
| Proyeksi | Telah memakai `RenderCamera::Project`. | Tetap sama, sekarang konsisten dengan clipping/depth. |

## Kontrak

`WorldToCamera` mengubah titik dunia memakai basis camera yang telah tervalidasi dan menolak world non-finite atau kamera yang belum siap. Mesh renderer memanggilnya saat clipping setiap triangle dan saat membangun vertex raster. Transform mesh, normal, material, staged PPM/BMP texture, directional light, depth test, dan culling yang sudah ada dipertahankan.

> Perubahan ini memperbaiki ruang koordinat renderer; ia tidak mengubah `SceneWorld`, transform entity, authority movement, asset registry, atau route skeletal.

## Bukti

`mesh_renderer_smoke` tetap membuktikan depth, culling, near clip, PPM/BMP staged texture, material light, dan rejection resource invalid. Ia sekarang juga membuat camera perspektif menghadap +X dan meraster triangle normal +X pada pusat framebuffer. `render_camera_smoke` tetap lulus. Kedua jalur lulus pada Release dan ASAN `detect_leaks=1`; suite non-Vulkan penuh mencapai **95/95 Release** dan **95/95 ASAN**.

## Batas

Tidak ada frustum clipping penuh selain near plane, matrix API publik, normal mapping, multiple light, specular/PBR, shadows, material graph, GPU raster, or scene-wide culling. Lighting directional Lambert yang ada tetap bounded software CPU.
