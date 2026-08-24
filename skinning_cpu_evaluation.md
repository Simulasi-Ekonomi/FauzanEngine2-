# CPU Skinning Evaluation

Tanggal evaluasi: 24 Agustus 2026

## Status

Modul legacy `Animation/Skinning` kini menyediakan **CPU position dan normal skinning** bounded dan fail-closed. Primitive ini menerima posisi xyz, normal xyz, dan maksimum empat influence per vertex. Ia tidak mengklaim sebagai sistem skeletal animation lengkap.

| Area | Bukti saat ini | Batas tegas |
|---|---|---|
| Posisi dan normal CPU | Linear-blend posisi dan normal maksimum 4096 vertex dan 128 matriks bone | Tidak ada tangent skinning atau mesh buffer interleaved |
| Validasi | Memeriksa finite position/normal/matrix/weight, indeks bone, count buffer, jumlah bobot, dan inverse-transpose affine yang invertible untuk normal | Primitive Skinning tidak sendiri membangun matriks bone; caller memasok palette |
| Failure atomic | Menghitung kandidat lalu menukar buffer hanya ketika seluruh input valid | Tidak ada transaksi lintas mesh atau frame scheduler |
| Evidence | Smoke Release dan AddressSanitizer membuktikan blend dua bone serta preservasi buffer pada jalur invalid | Tidak ada GPU evidence atau renderer integration |

## Kontrak yang dibuktikan

`Skinning::ApplySkinning` menolak input yang tidak memenuhi kontrak tanpa mengubah buffer posisi caller. Jalur yang dibuktikan mencakup indeks bone di luar rentang, bobot yang tidak berjumlah satu, matriks non-finite, dan jumlah `VertexWeight` yang tidak cocok.

`Skinning::ApplySkinningWithNormals` menambah jalur posisi-normal terpisah. Untuk setiap matriks bone finite yang affine dan memiliki linear 3×3 invertible, ia membangun inverse-transpose normal, menerapkan empat influence yang sama, menormalisasi hasil normal per vertex, lalu hanya menukar buffer posisi dan normal setelah seluruh vertex valid. Smoke membuktikan skala x non-uniform dua kali dan translasi x tiga memindahkan posisi `(1,1,0)` menjadi `(5,1,0)` serta mengubah normal `(1,1,0)` menjadi kira-kira `(0.447214,0.894427,0)`; zero normal, normal count salah, matriks singular, dan matriks projective ditolak tanpa mengganti output.

`Skeleton::TryAddBone` sekarang membangun metadata hierarchy single-root yang dibatasi 64 bone, sejalan dengan batas authoring. Bone pertama wajib root, tiap bone berikutnya wajib merujuk parent yang sudah ditambahkan, dan nama bone wajib unik serta tidak kosong. Duplicate, root kedua, parent negatif/maju, dan overflow kapasitas ditolak tanpa mengubah hierarchy.

`Skeleton::EvaluateGlobalBindPose` menyusun local bind matrix dari root menuju child menjadi global bind matrix dalam output sementara. Ia hanya mengganti output caller jika seluruh matriks lokal dan hasil komposisi finite; skeleton kosong atau matrix NaN menolak evaluasi tanpa mengubah output sebelumnya.

`Skeleton::DeriveInverseBindPose` membatasi derivasi pada matrix affine finite dengan komponen linear yang non-singular. Ia membuat seluruh kandidat inverse global bind sebelum menulis `Bone::inverseBindPose`, sehingga kegagalan singular tidak mengubah inverse bind yang sudah ada.

`Skeleton::EvaluateSkinningPalette` menerima tepat satu local pose affine finite untuk setiap bone setelah inverse bind berhasil diturunkan. Ia menyusun global pose mengikuti hierarchy yang telah tervalidasi, menghitung `globalPose × inverseBind` per bone dalam kandidat sementara, lalu mengganti output palette caller hanya jika seluruh pose valid. Local bind pose menghasilkan palette identitas; perubahan translasi local head menghasilkan translasi skinning yang sesuai. Count pose tidak cocok, pose non-affine, atau inverse bind yang belum tersedia menolak evaluasi tanpa mengubah output sebelumnya.

`SkeletalPoseClip` menyediakan satu clip TRS typed yang dibatasi 64 track bone dan 128 keyframe terurut per track. Ia mengharuskan setiap track dimulai pada waktu nol, memvalidasi translation/scale finite, quaternion nonzero, serta scale yang tidak nol; sampling kemudian menginterpolasi translation/scale secara linear dan quaternion dengan normalized lerp menjadi matriks local affine. Ia hanya mengganti output local pose apabila semua track tersedia dan valid. `SampleLooped` adalah operasi caller-invoked yang membungkus waktu dengan durasi maksimum track yang positif; clip statis dan clip tidak lengkap ditolak tanpa mengganti output. Smoke membuktikan sampel waktu 0,5 dan sampel loop waktu 1,5 menghasilkan translasi root x satu dan rotasi head z 90 derajat dari keyframe identitas ke 180 derajat, sementara waktu negatif dan clip tidak lengkap mempertahankan output sebelumnya.

`SampleBlended` membaurkan dua clip yang memiliki count track sama dengan sampling TRS dari masing-masing pada waktu caller-supplied, linear blend untuk translation/scale, dan normalized quaternion lerp untuk rotation. Faktor blend wajib finite pada `[0,1]`; count track tidak cocok atau track tidak lengkap gagal tanpa mengganti output. Smoke membuktikan blend 50% antara root x=1 dan root x=4 menghasilkan x=2,5, serta antara rotasi z 90 derajat dan identitas menghasilkan rotasi z 45 derajat.

`SkeletalPosePlayer` adalah pemegang state waktu lokal dengan snapshot owned dari `SkeletalPoseClip` lengkap saat bind. Caller dapat mengubah atau melepas sumber clip sesudah bind tanpa membuat referensi player dangling. Player menerima mode clamp atau loop, speed non-negatif maksimum 100, pause, lalu `Advance` dengan delta terbatas maksimal satu detik. Ia menyampel ke buffer kandidat terlebih dahulu dan hanya mengubah snapshot state waktu serta output caller setelah sampling berhasil. Smoke membuktikan advance clamp 0,25, pause tanpa perubahan waktu, speed dua kali, clamp terminal, loop yang membungkus dari 1,25 ke 0,25 melalui dua langkah valid, preservasi state/output pada delta negatif maupun bind-loop clip statis, serta advance yang tetap valid setelah clip sumber keluar scope.

`SkeletalAnimationController` menghubungkan snapshot `SkeletalPosePlayer` dengan hierarchy `Skeleton` dan inverse bind. Saat inisialisasi ia menyalin Skeleton, menurunkan inverse bind, memvalidasi count track clip sama dengan count bone, lalu menyimpan player snapshot. Saat `Advance`, ia lebih dahulu menyalin player, membangun local pose kandidat, mengevaluasi palette `globalPose × inverseBind`, dan hanya kemudian mengganti waktu controller serta output palette. Smoke membuktikan root animasi menggeser palette head x=1 pada waktu 0,5 lalu x=2 pada terminal, pause preservation, speed dua kali, delta negatif tanpa mutasi, dan penolakan re-inisialisasi clip dua-track pada skeleton tiga bone sambil mempertahankan konfigurasi lama.

`SkeletalAnimationController::AdvanceAndSkin` memperluas transaksi yang sama ke buffer posisi dan normal CPU. Ia membuat kandidat player, local pose, palette, posisi, dan normal; kemudian menjalankan `Skinning::ApplySkinningWithNormals` pada kandidat tersebut dan hanya mengubah waktu serta kedua buffer caller setelah seluruh jalur berhasil. Smoke membuktikan vertex head-bind `(0,2,3)` menjadi `(1,2,3)` pada waktu 0,5 dengan normal `(1,0,0)` tetap valid di bawah translasi root, sementara bone index invalid menolak operasi tanpa mengubah waktu controller, posisi, atau normal terakhir.

`SkeletalAnimationController::AdvanceWithRootMotion` mengekstrak delta translasi local root pada mode clamp maupun loop. Controller menyimpan translasi root awal, akhir, terakhir, serta waktu cycle; pada loop ia mengakumulasi segmen menuju akhir cycle, cycle penuh di tengah, dan segmen dari awal cycle menuju pose terkini. Jumlah wrap dibatasi 128 agar tidak menerima advance abnormal. Smoke membuktikan delta x=1 pada waktu 0,5, delta nol ketika pause dan setelah terminal clamp, delta x=1 ketika speed dua kali membawa waktu ke terminal, serta preservasi pada delta invalid. Untuk loop, smoke membuktikan satu wrap menghasilkan delta x=3 dan multi-wrap menghasilkan x=6 tanpa diskontinuitas output.

`SkeletalAnimationController::AdvanceApplyRootMotion` menerapkan delta translasi clamp atau loop bounded ke local transform `SceneWorld` entity. Sebelum mengevaluasi palette kandidat, ia mengembalikan translasi root local pose ke nilai root awal; dengan demikian pergerakan hanya dibawa oleh SceneWorld dan tidak dihitung dua kali lagi di palette skinning. Ia menyalin seluruh `SceneWorld` sebagai kandidat, membaca local transform entity, menambah delta root terakumulasi, dan memanggil validasi transform kandidat sebelum mengganti world, waktu controller, root state, dan palette bersama-sama. Smoke membuktikan entity clamp mulai x=10 menjadi x=11 pada waktu 0,5 sementara palette head tetap x=0; pada loop, x=10 menjadi x=13 setelah satu wrap dan x=19 setelah multi-wrap dengan palette in-place yang sama. Entity invalid ditolak tanpa mengubah world atau waktu controller.

Smoke canonical juga membuktikan jalur posisi CPU yang tersambung: local pose head yang menaikkan z satu unit menghasilkan palette head dengan translasi z satu unit; satu vertex yang dipengaruhi 100% oleh head kemudian berubah dari `(1,2,3)` menjadi `(1,2,4)` melalui `Skinning::ApplySkinning`. Bone index yang invalid tetap menolak operasi dan mempertahankan posisi hasil sebelumnya.

Jalur keyframe TRS juga tersambung secara eksplisit dalam smoke terpisah: clip pada waktu 0 menjadi bind pose Skeleton, sampel waktu 0,5 dievaluasi menjadi palette, lalu vertex head-bind `(0,2,3)` yang dipengaruhi 100% oleh head berubah menjadi `(1,2,3)` melalui `Skinning::ApplySkinning`.

Jalur yang sama kini membuktikan normal: pada sampel head berotasi z 90 derajat, posisi head-bind `(0,2,3)` dan normal `(1,0,0)` melalui palette yang sama menjadi posisi `(1,2,3)` serta normal `(0,1,0)` melalui `Skinning::ApplySkinningWithNormals`. Normal nol ditolak tanpa mengganti posisi ataupun normal output sebelumnya.

> Primitive ini adalah dasar CPU terbatas untuk deformasi posisi dan normal. Ia bukan pose solver, skeleton runtime, mesh skinning production, atau character animation system.

## Kesenjangan tersisa

`AnimationClip`, `AnimationPlayer`, `AnimationGraph`, dan `SkinningGPU` legacy masih tidak menyediakan blend state machine, renderer bridge, maupun GPU palette/upload. `SkeletalPoseClip`, `SkeletalPosePlayer`, dan `SkeletalAnimationController` baru menyediakan sampling clamp/loop, satu blend dua-clip caller-invoked, state waktu snapshot, controller palette/posisi/normal CPU, root-motion translasi clamp/loop bounded, dan aplikasi clamp/loop ke local transform SceneWorld snapshot; belum ada kepemilikan NeoRuntime, clock/tick otomatis, autoplay, state-machine/transitions, parameterized blend tree, additive layer, root-motion rotation atau navigation/physics integration, retargeting, tangent skinning, pengikatan mesh, atau render. Kesenjangan tersebut harus ditangani sebagai increment terpisah dengan kontrak dan smoke sendiri; status production readiness tetap **NOT PASSED**.
