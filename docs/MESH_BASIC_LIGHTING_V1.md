# Mesh Basic Lighting v1 — Directional Intensity and Material Tint

## Ruang Lingkup

`MeshRenderer` telah memiliki ambient plus directional Lambert CPU. Milestone ini menambahkan scalar `DirectionalLight::intensity` dengan batas `[0, 8]`, sehingga kekuatan directional light dapat disetel eksplisit tanpa mengubah determinisme rumus Lambert. Selain itu, `MeshMaterial::rgba` kini diteruskan sebagai tint untuk mesh bertekstur, sehingga material tidak diabaikan ketika texture aktif.

| Input | Perilaku |
|---|---|
| `ambient` | Tetap basis cahaya material yang tidak bergantung arah. |
| `directional` | Tetap bobot Lambert material. |
| `DirectionalLight::intensity` | Mengalikan komponen directional sebelum clamp 0–1. |
| `MeshMaterial::rgba` + texture | Memodulasi texel RGBA pada renderer software. |

## Validasi dan kompatibilitas

Intensity non-finite, negatif, atau di atas 8 ditolak sebagai `InvalidLight`. Direction nol tetap ditolak. Arah light, normal, staged PPM/BMP texture, material staging, depth, clipping camera-space, dan culling yang ada tidak diubah. Karena texture sekarang dihormati oleh tint material, dua smoke authoring yang dahulu mengunci texel merah murni diperbarui untuk memastikan channel merah tetap terlihat setelah tint material staged diterapkan.

> Ini adalah lighting Lambert CPU terbatas. Tidak ada multi-light, warna light, specular, normal map, shadow, PBR, light probe, atau renderer GPU.

## Bukti

`mesh_renderer_smoke` membuktikan texture putih ditint merah oleh material serta directional white dengan intensity 0,5 menghasilkan gray `0xFF808080`; ia juga membuktikan intensity 9 ditolak. `authoring_catalog_visual_binder_smoke` dan `editor_scene_mesh_binder_smoke` tetap membuktikan jalur mesh/material/texture authoring setelah asersi output diselaraskan dengan tint material yang benar. Semua smoke terkait lulus di Release dan ASAN dengan `detect_leaks=1`; suite non-Vulkan penuh mencapai **95/95 Release** dan **95/95 ASAN**.

## Batas

Material texture alpha saat ini mengikuti primitive software renderer; tidak ada blending mesh transparan khusus atau sort transparency mesh. Intensity adalah scalar, bukan sistem lingkungan atau per-light scene authoring.
