# Scene Sprite Refresh V1

`SceneSpriteAdapter::RefreshStaged` mengganti snapshot `CpuTextureResource` untuk tepat satu entity sprite yang sudah ada. Entity harus ditemukan, texture pengganti harus valid, dan source rect metadata yang telah ada harus tetap kompatibel dengan dimensi baru. Pada keberhasilan, width/height display, layer, order, tint, transform behavior, source rect, serta metadata lainnya tidak berubah.

Validasi selesai sebelum texture snapshot instance ditulis. Karena itu texture malformed atau source rect yang tidak lagi muat ditolak sambil mempertahankan frame instance sebelumnya. `editor_scene_sprite_binder_smoke` membuktikan byte registry baru dapat di-stage lalu merender warna baru, dan refresh malformed mempertahankan hash frame warna baru.

Ini bukan file watcher, registry refresh otomatis, batch multi-sprite atomic, GPU texture rebinding, UI editor, atau hot reload produksi.
