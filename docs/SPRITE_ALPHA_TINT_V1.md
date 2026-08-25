# Sprite Alpha and Tint v1 — Deterministic Software Composition

## Ruang Lingkup

`SpriteBatch` sudah memiliki sorting stabil berdasarkan `layer`, `order`, dan sequence. Milestone ini membuat metadata `SpriteDraw::rgba` benar-benar memengaruhi output pada kedua jalur sprite: warna flat dan sprite bertekstur. Alpha diperlakukan sebagai source-over deterministik pada framebuffer CPU, sedangkan texture bertekstur dimodulasi dengan tint RGBA sebelum komposisi.

| Jalur | Sebelum | Sesudah |
|---|---|---|
| Sprite warna flat | `rgba` ditulis overwrite. | `rgba` dikomposisikan source-over terhadap piksel tujuan. |
| Sprite bertekstur | `rgba` SpriteDraw diabaikan. | Texel RGBA dimodulasi oleh tint `rgba`, lalu dikomposisikan source-over. |
| Mesh/raw renderer | Tetap dapat memanggil renderer tanpa blend opt-in. | Default API tetap overwrite untuk pemanggil non-sprite. |

## Kontrak

Blend memakai aritmetika integer dengan rounding eksplisit per kanal dan output alpha framebuffer tetap opaque. `SpriteBatch` tidak mengubah sort order, kapasitas, staging texture, SceneSpriteAdapter ownership, atau SceneDocument contract. Texture harus tetap lolos validasi `CpuTextureResource` yang sudah ada; tidak ada loader format baru pada milestone ini.

> Alpha/tint ini berada di renderer software CPU. Tidak ada klaim premultiplied alpha, atlas, clipping quad, batching GPU, shader material, atau blend state Vulkan.

## Bukti

`sprite_batch_smoke` mempertahankan bukti layer hijau di atas merah, lalu membuktikan blend merah 50% di atas biru untuk sprite flat dan sprite bertekstur putih dengan tint RGBA yang sama. `software_renderer_smoke` kembali membuktikan UV perspektif serta lighting texture lama. Kedua smoke lulus pada Release dan ASAN `detect_leaks=1`; suite non-Vulkan penuh mencapai **95/95 Release** dan **95/95 ASAN**.

## Batas

Tidak ada atlas/flipbook, animation sprite, sampler modes, alpha testing, clipping, z-sort transparency kompleks, render target, GPU acceleration, atau authoring UI receipt baru. Ketika dua triangle quad berbagi boundary raster, semantics tetap mengikuti coverage software renderer yang ada; milestone ini tidak mencoba mengganti aturan rasterisasi tersebut.
