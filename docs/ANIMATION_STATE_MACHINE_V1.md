# Animation State Machine V1

`AnimationStateMachine` adalah selector scalar bounded di atas `AnimationTimeline`. State mengikat ID state ke ID track dan mode `Clamp` atau `Loop`; transition mengikat state asal/tujuan dengan durasi finite non-negatif. Instance dibatasi hingga 32 state dan 64 transition.

`Start` memilih state aktif dan mengatur waktu menjadi nol. `Trigger` hanya menerima transition dari state aktif serta menolak transition baru selama blend berlangsung. Transition berdurasi nol berpindah seketika; transition lain mensampling source dan target pada waktu lokal masing-masing lalu menggunakan linear blend deterministik. `Update` hanya menerima delta finite non-negatif dan tidak mengubah waktu/state bila validasi gagal.

Komponen ini tidak memiliki `SceneWorld`, `TransformAnimationBinding`, route, controller, atau authority gate. Ia tidak dapat menulis transform, root motion, input movement, atau skeletal route. Konsumennya harus secara eksplisit memilih bagaimana nilai scalar hasil `Sample` akan digunakan.

`animation_state_machine_smoke` membuktikan state bounded, transition valid, blend 0.5-detik, completion, transition instan, missing-track fail-closed, serta invalid delta dan concurrent-trigger rejection. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **100/100 Release** dan **100/100 ASAN**.

Ini bukan skeletal animation, blend tree multidimensi, animation graph, root motion, character/NPC behavior integration, asset import, multiplayer prediction, APK, atau release evidence.
