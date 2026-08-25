# Gameplay Physics Query V1

`GameplayPhysicsQuery` memberi seam raycast 2D read-only di atas snapshot collider datar aktif milik `XPBDPhysicsSystem`. `GameplayRay2` menerima origin/direction XZ, max distance, dan collision mask; `GameplayRayHit2` memetakan hit dari indeks solver internal menjadi `EntityID`, distance, dan normal XZ.

Query menolak origin, direction, atau distance yang tidak finite; direction nol; distance negatif; dan mask `COLLISION_LAYER_NONE`. Untuk miss, mapping yang gagal, atau input invalid, object hit caller tidak ditulis. Wrapper tidak memanggil `Step`, tidak mengubah layer, constraint, ECS, velocity, atau solver configuration. `TryGetEntityId` pada XPBD hanya membaca mapping index snapshot aktif.

`OverlapCircle` menambahkan query circle XZ bounded atas `OverlapSphere` XPBD. Ia menolak center/radius non-finite, radius negatif, dan mask kosong; hasil dibatasi maksimal 256 `EntityID`. Empty overlap adalah query sukses dengan output kosong, sedangkan invalid input atau kegagalan mapping mempertahankan vector caller sebelumnya.

`gameplay_physics_query_smoke` membuktikan raycast nearest-hit, mapping `EntityID`, mask layer yang memfilter hit, input NaN, miss, invalid mask, overlap circle, empty overlap, serta preservasi output pada kegagalan. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **102/102 Release** dan **102/102 ASAN**.

Ini hanya query circle-collider XZ. Ia bukan 3D physics, Rigidbody API, authoring collider, trigger/event, sweep, raycast scene editor, CCD gameplay contract, atau release evidence.
