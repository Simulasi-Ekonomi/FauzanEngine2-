# Farm Player Input Bridge v1 — Bounded Local Actions

## Tujuan

`FarmPlayerInputBridge` menghubungkan snapshot `InputState` ke aksi pemain yang telah tersedia di `FarmWorldTool`. Ia memperkenalkan kontrol grid dan interaksi crop yang **terbatas**, bukan sistem input game lengkap atau jalur bypass untuk ekonomi maupun governance.

| Input action | Perilaku | Target authority |
|---|---|---|
| `farm_move_up`, `farm_move_down`, `farm_move_left`, `farm_move_right` | Memindahkan karakter tepat satu tile cardinal per langkah input. | `FarmWorldTool::SetCharacterState` |
| `farm_interact` + aksi `Till` | Mengolah tile posisi karakter saat `justPressed`. | `FarmWorldTool::PlayerTill` |
| `farm_interact` + aksi `PlantWheat` | Menanam Wheat menggunakan inventori FarmSystem yang ada. | `FarmWorldTool::PlayerPlant` |
| `farm_interact` + aksi `Water` | Menyiram tile saat ini. | `FarmWorldTool::PlayerWater` |
| `farm_interact` + aksi `Harvest` | Memanen tile saat ini dan menyimpan hasil di jalur FarmSystem. | `FarmWorldTool::PlayerHarvest` |

## Batas dan kegagalan fail-closed

Bridge menuntut lima action yang berbeda dan valid sebelum pemrosesan. Movement diagonal/oposisi dinormalisasi menjadi tidak bergerak pada sumbu yang konflik; interaction yang digabung dengan movement pada frame yang sama ditolak. Perpindahan yang melewati batas world ditolak sebelum `FarmWorldTool` dimutasi. Aksi hanya dipicu pada `justPressed`, sehingga tombol yang tetap ditekan tidak membanjiri operasi farm.

> Bridge tidak menyediakan API untuk sell, top-up, issue permit, placement building, ban, policy, token, credential, deploy, atau agent command. Semua aksi yang ada tetap melewati validasi `FarmWorldTool` dan `FarmSystem`.

## Bukti lokal

`farm_player_input_bridge_smoke` mengikat lima input code, menggerakkan karakter satu tile, melakukan Till → PlantWheat → Water melalui input, lalu membuktikan penolakan kombinasi movement-plus-interact, penolakan koordinat di luar world, dan penolakan `InputState` dengan action tidak lengkap. Smoke lulus pada Release dan ASAN dengan `detect_leaks=1`; suite non-Vulkan penuh mencapai **92/92 Release** dan **92/92 ASAN**.

## Batas yang tersisa

Tidak ada keyboard mapping perangkat nyata, remap persistence, touch/gamepad profile, repeat rate, kamera, collision, building placement interaction, UI/hotbar/inventory view, feedback audio/visual, world save UI, networking, client prediction, multiplayer authority, Android build, atau gameplay loop yang dapat dipakai end-user. Kontrol ini hanya proof jalur input lokal ke API gameplay yang sudah memiliki batas authority.
