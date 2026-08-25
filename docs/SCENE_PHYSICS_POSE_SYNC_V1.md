# Scene Physics Pose Sync V1

`ScenePhysicsPoseSync` adalah seam satu arah dari `SceneWorld` ke komponen posisi ECS fisika. Caller mengikat `SceneEntity` unik ke `EntityID` fisika unik. Saat `Sync`, seluruh transform world dan entity dengan komponen position divalidasi serta dikumpulkan sebagai candidate sebelum setter ECS pertama dipanggil; kegagalan binding di tengah tidak menghasilkan write parsial.

Sync menyalin hanya X/Z. Ia tidak menulis `SceneWorld`, tidak menjalankan XPBD, tidak menarik pose dari physics kembali ke scene, dan tidak memperoleh movement authority. Setelah sync, caller dapat secara eksplisit menjalankan `XPBDPhysicsSystem::Step` untuk membangun snapshot query terbaru.

`scene_physics_pose_sync_smoke` membuktikan dua binding world-to-ECS, duplicate scene binding rejection, serta preservasi pose fisika pertama ketika binding kedua kehilangan transform world. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **110/110 Release** dan **110/110 ASAN**.

Ini bukan sync dua arah, interpolasi, ownership physics atas transform scene, rigidbody authority, 3D body sync, rollback network, atau release evidence.
