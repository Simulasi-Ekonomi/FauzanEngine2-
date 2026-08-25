# Mesh Double-Sided Material V1

`MeshMaterial::cullBackFaces` sudah menjadi kontrak eksplisit renderer software: nilai default `false` mempertahankan raster dua-arah, sedangkan `true` mencull triangle ber-winding belakang setelah clip/proyeksi. Mode ini tidak mengubah depth, lighting directional, material tint, texture, atau candidate framebuffer atomik.

`mesh_renderer_smoke` membuktikan triangle winding belakang tetap terlihat dengan material default, tetapi tidak terlihat dengan `cullBackFaces=true`. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **96/96 Release** dan **96/96 ASAN**.

Ini bukan PBR, normal-map two-sided shading, shadow, GPU rasterizer, game, APK, atau release evidence.
