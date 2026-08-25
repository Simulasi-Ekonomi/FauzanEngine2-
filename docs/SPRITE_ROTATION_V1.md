# Sprite Rotation V1

`SpriteDraw::rotationRadians` menambahkan rotasi quad 2D finite di sekitar pusat sprite sebelum clipping frustum. Nilai harus finite; default `0` mempertahankan pemanggil lama. Alpha source-over, texture tint, UV, sorting stabil, clipping, dan candidate-frame atomic flush tetap menggunakan jalur yang sama.

`sprite_batch_smoke` membuktikan output quad `3×1` berbeda setelah rotasi π/2, di samping evidence clipping/alpha/tint/atomic existing. Target lulus Release serta ASAN `detect_leaks=1`; broad non-Vulkan lulus **96/96 Release** dan **96/96 ASAN**.

Ini bukan animation system, flipbook, atlas, skeletal animation, GPU batching, UI framework, game, APK, atau release evidence.
