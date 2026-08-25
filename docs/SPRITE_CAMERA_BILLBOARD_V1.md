# Sprite Camera Billboard V1

`SpriteDraw::faceCamera` adalah opt-in billboard yang membangun quad dari basis `RenderCamera::Right()` dan `RenderCamera::Up()` tervalidasi. Rotasi lokal diterapkan sebelum basis kamera; default `false` mempertahankan quad world-plane lama.

`sprite_batch_smoke` membuktikan kamera perspective menghadap `+X` meraster sprite billboard pada `x=5`. Alpha/tint, UV, sorting, clipping frustum, dan atomic flush tetap berada pada jalur sama. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **96/96 Release** dan **96/96 ASAN**.

Ini bukan animation, spherical impostor, GPU billboard batching, scene culling, UI framework, game, APK, atau release evidence.
