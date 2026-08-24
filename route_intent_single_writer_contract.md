# Kontrak Route-Intent dan Satu Penulis Transform

## Status dan tujuan

Dokumen ini awalnya adalah **spesifikasi pra-implementasi** untuk menghubungkan navigasi grid dengan locomotion skeletal. Tahap pertama menambahkan seam C++ bounded pada `GridRouteFollower`: `PeekIntent` mengeluarkan snapshot `RouteIntent` tanpa menulis world/cursor, sedangkan `CommitIntent` menerima receipt nilai dari writer eksternal dan memajukan paling banyak satu cursor hanya setelah entity benar-benar tiba. Tahap kedua menambahkan `RouteRootMotionAdapter`, yang memakai seam tersebut untuk memanggil root-motion skeletal sebagai satu-satunya writer transform. Tidak ada jalur ini yang dimiliki atau dipanggil otomatis oleh `NeoRuntime`.

Tujuannya adalah menutup kelas kegagalan berikut tanpa menggabungkan dua delta translasi secara aritmetis: route kinematik menggerakkan entity menuju cell berikutnya, sementara root motion dari clip juga menggerakkan entity pada frame yang sama. Menjumlahkan keduanya akan membuat jarak/kecepatan tidak lagi berasal dari navigasi ataupun animation clip, mempersulit determinisme, dan meniadakan nilai fail-closed dari gate authority.

> **Invariant utama:** untuk satu `SceneEntity` pada satu tick unpaused, hanya satu subsystem boleh melakukan `SceneWorld::SetTransform`. Route navigation boleh menentukan niat tujuan, tetapi tidak boleh menulis transform ketika locomotion skeletal dipilih sebagai penulis.

## Kontrak authority yang dipertahankan

| Konteks entity | Penulis transform yang diizinkan | Peran route | Peran skeletal | Claim gate |
|---|---|---|---|---|
| Route kinematik mandiri saat ini | `GridRouteFollower` melalui `KinematicMotionController` | Menentukan dan menjalankan target cell | Tidak terikat | `KinematicRoute` |
| Skeletal root motion mandiri saat ini | `SkeletalAnimationController` | Tidak terikat | Menghasilkan dan menerapkan translasi root | `SkeletalRoot` |
| Adapter planar teruji | `RouteRootMotionAdapter` mendelegasikan write ke root-motion controller | Menghasilkan **intent** target cell, tanpa write | Satu-satunya sumber translasi entity | `SkeletalRoot` |

`MovementAuthorityGate` tetap merupakan pengaman per tick, bukan mekanisme blend. `NeoRuntime` mereset gate hanya pada tick **unpaused**, sebelum route built-in dijalankan. Oleh karena itu, paused tick tidak boleh dianggap sebagai peluang untuk menulis atau meng-commit progress route/animation.

## Bentuk minimal route intent

Implementasi berikutnya harus menggunakan objek nilai bounded, misalnya `RouteIntent`, bukan pointer hidup ke `GridRouteFollower` atau `SceneWorld`. Bentuk tepatnya dapat disesuaikan setelah API diperkenalkan, tetapi seluruh field semantik di bawah wajib tersedia dan tervalidasi.

| Field semantik | Sumber | Aturan validasi | Fungsi |
|---|---|---|---|
| Entity dan generation | Caller | Entity valid di snapshot world | Mengikat intent pada satu actor konkret |
| Route revision/cursor | Follower | Cocok pada commit | Mencegah commit dari intent basi atau dobel |
| Current cell dan next cell | Follower | Cell adjacent, passable, berada dalam navigation | Menyatakan segmen route aktif |
| Target planar position | Next cell | Finite dan tepat pada pusat/corner grid yang telah didefinisikan | Sasaran lokomosi; bukan delta transform |
| Remaining planar distance | Snapshot transform ke target | Finite, non-negatif, bounded | Validasi progres/arrival, bukan velocity source |
| Toleransi arrival | Kontrak tetap | Positif dan lebih kecil dari setengah unit cell | Hanya root writer yang boleh menyatakan tiba |

`GridRouteFollower` kini memiliki operasi baca-intent dan operasi commit terpisah di samping `Step` lama. `PeekIntent` memvalidasi start/target/obstacle seperti `StepInPlace`, lalu mengeluarkan intent tanpa mengubah route index, `started_`, controller, atau world. `CommitIntent` menerima `RouteIntentReceipt` bernilai, memvalidasi entity, route revision, cursor, endpoint, obstacle, serta posisi aktual pada segmen, lalu memajukan satu cursor hanya saat tiba dalam toleransi. Receipt tanpa `motionApplied`, intent basi/diubah, target terblokir, entity hilang, atau posisi di luar segmen ditolak tanpa menulis transform atau memajukan cursor. Commit parsial yang masih berada pada segmen tetap valid tetapi tidak memajukan cursor, sehingga intent yang sama dapat diminta lagi.

## Prosedur single-writer per tick

Adapter tahap pertama tidak mendukung replanning otomatis, waktu clip yang berubah karena parameter gameplay, maupun rotasi route. Prosedur commit yang diimplementasikan adalah sebagai berikut.

1. Runtime atau caller membuat snapshot `SceneWorld`, follower, dan controller locomotion. Jika runtime paused, prosedur berhenti sebelum membuat intent.
2. Follower menghasilkan tepat satu `RouteIntent` untuk next cell. Bila route selesai, tidak ada writer locomotion yang dipanggil dan tidak ada cursor commit.
3. `RouteRootMotionAdapter` menyalin follower, skeletal controller, world, authority, dan palette sebagai kandidat. Setelah root-motion candidate diterapkan, ia memvalidasi translasi planar finite, tanpa komponen Y, pada sumbu segmen dan menuju target. Adapter **tidak** mengskalakan, membalik, atau menjumlahkan delta root dengan delta route untuk mengejar target.
4. `SkeletalAnimationController::AdvanceApplyRootMotionGuarded` menjadi satu-satunya jalur yang berhak mengubah entity tersebut dan harus memperoleh `SkeletalRoot` authority sebelum player time, palette, atau world berubah.
5. Setelah write root motion berhasil, adapter membangun receipt kandidat dari snapshot world baru. Cursor route hanya boleh maju bila receipt membuktikan arrival pada target intent di dalam toleransi. Jika belum tiba, cursor tidak bergerak; tick berikutnya dapat mengeluarkan intent yang sama.
6. Semua commit state—player skeletal, palette, SceneWorld, dan cursor route—harus dipersiapkan sebagai kandidat dan diswap hanya sesudah setiap validasi di atas lulus. Kegagalan di satu langkah harus mempertahankan semua state yang relevan.

Kebutuhan ini sengaja menolak mode yang sering tampak mudah tetapi tidak dapat dibuktikan dengan kontrak saat ini: root motion yang bergerak sebagian menuju target lalu kinematic motion “menyelesaikan” sisa jarak, atau route follower yang mengubah cursor sebelum root motion berhasil. Kedua mode tersebut menciptakan dua penulis transform atau state progress yang dapat terlepas dari posisi nyata.

## Batas teknis yang eksplisit

`SceneWorld` menyimpan rotasi local sebagai Euler, sedangkan `SkeletalPoseClip` memakai quaternion. Tidak ada konversi root-rotation yang telah disepakati dan tervalidasi. Karena itu, tahap route-intent awal adalah **translasi planar saja**; heading/yaw route harus tetap non-writer dan root rotation tidak boleh diterapkan sampai ada kontrak ruang, urutan Euler, sumbu forward clip, normalisasi sudut, serta smoke terpisah.

Selain itu, tidak ada bukti bahwa semua clip root motion memiliki jarak, fase, atau arah yang cocok dengan cell grid. Adapter tidak boleh mengasumsikan hal tersebut. Tahap implementasi pertama harus menerima hanya clip locomotion yang dinyatakan cocok oleh metadata bounded yang tervalidasi atau membatasi smoke pada clip uji dengan displacement eksplisit. Clip tanpa kecocokan harus ditolak sebelum time/palette/world/cursor berubah.

Dokumen ini juga tidak menambahkan integrasi otomatis ke `NeoRuntime`. Sebelum implementation gate terpenuhi, runtime tetap hanya menjalankan satu `GridRouteFollower` built-in melalui `StepGuarded`. Ia tidak boleh membuat controller skeletal, mengikat clip, menjalankan adapter, atau mengklaim NPC locomotion otomatis.

## Kriteria smoke dan bukti implementasi

`route_root_motion_adapter_smoke` kini membuktikan subset implementasi yang aman pada Release dan AddressSanitizer dengan `detect_leaks=1`. Semua kegagalan yang diuji mempertahankan state caller karena adapter hanya menukar kandidat setelah `CommitIntent` berhasil.

| Kasus | Bukti yang wajib | State yang harus tetap utuh bila gagal |
|---|---|---|
| Intent valid | `PeekIntent` membaca next cell tanpa write world/cursor | World, route index, player, palette |
| Root writer tunggal | Root motion mengubah transform; route tidak memanggil `KinematicMotionController::Step` | Tidak ada delta kedua |
| Arrival exact/tolerant | Receipt valid memajukan satu cell saja | Tidak ada lompat dua cell |
| Partial root advance | Entity belum tiba; intent/cursor tetap pada cell yang sama | Cursor dan target berikutnya |
| Direction/invalid planar delta | Adapter menolak candidate root write yang tidak searah atau memiliki Y | Player time, palette, world, cursor |
| Authority conflict | `KinematicRoute` yang sudah di-claim membuat skeletal writer gagal | Player time, palette, world, cursor |
| Blocked target/replan | Intent/commit menolak target dinamis terblokir; replan tetap caller-triggered | World dan cursor lama |
| Paused runtime | Masih **belum diintegrasikan**; tidak ada pemanggilan adapter oleh runtime | World, route, player |
| Stale intent | Revision atau entity generation mismatch ditolak | Cursor dan transform terkini |

Smoke tidak boleh menggunakan NPC, collision, physics, renderer, prediction, network, atau route rotation sebagai substitusi bukti. Setelah smoke itu lulus pada kedua konfigurasi, integrasi `NeoRuntime` harus tetap menjadi item terpisah dengan smoke lifecycle sendiri.

## Keputusan implementasi

`grid_route_intent_smoke` membuktikan intent awal tidak menulis world/cursor, receipt tidak-terapkan ditolak, transform parsial mempertahankan cursor, target terblokir mempertahankan output intent caller, intent basi/diubah ditolak, dan arrival memajukan satu cell hingga goal. `route_root_motion_adapter_smoke` membuktikan root skeletal menjadi writer tunggal untuk progress parsial dan arrival, direction mismatch serta authority conflict mempertahankan world/player/palette/cursor, dan target terblokir ditolak sebelum write. Urutan aman berikutnya adalah lifecycle opt-in yang terpisah di `NeoRuntime`, hanya setelah smoke pause/goal/replan/authority lifecycle yang khusus. Tidak ada klaim gameplay NPC, navigation-animation sync penuh, root rotation, collision/physics coupling, multiplayer movement authority, renderer character binding, atau production readiness yang mengikuti desain ini.
