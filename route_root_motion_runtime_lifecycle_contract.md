# Kontrak Lifecycle Opt-In NeoRuntime untuk Route Root Motion

## Status

Dokumen ini menjelaskan ownership opt-in `RouteRootMotionAdapter` oleh `NeoRuntime`. Adapter caller-invoked, `RouteIntent`, dan `CommitIntent` memiliki bukti C++ tersendiri; runtime kini dapat menyimpan adapter, controller skeletal snapshot-owned, dan palette locomotion untuk satu route static clamp yang lurus. Semua batas di bawah tetap berlaku.

> **Keputusan batas:** runtime tidak boleh mengaktifkan root-motion route secara default, tidak boleh mengganti static `GridRouteFollower` kinematik yang ada, dan tidak boleh membangun adapter dari asset hidup atau pointer caller.

## Mode konfigurasi yang diizinkan

Kontrak berikut mengusulkan mode eksplisit yang saling-eksklusif untuk entity route static tunggal runtime. Bentuk field C++ dapat disesuaikan, tetapi semantiknya tidak boleh diperluas tanpa smoke baru.

| Mode | Konfigurasi | Entity writer | Status saat ini |
|---|---|---|---|
| Disabled | Tidak ada route motion | Tidak ada | Sudah ada |
| Kinematic static route | `enableRouteMotion` saat ini | `GridRouteFollower::StepGuarded` melalui kinematic controller | Sudah ada |
| Skeletal static route | Flag opt-in, snapshot skeleton/clip, direction assertion, dua cell | `RouteRootMotionAdapter` melalui `AdvanceApplyRootMotionGuarded` | Terbukti pada smoke lifecycle |

Mode kinematic dan skeletal **tidak boleh aktif bersamaan**. Runtime harus menolak kombinasi tersebut pada `Initialize` sebelum membuat SceneWorld route entity, follower, controller, atau mengubah state menjadi initialized. `InputMotionBridge` yang terpisah tidak berubah oleh kontrak ini karena entity-nya terpisah; namun tidak ada penggabungan input dengan skeletal route pada tahap ini.

## Data konfigurasi ownership

Konfigurasi skeletal route harus membawa snapshot nilai lengkap, bukan referensi non-owning, setidaknya berupa `Skeleton`, `SkeletalPoseClip`, dan `SkeletalPosePlaybackMode`. Pada initialize, runtime harus menyalin konfigurasi itu ke candidate `SkeletalAnimationController` melalui API `Initialize` yang sudah memvalidasi hierarchy/bind/inverse-bind/track compatibility. Clip, skeleton, atau mode invalid harus menyebabkan `InvalidConfiguration` tanpa mengganti runtime sebelumnya.

Route tetap memakai batas static route existing: 2–512 cell valid dalam grid yang terinisialisasi. Pada implementasi pertama, route harus terdiri dari **satu segmen planar** saja dan konfigurasi harus menyatakan arah clip yang diharapkan sebagai assertion bounded. Alasannya, root clip yang ada belum mempunyai metadata sumbu-forward, scaling ke grid, blending locomotion, atau root-rotation conversion. Validasi runtime harus menolak route berbelok/multi-segmen dan tidak boleh membiarkan adapter baru berhasil pada segmen pertama lalu gagal karena arah clip pada segmen berikutnya.

| Data | Validasi initialize | Alasan fail-closed |
|---|---|---|
| Flag skeletal route | Tidak bersama mode route kinematik | Satu entity tidak memiliki dua writer route |
| Skeleton dan clip snapshot | `SkeletalAnimationController::Initialize` berhasil | Tidak ada pointer clip/hierarchy dangling |
| Mode playback | Clamp saja pada tahap runtime pertama | Tidak ada wrap route/cycle distance contract |
| Route | Tepat dua cell, adjacent, tidak terblokir, dalam grid | Satu target dan satu sumbu yang dapat diaudit |
| Arah assertion clip | Salah satu ±X/±Z dan cocok dengan route | Menolak clip yang tidak mampu mencapai segmen |
| Kecepatan/delta | Tetap berasal dari clip root; tidak ada scale/koreksi kinematic | Menjaga satu sumber displacement |

Assertion arah bukan konversi rotasi. Adapter masih wajib memeriksa delta aktual setelah root motion candidate; mismatch tetap gagal tertutup dan mempertahankan controller/world/cursor/palette/gate caller.

## Urutan tick dan authority

Pada setiap tick unpaused yang valid, runtime tetap memanggil `MotionAuthority()->BeginFrame()` sekali sebelum route motion. Ketika skeletal mode dipilih dan follower belum mencapai goal, urutan yang diizinkan adalah:

1. `RouteRootMotionAdapter::Advance` menghasilkan `RouteIntent` dari candidate follower tanpa write transform.
2. Adapter meminta `SkeletalRoot` pada candidate gate dan memanggil root-motion guarded sebagai satu-satunya writer.
3. Adapter memvalidasi delta dan meng-commit receipt cursor hanya setelah arrival.
4. Runtime mengganti state candidate hanya bila seluruh adapter call berhasil.

Tick paused harus berhenti sebelum langkah di atas, seperti route static saat ini: tidak ada player-time advance, world write, route cursor commit, atau claim baru. Claim dari unpaused tick sebelumnya juga tidak dihapus oleh paused tick; smoke membuktikan claim `SkeletalRoot` tetap konflik dengan `KinematicRoute` sampai tick unpaused berikutnya menjalankan `BeginFrame`. Tick setelah resume melakukan `BeginFrame` baru sebelum adapter berjalan. Setelah goal, tick berikutnya tetap mereset gate tetapi tidak memanggil adapter atau skeletal controller, sehingga claim baru dapat diperoleh sesuai kontrak `MovementAuthorityGate` yang ada.

Runtime tidak menyediakan injection hook untuk memaksa competing claim di antara `BeginFrame` dan adapter. Konflik same-frame tetap menjadi bukti adapter terisolasi; lifecycle runtime hanya membuktikan bahwa ia memilih `SkeletalRoot` ketika mode skeletal aktif dan tidak menggunakan jalur kinematic untuk entity tersebut.

## Replan, failure, dan shutdown

Replan tetap caller-triggered dan tidak boleh mengubah player time atau palette. Untuk tahap static satu-segmen, `ReplanRouteRootMotion` hanya boleh diterima sebelum route mulai atau dari posisi cell yang benar-benar telah dicapai; replacement harus tetap satu segmen dan cocok dengan assertion arah. Runtime yang telah memasuki state `Failed` karena target block, root mismatch, atau adapter failure tidak dapat dipulihkan oleh replan, persis seperti kebijakan static route kinematik saat ini.

`Shutdown` mereset adapter, controller skeletal, palette, route follower, navigation, entity handle, dan route mode bersama SceneWorld secara deterministik. `SkeletalRouteMotionController()` mengembalikan `nullptr` sesudah shutdown; tidak ada accessor palette atau clip yang diekspos oleh runtime.

## Smoke wajib sebelum source diubah

`runtime_route_root_motion_smoke` lulus pada Release dan AddressSanitizer dengan `detect_leaks=1`. Ia membuktikan snapshot skeletal/clip kosong, playback loop, konfigurasi eksklusif, dan route berbelok ditolak; kemudian membuktikan progress parsial dan arrival dari root-motion saja, paused preservation, goal stability dan reset gate pada tick berikutnya, replan skeletal ditolak eksplisit, teardown accessor, direction mismatch atomic, serta next target terblokir atomic.

| Kasus | Bukti wajib |
|---|---|
| Default dan incompatibility | Skeletal mode default-off; snapshot skeleton/clip kosong, playback loop, kombinasi kinematic+skeletal, dan route berbelok ditolak di initialize tanpa ownership parsial. |
| Partial dan arrival | Dua tick skeletal root clip membawa entity menuju target melalui adapter saja; cursor tidak maju pada partial, lalu maju tepat pada arrival. Tidak ada kinematic writer. |
| Pause/resume | Paused tick menjaga transform, route cursor, player time, palette, dan tidak membuat claim; tick resume berjalan dari state sama. |
| Goal stability/reset | Tick goal-stable tidak memajukan player/world dan mereset claim frame sehingga authority baru dapat diambil sesudah tick. |
| Explicit replan | Versi satu-segmen menolak `ReplanRouteMotion` eksplisit dengan `RouteReplanFailed`, tanpa player/palette/world mutation. Tidak ada replan otomatis. |
| Failure atomicity | Target block, root direction mismatch, dan authority conflict mempertahankan controller, palette, world, follower, dan gate caller. |
| Shutdown | Pointer/accessor skeletal-route tidak tersedia setelah shutdown dan lifecycle runtime tetap berakhir bersih. |

Tidak ada bagian kontrak ini yang menambahkan root rotation, character mesh binding, clip asset importer, blend tree, route steering/avoidance, collision/physics, NPC orchestration, client prediction, multiplayer authority, atau production readiness.
