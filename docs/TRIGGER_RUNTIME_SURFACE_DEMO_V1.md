# Trigger Runtime Surface Demo V1

`RunTriggerRuntimeSurfaceDemo` adalah vertical slice finite yang menggabungkan movement, physics mirror, trigger, dan render. Pada setiap frame, `MovementAuthorityGate` memberi claim `KinematicRoute`; hanya `KinematicMotionController` menulis actor `SceneWorld`. `ScenePhysicsPoseSync` kemudian menyalin pose X/Z ke satu circle body static ECS, caller menjalankan `XPBDPhysicsSystem::Step`, dan `GameplayTriggerTracker` membaca snapshot itu untuk delta enter/exit.

Actor bergerak dari X=-0.75 melalui trigger circle di X=0/Z=3, menghasilkan satu enter dan satu exit tanpa callback atau mutasi trigger. Sprite actor berasal dari `TextureImportPipeline`, dibaca `SceneSpriteAdapter`, lalu dirender/present finite melalui software surface dan artifact PPM.

`trigger_runtime_surface_demo_smoke` membuktikan konfigurasi invalid ditolak, empat frame render/present, satu enter, satu exit, pixel non-background, posisi akhir X=0.75, dan artifact `P6`. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **111/111 Release** dan **111/111 ASAN**.

Ini adalah proof integrasi runtime bounded, bukan game playable, collision response, NPC behavior, persistent host, multiplayer, APK, atau release evidence.
