# Gameplay Trigger Tracker V1

`GameplayTriggerTracker` adalah tracker trigger circle XZ read-only di atas `GameplayPhysicsQuery::OverlapCircle`. Setelah query berhasil, ia mengurutkan dan menduplikasi hasil `EntityID`, membandingkannya dengan active set sebelumnya, lalu mempublikasikan delta `entered` dan `exited` yang terurut. Empty overlap valid menghasilkan delta exit bagi entity yang sebelumnya aktif.

Jika konfigurasi invalid, tracker belum diinisialisasi, atau query gagal—termasuk batas maksimum hasil overlap—active set dan delta sebelumnya dipertahankan. Komponen ini tidak menjalankan callback, tidak menulis entity flag, transform, velocity, solver, layer, atau ECS; consumer harus mengambil delta secara eksplisit dan memutuskan perilaku gameplay sendiri.

`gameplay_trigger_tracker_smoke` membuktikan enter, steady update tanpa delta, exit, urutan deterministic, serta preservasi state saat query dibatasi capacity. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **109/109 Release** dan **109/109 ASAN**.

Ini bukan sistem trigger produksi: tidak ada callback dispatcher, trigger geometry umum, scene-body sync, 3D volume, lifecycle multiplayer, persistence, atau release evidence.
