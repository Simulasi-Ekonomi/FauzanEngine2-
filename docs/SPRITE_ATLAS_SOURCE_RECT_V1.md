# Sprite Atlas Source Rectangle v1

`SpriteDraw` dan `SceneSpriteAdapter::AddStaged` kini menerima rectangle source CPU opsional. Rectangle nol mempertahankan sampling full-texture sebelumnya. Rectangle nonnol harus berada sepenuhnya dalam bounds texture; rectangle parsial, out-of-bounds, atau rectangle pada sprite tanpa texture ditolak sebelum masuk batch.

UV rectangle diterapkan sebelum clipping frustum sehingga hasil clip tetap menginterpolasi UV atlas yang benar. `SpriteBatch::Flush` tetap mengurutkan layer/order dan merender ke framebuffer kandidat sebelum commit, sehingga penolakan tidak memodifikasi framebuffer caller.

| Gate | Hasil |
|---|---|
| `sprite_atlas_source_rect_smoke` Release | Lulus; frame atlas merah dan biru masing-masing menghasilkan 144 pixel. |
| `sprite_atlas_source_rect_smoke` ASAN | Lulus dengan `detect_leaks=1`. |
| Broad non-Vulkan | 123/123 Release dan 123/123 ASAN lulus. |

Tidak ada GPU atlas upload, texture streaming, sprite editor UI, flipbook timeline, skeletal animation, atau production readiness claim pada increment ini.
