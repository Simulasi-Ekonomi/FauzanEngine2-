# Animation Locomotion Bridge V1

`AnimationLocomotionBridge` menerjemahkan satu snapshot `KinematicPlanarInput` menjadi trigger transisi idle/lokomotion pada `AnimationStateMachine`. Konfigurasi memuat transition ID eksplisit dan threshold gerak finite non-negatif. Input dengan panjang planar di atas threshold memicu idle-to-locomotion; input di bawah atau sama dengan threshold memicu locomotion-to-idle.

Bridge tidak memiliki `SceneWorld`, `KinematicMotionController` mutable, `RouteIntent`, `MovementAuthorityGate`, atau API transform. Ia tidak menjalankan controller, tidak mengambil movement authority, dan tidak menulis root motion. Jika state machine belum dimulai, input non-finite, atau trigger state gagal, status locomotion bridge sebelumnya dipertahankan.

`animation_locomotion_bridge_smoke` membuktikan configuration fail-closed, penolakan state machine yang belum dimulai, idle dead-zone, transisi locomotion/idle, serta penolakan NaN sambil mempertahankan state. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **101/101 Release** dan **101/101 ASAN**.

Ini bukan skeleton, root motion, transform binding, NPC brain, navigation, collision, prediction multiplayer, APK, atau release evidence.
