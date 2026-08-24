# Kontrak Metadata Locomotion Skeletal Multi-Segmen

## Status dan alasan desain

Implementasi saat ini membuktikan `NeoRuntime` dapat menjalankan **satu** segmen route lurus dua-cell dengan `RouteRootMotionAdapter`: root motion skeletal adalah satu-satunya penulis transform, sementara `GridRouteFollower` hanya menghasilkan intent dan menerima receipt cursor. Konfigurasi runtime saat ini memakai satu assertion arah (`±X` atau `±Z`) dan menolak route berbelok atau loop.

`SkeletalPoseClip` memiliki keyframe translasi root dan `SkeletalAnimationController` dapat mengeluarkan `RootMotionDelta`, tetapi clip tidak membawa metadata semantik tentang arah locomotion, panjang stride, kesiapan transisi, atau kompatibilitas grid. `SkeletalLocomotionMetadata::ValidateCardinalOneCell` menyediakan preflight isolated atas satu clip: ia menyalin clip, memeriksa duration positif maksimal satu detik, dan memverifikasi root start nol serta terminal tepat satu cell pada arah requested. `SkeletalLocomotionRegistry` kini dapat menyimpan hingga empat snapshot clip cardinal setelah preflight, menolak registry kosong, lebih dari empat entry, clip invalid, dan arah duplikat tanpa mengganti registry lama. Ia belum mengaktifkan atau memilih controller route runtime. Karena `SceneWorld` memakai Euler sementara pose clip memakai quaternion, desain ini **tidak** menambahkan root rotation atau rotasi delta untuk mengubah satu clip +X menjadi +Z.

> **Invariant:** setiap segmen cardinal route memakai clip yang memang authored dengan translasi root pada arah segmen tersebut. Runtime tidak memutar, menskalakan, memantulkan, atau menambahkan kinematic correction ke displacement clip.

## Metadata nilai yang diperlukan

Ekstensi berikut harus menggunakan set bounded berisi paling banyak empat deskriptor nilai—satu per arah cardinal—bukan pointer hidup, asset handle global, atau callback yang dapat menjalankan code. Nama C++ dapat berbeda, tetapi semantiknya wajib setara.

| Field metadata | Validasi saat initialize | Tujuan |
|---|---|---|
| Arah cardinal | Tepat `+X`, `-X`, `+Z`, atau `-Z`; maksimal satu entry per arah | Memilih clip untuk segmen grid yang aktif |
| Snapshot skeleton + clip | Controller candidate dapat `Initialize` dan bone count cocok | Tidak ada dangling reference atau hierarchy/track invalid |
| Playback | Clamp saja pada tahap pertama | Tidak ada policy cycle distance/wrap antar-cell |
| Root start translation | Tepat `(0,0,0)` dalam epsilon | Transisi baru tidak menyuntik teleport/root offset |
| Root terminal translation | Tepat satu unit pada arah metadata, tanpa komponen silang/Y | Membuktikan clip mampu menutup satu cell tanpa transform conversion |
| Durasi | Positif, finite, dibatasi satu detik atau kurang | Menjaga satu segmen selesai dengan bounded tick count |
| Keunikan direction | Tidak ada direction duplikat | Menjaga pemilihan deterministik |

Metadata harus diverifikasi terhadap sampel clip aktual pada waktu nol dan terminal durasi menggunakan jalur pose/root motion yang sama seperti controller, bukan dipercaya sebagai deklarasi string. Clip diagonal, vertikal, stationary, root start non-zero, durasi nol, atau terminal yang bukan tepat satu unit harus ditolak sebelum runtime membuat entity route atau menyimpan controller.

## Kontrak transisi route

Untuk route `C0→C1→C2`, setiap segmen menentukan arah cardinal dari difference `Ci+1 - Ci`. Runtime memilih metadata clip untuk segmen pertama di initialize. Setelah receipt adapter mengkonfirmasi entity mencapai `C1`, runtime boleh menyiapkan **kandidat** controller untuk arah segmen berikutnya dari snapshot clip yang sesuai, dengan player time nol dan palette kandidat.

| Tahap | State yang dibaca | State yang baru boleh diganti setelah sukses |
|---|---|---|
| Intent segmen aktif | Follower cursor, SceneWorld, metadata arah aktif | Tidak ada |
| Root-motion step | Controller aktif kandidat, palette kandidat, world kandidat, gate kandidat | Tidak ada sebelum receipt lulus |
| Arrival `Ci+1` | Receipt follower tervalidasi | Cursor follower dan state controller aktif selesai |
| Transition clip berikutnya | Metadata snapshot untuk arah `Ci+1→Ci+2` | Controller/palette aktif baru pada time zero |
| Kegagalan lookup/validasi transition | Semua kandidat | Tidak ada; cursor/world/controller/palette/gate caller dipertahankan |

Tidak ada controller baru yang boleh di-commit sebelum segmen sebelumnya tiba tepat. Karena itu, root clip tidak dapat berganti di tengah cell dan tidak ada dua clip atau kinematic controller yang menulis transform pada satu tick. Transition boleh terjadi setelah arrival sebagai persiapan state untuk tick berikutnya, bukan sebagai second world write pada tick arrival.

### Snapshot kandidat minimum

Implementasi transisi harus membangun satu `SegmentTransitionCandidate` bernilai yang memuat follower candidate, controller aktif candidate, controller segmen berikutnya candidate, palette candidate, authority candidate, serta revision/cursor intent yang mengikatnya. Receipt arrival dari adapter tetap menjadi satu-satunya pemicu pembentukan kandidat ini. Bila inisialisasi controller berikutnya, validator metadata, atau preflight target berikutnya gagal, kandidat dibuang seluruhnya: controller sebelumnya, cursor follower, transform world, palette, dan claim authority caller tidak boleh diganti. Pada arrival yang berhasil, commit atomik harus menulis cursor arrival dan menyimpan controller berikutnya pada time nol, tetapi tidak menjalankan root motion clip baru sampai tick sesudahnya.

## Batas route dan replan

Tahap multi-segmen pertama tetap memakai `GridRouteFollower` bounded (maksimum 512 cell), tetapi runtime perlu menyimpan satu copy metadata per arah saja. Replan tetap caller-triggered. Replan tidak boleh mengganti controller aktif sampai route replacement dan metadata semua segmen telah dipreflight; jika replacement memerlukan arah yang metadata-nya tidak tersedia, replan gagal tanpa mengubah follower/controller/world/palette.

Loop playback tidak dimasukkan ke desain ini. Agar loop dapat dipakai kelak, diperlukan metadata per-cycle displacement, batas jumlah wrap per tick, aturan phase transition antar-cell, dan smoke terpisah. Root rotation, blend tree, stride scaling, path smoothing, steering, collision/physics, NPC behavior, client prediction, multiplayer authority, renderer mesh binding, dan asset import tetap di luar scope.

## Smoke wajib sebelum implementasi

Smoke baru, misalnya `runtime_skeletal_route_multisegment_smoke`, harus lulus Release serta AddressSanitizer dengan `detect_leaks=1` dan membuktikan hal berikut.

| Kasus | Bukti fail-closed |
|---|---|
| Metadata valid dua arah | Clip +X lalu clip +Z membawa route `(0,0)→(1,0)→(1,1)` dengan tepat satu root writer per tick |
| Transition tepat | Cursor pertama tiba, controller berikutnya mulai pada time nol, dan tidak ada second transform write pada tick arrival |
| Metadata hilang | Arah segmen berikutnya tidak tersedia; initialize/replan ditolak tanpa entity/controller partial |
| Metadata curang | Root start non-zero, diagonal/Y, stationary, panjang bukan satu, duration invalid, duplikat arah, atau clip/skeleton invalid ditolak sebelum ownership |
| Transition failure atomic | Clip kedua gagal init/validate; world di cell pertama, cursor, controller pertama, palette, dan gate tetap utuh |
| Pause/goal/authority | Semantik pause claim, reset unpaused, conflict, dan goal-stable dari mode satu segmen dipertahankan |
| Replan metadata preflight | Replacement membutuhkan arah tak tersedia ditolak tanpa player/world/cursor mutation |

Dokumen ini bukan implementasi multi-segmen dan tidak mengubah status readiness. Validator satu-clip yang tersedia hanya menutup preflight direction/start/terminal/duration; ia belum menyediakan registry empat arah, transition controller, route replan preflight, atau lifecycle runtime multi-segmen. Ia menetapkan informasi minimum agar langkah berikutnya tidak berakhir dengan konversi rotasi spekulatif atau blending transform ganda.
