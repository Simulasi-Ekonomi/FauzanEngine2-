# FauzanEngine Authoring Capability Audit — 23 Agustus 2026

## Metode audit

Status hanya dihitung bila source tercantum pada `Source/NeoEngine/CMakeLists.txt` dan memiliki jalur test/smoke aktif. File yang berada di direktori kanonis tetapi tidak masuk `XPBD_RUNTIME_SOURCES` diperlakukan sebagai legacy/inactive, bukan sebagai engine capability. Audit ini tidak menganggap nama kelas, header, atau Coba Memory sebagai bukti implementasi.

| Capability yang diminta | Bukti aktif saat ini | Status jujur |
|---|---|---|
| Skeleton dan rig | `AuthoringCatalog` kanonis menyimpan skeleton definition bounded dengan maksimal 64 bone, satu root, parent terdahulu, serta validasi duplicate/hierarchy. | **Definition aktif.** Tidak ada skeletal pose, skinning, GLTF import, atau animation runtime kanonis. |
| Character authoring | `AuthoringCatalog` memvalidasi character terhadap skeleton/material serta health/stamina; FarmCharacterState tetap terpisah untuk Farm. | **Definition bounded aktif.** Belum blueprint visual, rig animation, equipment visual, combat, atau generic persistence. |
| Building authoring | `FarmWorldTool` memiliki permit, type, place/remove building, serialisasi dan binding SceneWorld. | **Aktif untuk Farm bounded.** Tidak ada blueprint/prefab building generic, mesh/material, upgrade graph, atau construction animation. |
| Scene authoring | `AuthoringCatalog` menyimpan hingga 16 scene dan 512 placement; actor/building scene dapat di-bind serta ditick di SceneWorld milik NeoRuntime. | **Aktif sebagai transform scene bounded.** Tidak ada prefab/component schema, streaming, editor, navmesh, atau renderer scene graph. |
| Item authoring | `AuthoringCatalog` menyediakan class/material/stack/weight definition; `RpgSandboxGame` tetap mempunyai item sandbox sendiri. | **Definition bounded aktif.** Tidak ada visual binding, crafting data, serial persistence, atau authority service. |
| Narrative | `AuthoringCatalog` memvalidasi content key dan hingga empat branch destination yang telah diketahui. | **Definition bounded aktif.** Tidak ada localization, trigger state, dialogue UI, voice, atau save progression. |
| NPC movement/behavior | Farm NPC memiliki role/quest grid; AuthoringCatalog menambahkan idle/patrol/wander/chase-origin deterministic untuk actor definition; `KinematicMotionController` menyediakan langkah planar continuous SceneWorld yang terpisah dengan speed/duration bounded dan input ter-normalisasi. | **Aktif terbatas.** Tidak ada coupling controller ke route/behavior, pathfinding dinamis, collision/avoidance, perception, combat AI, animation integration, atau multi-world scheduling. |
| Monster movement/behavior | Actor monster authored memakai behavior deterministic dan transform scene; RPG respawn/drop tetap terpisah. | **Aktif terbatas.** Tidak ada combat/targeting/pathfinding/animation monster. |
| Kekerasan/material fisik | AuthoringCatalog memvalidasi hardness/friction/restitution/density dan menghasilkan contact response deterministic. | **Definition/contact contract aktif.** Belum terhubung ke solver XPBD collider, destruction, audio, atau renderer material. |
| Self-repair | `AgentAutonomyPolicy` mengizinkan diagnosis dan draft repair plan hanya dry-run. | **Boundary aktif, bukan repair otomatis.** Tidak ada patch executor, sandbox worker, rollback artifact, atau safe recovery service. |
| Self-learning | Policy mengizinkan proposal learning-evaluation hanya dry-run. | **Belum learning aktif.** Tidak ada governed training data, evaluation runner, model registry, drift control, atau player-impact authorization. |
| Self-deploy | Policy menolak deploy/publish serta source/runtime/authority mutation. | **Dilarang secara default dan diuji.** Tidak ada agent yang boleh deploy, publish, mengubah runtime/economy/ban, atau memakai credential. |

## Prioritas engineering

Prioritas berikutnya adalah membuat **domain authoring yang kecil, versioned, deterministic, dan reusable**, bukan mencoba menyalakan semua source legacy. Fondasi minimal mencakup definition bagi rig/skeleton, character, building, item, narrative, actor NPC/monster, scene placement, dan collision material; semua harus melalui validation limit, serialize/deserialize, SceneWorld propagation, serta smoke Release/AddressSanitizer. Setelah itu barulah animation/render authoring, navmesh, combat AI, generic editor, persistence service, dan Android client dapat dinilai sebagai pekerjaan terpisah.

## Batas agent

"Self-repair" dalam tahap ini berarti agent dapat mengumpulkan evidence, mengklasifikasikan kegagalan, dan menerbitkan rencana diagnosis/test/rollback typed. "Self-learning" harus tetap offline, memakai data yang disetujui, menghadapi evaluation gate, dan tidak mengubah gameplay/live economy secara otomatis. "Self-deploy" tidak diaktifkan: deploy/publish memerlukan artifact yang telah lulus, secret yang dikelola aman, approval eksplisit, dan operasi yang dapat dibatalkan.

## Evidence navigation dan world terbaru

`GridNavigation` kini menjadi source kanonis dengan BFS grid 4-arah bounded, route reconstruction, obstacle query, dan error fail-closed untuk endpoint terblokir atau tujuan tak terjangkau. Character, NPC, dan Monster actor authored dapat menerima goal eksplisit dan memprioritaskan route tersebut sebelum kembali ke behavior typed. Smoke Release/AddressSanitizer membuktikan detour obstacle dan propagasi transform SceneWorld; belum ada dynamic replanning, navmesh 3D, steering, avoidance, perception, combat, animation, prediction client, atau authority multiplayer.

`GridRouteFollower` sekarang dapat mengeksekusi route `GridCell` empat-arah yang sudah tervalidasi sebagai perpindahan continuous di atas `KinematicMotionController`, dengan maksimum 512 cell, start-transform checking, chunk yang dibatasi controller, snapping arrival presisi, dan revalidasi current/target cell terhadap `GridNavigation` sebelum mutasi. Bila target baru diblokir, caller dapat meminta replan bounded dari cell terakhir yang telah dicapai menuju final goal yang tersimpan. Smoke Release/AddressSanitizer membuktikan route `(0,0)→(1,0)→(1,1)` tiba tepat melalui empat chunk, menolak cell tidak adjacent, transform awal tidak sesuai, dan obstacle yang ditambahkan setelah route dibuat tanpa mengubah transform, lalu menemukan detour deterministik ke goal. Ini tetap follower lokal, bukan nav agent otomatis: tidak ada trigger/policy replanning mandiri, collision/avoidance, integrasi otomatis dengan actor authored/Farm NPC, steering, navmesh 3D, prediction, atau authority multiplayer.

`WorldAuthoring` membentuk biome water/forest/stone/meadow dari seed deterministic, tree/forest bounded, occupancy, obstacle navigation, footprint building, serialisasi atomic, dan scene entity. Building setelah scene bind juga menambah entity secara atomik. `NeoRuntime` kini memilikinya; vertical-slice smoke membuktikan forest runtime 121 tree, satu building, character route, NPC, monster, item, dan narrative definition. Semua evidence tersebut tetap **bounded headless simulation**: tidak ada terrain/foliage renderer, skeletal animation, gameplay combat, editor, streaming world, persistent server, atau game released.

`AgentManualRepairProtocol` memerlukan flag perintah eksplisit pengguna, ID terbatas, scope allowlist, dan symptom aman sebelum menghasilkan diagnosis key, patch-scope key, serta target regression. Perintah repair tersirat atau request deploy/publish/credential/economy/ban/session ditolak; protokol tidak mengubah code, menjalankan shell, melakukan deployment, maupun memperoleh authority runtime atau pemain.
