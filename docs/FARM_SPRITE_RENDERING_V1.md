# Farm Sprite Rendering v1 — Bounded Textured Presentation

## Tujuan

`FarmSpriteRenderAdapter` menyediakan satu jalur presentasi 2D bertekstur untuk state nyata dari `FarmSystem` dan `FarmWorldTool`. Adapter ini **membaca** tile, building, NPC, dan karakter yang sudah dimiliki sistem simulasi; ia tidak menjalankan ekonomi, quest, permit, anti-fraud, AI agent, persistence, atau authority permainan.

| Input nyata | Efek presentasi | Kepemilikan tetap berada pada |
|---|---|---|
| `FarmSystem::TileStateAt` | Memilih texture empty, tilled, growing, atau harvestable. | `FarmSystem` |
| `FarmWorldTool::Buildings` | Memilih texture jenis building dengan layer 10. | `FarmWorldTool` |
| `FarmWorldTool::Npcs` | Memilih texture peran NPC dengan layer 20. | `FarmWorldTool` |
| `FarmWorldTool::Character` | Menampilkan texture pemain dengan layer 30. | `FarmWorldTool` |

## Kontrak asset dan frame

`FarmSpriteAssetSet` membutuhkan 16 ID asset texture eksplisit: empat state tile, enam tipe building, lima peran NPC, dan satu texture pemain. Setiap ID harus merujuk `AssetRegistry` asset `Texture` yang `Ready` dan dapat di-stage sebagai PPM P6 atau BMP BI_RGB. Tidak ada pencarian file, network, fallback placeholder, atau asset generation tersembunyi.

Sebelum menyentuh renderer target, adapter menyalin `TextureStagingStore`, memvalidasi/refresh semua 16 asset pada kandidat, membuat `SpriteBatch`, dan mengantrikan seluruh draw. Hanya setelah validasi tersebut, ia meraster ke `SoftwareRenderer` kandidat yang terpisah. Frame target diganti pada akhir operasi yang sukses.

> Rejection asset kosong, asset salah/tidak siap, decode gagal, world mismatch, atau kapasitas tidak menghapus frame renderer yang sudah ada dan tidak mengganti `TextureStagingStore` pemanggil.

## Kapasitas dan urutan

Jumlah draw dihitung sebagai `lebar × tinggi + building + NPC + pemain` dan ditolak sebelum queue apabila melampaui kapasitas `SpriteBatch` 2.048. Tile diberi layer 0, building 10, NPC 20, dan pemain 30; order di dalam layer dibuat deterministik dari koordinat atau ID bounded. Adapter tidak mengubah `FarmRenderAdapter` lama yang masih menyediakan referensi warna deterministik untuk test regresi.

## Bukti lokal

`farm_sprite_render_smoke` menginisialisasi `FarmSystem` 4×4 dan `FarmWorldTool`, menumbuhkan tile melalui `Till`/`Plant`/`Water`/`Tick`, menerbitkan serta menempatkan permit Barn, memindahkan karakter, dan mengimpor 16 PPM texture ke `AssetRegistry`. Smoke membuktikan staging 16 texture, hash frame deterministik saat render diulang, frame preservation ketika ID pemain dikosongkan, serta preservation ketika texture tile dibuat stale dan invalid. Smoke target lulus pada Release dan ASAN dengan `detect_leaks=1`; suite non-Vulkan penuh mencapai **91/91 Release** dan **91/91 ASAN**.

## Batas yang tersisa

Ini bukan UI yang dapat dimainkan dan bukan pembuktian game Farm rilis. Tidak ada input pemain, kamera pan/zoom, hit testing, tilemap/atlas/animasi sprite, alpha blend, clipping, GPU renderer, asset streaming, 1.024×1.024 grid, networking, multiplayer authority, Android APK/AAB, atau device proof. Walaupun objek NPC dan building berasal dari `FarmWorldTool`, visual ini belum memperlihatkan skeletal animation, path visualization, atau kecerdasan NPC sebagai presentation production.
