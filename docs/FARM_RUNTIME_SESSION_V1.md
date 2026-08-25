# Farm Runtime Session v1 — Input, Simulation, and Sprite Frame

## Tujuan

`FarmRuntimeSession` adalah host-side lifecycle terbatas yang menyatukan komponen yang sudah memiliki kontrak sendiri: `InputState`, `FarmPlayerInputBridge`, `FarmWorldTool`, dan `FarmSpriteRenderAdapter`. Session tidak memiliki ekonomi, entity, world, asset, atau renderer; ia hanya menyimpan referensi pemanggil dan menjalankan urutan frame eksplisit.

| Urutan frame | Operasi | Authority yang tetap berlaku |
|---|---|---|
| 1 | `InputState::BeginFrame` dan `FarmPlayerInputBridge::Step` | Validasi action serta aksi karakter tetap melalui `FarmWorldTool`. |
| 2 | `FarmWorldTool::Tick` | Tick crop dan NPC tetap berada di simulation FarmWorld/FarmSystem. |
| 3 | `FarmSpriteRenderAdapter::RenderWorld` | Staging asset dan frame kandidat tetap berada pada adapter renderer. |
| 4 | Increment `frameCount` | Hanya terjadi setelah ketiga langkah berhasil. |

## Failure boundary

Session menolak frame dengan `simulationTicks == 0`, input yang tidak lengkap/tidak valid, tick world yang gagal, atau render yang gagal. Bila input atau parameter tick ditolak, tick world dan renderer tidak dijalankan sehingga frame renderer lama dipertahankan. Bila render gagal setelah tick world berhasil, state world sudah dapat berubah; session **tidak** mengklaim transactional rollback game state.

> Tidak ada command agent, economy sell/top-up, permit, ban, secret, network, persistensi, atau deployment API pada session ini. Pemanggil tetap harus menginisialisasi object Farm, asset registry, texture staging, dan renderer sebelum session siap.

## Bukti lokal

`farm_runtime_session_smoke` membuat FarmSystem/FarmWorldTool nyata, 16 texture PPM terdaftar, InputState terikat, lalu membuktikan input right bergerak satu tile, interaksi Till mengubah state tile melalui frame terpadu, hash frame berubah dengan state, `simulationTicks=0` ditolak tanpa mengganti frame, dan InputState tanpa action ditolak tanpa mengganti frame atau jumlah frame. Smoke lulus pada Release dan ASAN dengan `detect_leaks=1`; suite non-Vulkan penuh mencapai **93/93 Release** dan **93/93 ASAN**.

## Batas yang tersisa

Session ini bukan executable game player-facing. Belum ada platform event loop, window/present swapchain, keyboard perangkat nyata, audio, UI, save/load host lifecycle, animation, camera controller, physics interaction, multiplayer, anti-cheat client/server loop, Android packaging, atau recording gameplay. Ia hanya membuktikan satu jalur runtime lokal yang terhubung dari input terbatas ke simulation dan gambar sprite.
