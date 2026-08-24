# FauzanEngine Authoritative Persistence Design

## Status dan batas

Dokumen ini adalah **desain penerapan berikutnya**, bukan bukti bahwa persistence produksi, server multiplayer, atau recovery lintas-proses telah tersedia. Bukti saat ini hanya mencakup `AuthoritativeCommandGate`, loopback TCP satu klien yang dibatasi, dan checkpoint in-memory/local yang terversi. Tidak ada proses jaringan yang dideploy, basis data yang disediakan, kredensial yang dibuat, atau data pemain yang ditulis oleh pekerjaan ini.

## Tujuan kontrak

Layanan authority Farm produksi kelak harus menerima command dari sesi yang terautentikasi, menerapkan command tepat satu kali, mengembalikan revision/snapshot yang dapat direkonsiliasi, dan memulihkan world serta ledger replay tanpa mempercayai klien. Command ekonomi, top-up, ban, dan governance tetap dipisahkan ke jalur authority masing-masing; NPC, government, Coba, dan Aries tidak boleh memperoleh kemampuan tulis langsung terhadap saldo, receipt, atau ban.

| Data logis | Kunci dan batas utama | Peran authority |
|---|---|---|
| `player_session` | `session_id` unik, `player_id`, expiry, revocation, credential-hash hanya | Memetakan koneksi sah ke pemain tanpa menyimpan token mentah di save game. |
| `authority_player_cursor` | `player_id` unik, `last_sequence`, `revision`, rate-window | Mengunci urutan command dan cursor replay per pemain. |
| `authority_command` | unik `(player_id, command_id)` dan `(player_id, client_sequence)` | Menyimpan keputusan idempoten, revision, waktu server, dan hash payload terbatas. |
| `farm_world_snapshot` | `world_id`, `revision`, version, content-hash, bytes terenkripsi | Menyediakan titik rekonsiliasi dan recovery; bukan sumber kredensial. |
| `farm_checkpoint_manifest` | `world_id`, `revision`, snapshot reference, ledger reference, checksum | Mengikat world dan ledger pada satu revision transaksi. |
| `receipt_ledger` | receipt/provider reference unik, state verifikasi | Hanya jalur receipt authority yang boleh mengubahnya; tidak diterima dari command gameplay. |
| `trust_safety_evidence` | event id, player reference, immutable evidence hash, keputusan review | Menyimpan sinyal fraud/bukti appeal tanpa membolehkan agent mengubah ban langsung. |
| `authority_outbox` | event id unik, revision, delivered state | Menyediakan event telemetry/operational dari transaksi yang sudah committed. |

## Satu transaksi command

Implementasi database nantinya harus melaksanakan urutan berikut di dalam satu transaksi yang memiliki isolasi dan lock yang didokumentasikan untuk database terpilih. Pertama, layanan memverifikasi sesi dan status ban dari sisi server. Kedua, layanan mengambil lock cursor pemain/world yang relevan dan mencari `command_id`; hasil yang telah tersimpan dikembalikan sebagai replay tanpa mengeksekusi handler. Ketiga, layanan menolak sequence yang tidak tepat, tick tidak dalam jendela, payload melampaui batas, atau rate limit. Keempat, layanan menjalankan handler domain yang fail-closed dan menghitung revision baru. Terakhir, layanan menulis command decision, cursor, mutation world, snapshot/checkpoint reference jika dijadwalkan, serta outbox dalam commit yang sama.

Jika commit gagal, tidak boleh ada revision, economy mutation, command replay record, maupun outbox yang tampak sebagai sukses. Respon server hanya boleh dikirim setelah commit sukses. Replikasi atau publisher outbox harus bersifat at-least-once dan konsumen telemetry harus deduplikasi dengan event id; publisher tidak boleh mengubah keputusan gameplay.

## Kontrak recovery dan rekonsiliasi

Recovery harus memilih manifest terakhir dengan checksum utuh, memverifikasi versi world dan ledger, lalu membangun state baru sebelum atomically mengganti state aktif. Sesi tidak boleh dimasukkan ke checkpoint; setiap koneksi pascarecovery harus menjalani autentikasi dan bind ulang. Saat klient memiliki prediction lokal, server mengirim revision dan snapshot/delta yang bersumber dari commit. Klien membuang prediction pada revision konflik dan hanya mengaplikasikan perubahan yang ditandai server-authoritative.

| Kegagalan | Perilaku wajib | Bukti yang masih harus dibuat |
|---|---|---|
| Command duplikat setelah reconnect | Kembalikan keputusan/revision tersimpan tanpa handler kedua. | Dua koneksi nyata, database restart, dan assertion satu mutation. |
| Crash antara mutation dan response | Setelah restart, command dapat direplay dari record committed tanpa double-spend. | Kill/restart proses di titik fault injection. |
| Snapshot/ledger rusak | Tolak manifest, pilih recovery point tervalidasi sebelumnya, audit kejadian. | Uji checksum, truncation, dan rollback database. |
| Dua command berurutan bersaing | Hanya sequence tepat berikutnya yang commit; yang lain ditolak atau direplay. | Uji konkurensi multi-koneksi dan isolation database. |
| Provider receipt gagal | Tidak menambah saldo; record audit/pending terpisah dari command gameplay. | Uji sandbox provider, signature, duplicate, retry, dan appeal. |

## Release-gate yang tetap terbuka

Sebelum klaim persistence atau multiplayer produksi, perlu dipilih database/hosting secara eksplisit, dibangun migration yang direview, diuji backup/restore, enkripsi serta key rotation, authentication/TLS/rate limiting pada edge, concurrent rooms, observability/SLO, penghapusan/retensi data, security review, dan failure/load test dengan klien nyata. Keputusan hosting baru dapat dibuat setelah target koneksi, ukuran world, CPU/memory, throughput persistence, serta kebutuhan operasi dicatat; sandbox default tidak boleh diperlakukan sebagai game server jangka panjang.
