# Asset Refresh Sprite Atomic V1

`AssetRefreshExecutor::ExecuteSpritesAtomic` menjalankan plan manual yang hanya berisi `RefreshTexture` dan `RefreshSpriteInstance` pada salinan `TextureStagingStore` serta `SceneSpriteAdapter`. Binding sprite diperiksa terhadap asset ID dan texture snapshot; kandidat texture harus current sebelum instance refresh. Kedua store baru dikomit setelah seluruh action berhasil.

`asset_refresh_executor_smoke` membuktikan refresh texture lalu instance sprite pada candidate-copy, serta mempertahankan jalur mesh refresh yang ada. Release, ASAN `detect_leaks=1`, dan broad non-Vulkan lulus 140/140.

Ini bukan auto-planning diagnostics untuk sprite, filesystem watch, GPU resource refresh, editor UI, atau hot reload produksi.
