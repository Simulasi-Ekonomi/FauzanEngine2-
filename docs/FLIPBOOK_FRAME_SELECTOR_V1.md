# Flipbook Frame Selector v1

`FlipbookFrameSelector` memetakan sample finite `[0,1]` ke satu `SpriteSourceRect` pada grid atlas CPU. Konfigurasi memvalidasi dimensi texture/frame yang membagi tepat, kapasitas grid, dan maksimum 256 frame. Sample `1.0` dipetakan ke frame terakhir; sample invalid mempertahankan output caller.

Selector hanya menghasilkan rectangle frame-local. Ia tidak menyimpan SceneWorld, texture resource, transform, RouteIntent, MovementAuthorityGate, atau state gameplay.

| Gate | Hasil |
|---|---|
| `flipbook_frame_selector_smoke` Release/ASAN | Lulus; endpoint dan midpoint grid 2×2, konfigurasi invalid, serta preservasi output NaN. |
| Broad non-Vulkan | 124/124 Release dan 124/124 ASAN dengan `detect_leaks=1`. |

Ini bukan flipbook playback runtime lengkap, sprite animation event, skeletal animation, GPU upload, atau production readiness.
