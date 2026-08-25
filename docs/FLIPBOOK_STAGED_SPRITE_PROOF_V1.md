# Flipbook Staged Sprite Proof v1

`SceneSpriteAdapter::QueueFrame` menerima `SpriteSourceRect` hanya saat batch frame dibentuk. Ia membaca transform `SceneWorld`, meneruskan rectangle ke `SpriteBatch`, dan tidak menyimpan rect sebagai state world maupun menulis transform.

Smoke atlas memilih frame akhir atlas biru melalui `FlipbookFrameSelector`, mengirimkannya ke staged adapter, lalu membuktikan 64 pixel biru rendered dan transform actor tetap tidak berubah. Input rect invalid tetap ditolak oleh batch.

| Gate | Hasil |
|---|---|
| Target Release/ASAN | `sprite_atlas_source_rect_smoke` lulus, termasuk selector → QueueFrame staged sprite. |
| Broad non-Vulkan | 124/124 Release dan 124/124 ASAN dengan `detect_leaks=1`. |

Proof ini bukan skeletal animation, runtime flipbook event system, GPU animation, atau production readiness.
