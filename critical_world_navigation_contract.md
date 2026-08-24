# FauzanEngine Critical Navigation and World Authoring Contract

## Scope

Kontrak ini menutup dua gap yang benar-benar kritis: actor membutuhkan route yang menghindari obstacle, dan world membutuhkan terrain/forest/building authoring yang deterministic serta bounded. Ini bukan terrain renderer, world streaming, navmesh 3D, atau game yang siap rilis.

| Subsystem | Kontrak bounded | Bukti yang diwajibkan |
|---|---|---|
| `GridNavigation` | Grid 4–128 sel per sisi, obstacle set/query, BFS 4-arah, route pendek yang direkonstruksi, dan penolakan start/goal terblokir atau tak terjangkau. | Deterministic route, obstacle detour, dan penolakan invalid/unreachable. |
| `WorldAuthoring` | Terrain biome dari seed, water blocked, forest/tree placement, building footprint/occupancy, count limits, scene entities, dan serialized state. | Seed yang sama menghasilkan state sama; tree/building valid; scene binding dan navigation obstacle terverifikasi. |
| `AuthoringCatalog` actor | Character, NPC, dan monster actor dapat menerima goal eksplisit lalu mengikuti route bounded ke grid destination yang valid. | Ketiga kind bergerak menuju goal dan transform SceneWorld ikut berubah tanpa melewati obstacle. |
| Vertical slice | One non-UI world menggabungkan terrain/forest/building, navigation, scene, actor definition, movement, dan renderer-facing transforms. | Smoke Release/ASAN; bukan client, multiplayer, atau content-complete game. |

## Safety and determinism

Seed `0`, world dimensi di luar batas, occupied/water placement, duplicate placement, route lebih panjang dari capacity, atau deserialize corrupt harus gagal tertutup. World generator tidak mengambil network data, tidak mengeksekusi prompt, dan tidak membuat data pemain. Building footprint menjadi obstacle; tree dapat menjadi visual/occupancy feature sesuai parameter generator, tetapi tidak memberi authority terhadap economy atau game state remote.

## Repair manual agent

Perintah repair manual harus berupa request typed dengan ID, target scope allowlist, diagnosis, dan proposed patch/test plan. Responsnya adalah proposal auditable; request tidak menjalankan shell, memodifikasi source, memanggil deployment, menyentuh credential, atau mengubah runtime/economy/ban/session. Approval eksplisit masa depan baru boleh mengizinkan worker tersandbox yang menjalankan operasi allowlisted dan membuktikan hasilnya; kontrak ini tidak membangun worker tersebut.
