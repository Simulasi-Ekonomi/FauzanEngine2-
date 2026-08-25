# Gameplay Physics Body V1

`GameplayPhysicsBodyBuilder` membuat satu circle body XZ melalui komponen ECS kanonis `Position`, `Velocity`, dan `Collider`. Konfigurasi `Dynamic` memerlukan mass finite positif lalu mengisi inverse mass `1/mass`; konfigurasi `Static` hanya menerima velocity nol dan mengisi inverse mass nol. Kedua tipe memerlukan pose/velocity finite dan radius finite positif.

Validasi selesai sebelum `CreateEntity`; konfigurasi invalid tidak menciptakan entity baru serta tidak mengganti parameter `EntityID` caller. Builder tidak memiliki `XPBDPhysicsSystem`: caller sendiri menentukan kapan memanggil `Step`, bagaimana mengatur collision layer setelah snapshot fisika tersedia, bagaimana menghancurkan entity, dan bagaimana mengelola movement authority.

`gameplay_physics_body_smoke` membuktikan body dynamic mass 2 menghasilkan inverse mass 0.5, body static ber-inverse-mass nol, preservasi entity/revision pada konfigurasi invalid, serta body yang dibuat dapat dibaca oleh query XPBD setelah caller melakukan step. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **103/103 Release** dan **103/103 ASAN**.

Ini bukan Rigidbody API lengkap: tidak ada forces, impulses, damping, angular body API, authoring 3D collider, trigger event, ownership multiplayer, atau release evidence.
