# Kinematic Collision Preflight V1

`KinematicCollisionPreflight` melakukan raycast XPBD read-only sebelum satu langkah kinematic. Ia membaca transform lokal actor, menormalisasi input planar, menghitung jarak langkah `unitsPerSecond × seconds` ditambah clearance, lalu melakukan `GameplayPhysicsQuery::Raycast` dengan mask eksplisit. Hit valid memblokir langkah tanpa mengubah transform; no-hit mendelegasikan write transform hanya kepada `KinematicMotionController`.

Preflight tidak memanggil `XPBDPhysicsSystem::Step`, tidak mengubah ECS, layer, constraint, atau velocity. Ia juga tidak mengklaim authority secara internal; caller tetap perlu memperoleh `MovementAuthorityGate` sebelum menjalankan controller. Query invalid atau mapping snapshot yang gagal bersifat fail-closed dan tidak meneruskan motion.

`kinematic_collision_preflight_smoke` membuktikan obstacle XPBD ber-layer static memblokir langkah dengan transform tidak berubah, snapshot kosong meloloskan langkah kinematic ke X=0.5, dan input NaN mempertahankan transform. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **108/108 Release** dan **108/108 ASAN**.

Ini bukan collision response, sinkronisasi scene-body, sweep capsule, 3D physics, navigation, pathfinding, root motion, multiplayer prediction, atau release evidence.
