# Motion Animation Surface Demo V1

`RunMotionAnimationSurfaceDemo` adalah runtime slice finite yang menggabungkan satu penulis movement dan render sprite. Setiap frame dimulai dengan `MovementAuthorityGate`, lalu hanya `KinematicMotionController` yang memperoleh `KinematicRoute` dan menulis transform actor. `AnimationLocomotionBridge` menerima input yang sama tetapi hanya memicu transisi idle/move pada `AnimationStateMachine`; ia tidak memegang world atau menulis transform.

Demo mengimpor satu sprite PPM melalui `TextureImportPipeline`, mengikatnya dengan `SceneSpriteAdapter`, lalu merender transform world hasil kinematic menggunakan `SpriteBatch`, `SoftwareRenderer`, dan `SoftwareSurfacePresenter`. Dua frame pertama bergerak ke kanan; dua frame berikutnya berhenti sehingga animation state kembali idle. Output akhir adalah PPM eksplisit.

`motion_animation_surface_demo_smoke` membuktikan konfigurasi invalid ditolak, empat frame render/present tersembunyi, posisi X actor bertambah, sample animasi akhir idle bernilai nol, bridge berakhir non-locomoting, dan artifact PPM memiliki header `P6`. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **107/107 Release** dan **107/107 ASAN**.

Ini bukan gameplay lengkap, root motion, skeleton, NPC intelligence, navigation, physics-driven movement, host persisten, multiplayer, APK, atau release evidence.
