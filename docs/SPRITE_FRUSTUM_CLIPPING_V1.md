# Sprite Frustum Clipping V1

## Scope

`SpriteBatch::Flush` sekarang memotong quad sprite pada camera space terhadap enam bidang frustum: kiri, kanan, bawah, atas, near, dan far. Bentuk hasil clip menyimpan posisi world dan UV, lalu ditriangulasi sebagai fan sebelum memasuki jalur alpha source-over atau texture tint yang sudah ada.

> Sprite seluruhnya di luar frustum adalah **culling normal yang sukses**. Ia tidak lagi menyebabkan seluruh flush gagal hanya karena salah satu sudut quad berada di luar clip range.

| Perilaku | Kontrak |
|---|---|
| Quad seluruhnya di luar viewport | Tidak menghasilkan pixel; `Flush` sukses dan candidate framebuffer tetap identik. |
| Quad melintasi sisi viewport | Hanya wilayah inside yang diraster dengan UV terinterpolasi. |
| Quad texture | Reciprocal depth dihitung per vertex hasil clip pada mode perspektif. |
| Quad flat | Tetap memakai alpha source-over untuk setiap triangle hasil fan. |
| Kegagalan raster nyata | `Flush` gagal `DrawFailed`; candidate tidak di-commit sehingga framebuffer caller tetap utuh. |

Polygon hasil clip dibatasi sampai sepuluh vertex. Batas itu mencakup quad yang berpotensi bertambah titik setelah enam bidang frustum dan tidak mengubah batas global 2.048 sprite batch.

## Evidence

`sprite_batch_smoke` lulus Release dan ASAN dengan `ASAN_OPTIONS=detect_leaks=1`. Evidence mencakup sorting layer/order/sequence, alpha/tint flat dan texture, culling quad seluruhnya di luar viewport, raster quad yang melintasi sisi viewport, dan atomic rejection menggunakan quad degenerat yang benar-benar memicu `DrawFailed`.

Setelah perubahan ini, semua executable smoke root non-Vulkan lulus **96/96 Release** dan **96/96 ASAN** dengan `detect_leaks=1`. `vulkan_*` serta `sdl_audio_bridge_smoke` tetap berada di luar angka tersebut sesuai konvensi evidence yang terdokumentasi.

## Batas yang tetap terbuka

Ini bukan sprite atlas, flipbook/animation, rotation, nine-slice, clipping mask, sampler mode, render target, GPU batching, transparency sorting kompleks, culling scene-wide, UI layout, IME, accessibility, game playable, APK, ataupun readiness rilis. Clipping hanya memecahkan batas lokal satu quad dalam renderer software aktif.
