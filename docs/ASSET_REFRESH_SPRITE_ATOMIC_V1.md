# Asset Refresh Sprite Atomic V1

Dokumen ini mencatat baseline E.7b untuk `AssetRefreshExecutor::ExecuteSpritesAtomic`: plan texture-plus-sprite dijalankan pada salinan `TextureStagingStore` serta `SceneSpriteAdapter`, lalu kedua store dikomit hanya setelah seluruh action berhasil. E.7c mempertahankan kontrak atomik ini dan menambahkan diagnostik terpisah untuk menghasilkan plan bounded multi-sprite; lihat `ASSET_REFRESH_SPRITE_DIAGNOSTICS_V1.md`.

`asset_refresh_executor_smoke` membuktikan candidate-copy texture-plus-sprite dan mempertahankan jalur mesh refresh yang ada. Evidence E.7c mencakup Release, ASAN `detect_leaks=1`, dan broad non-Vulkan 140/140 pada kedua konfigurasi.

Ini bukan filesystem watch, GPU resource refresh, editor UI, atau hot reload produksi.
