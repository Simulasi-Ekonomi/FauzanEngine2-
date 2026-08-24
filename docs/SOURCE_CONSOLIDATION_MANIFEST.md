# Manifest Konsolidasi Source FauzanEngine

**Status:** rencana integrasi berbasis inventaris; belum menjadi bukti readiness produk.

Dokumen ini menetapkan cara mengonsolidasikan workspace penuh FauzanEngine tanpa membuang editor, backend, agen, Android, atau source engine historis. Prinsipnya adalah **preserve first, integrate by contract, replace only proven stubs**. Tidak ada subtree yang boleh ditimpa massal hanya karena memiliki fungsi atau klaim yang mirip.

## Inventaris dan keputusan awal

| Subtree | Peran teramati | Keputusan | Batas integrasi |
|---|---|---|---|
| `Source/NeoEngine/` | Runtime C++ kanonis, `neo_core`, smoke test, XPBD V5, `SceneWorld`, registry/manifest asset, runtime persistence. | **Pertahankan sebagai source of truth runtime.** | Perubahan perilaku harus masuk CMake aktif dan disertai smoke/ASAN yang relevan. |
| `Tests/` | Bukti executable smoke kanonis. | **Pertahankan.** | Tidak dipakai sebagai implementasi produk atau pengganti kontrak runtime. |
| `engine/Source/NeoEngine/` | Source legacy yang lebih luas, termasuk banyak sistem domain dan codepath historis. | **Pertahankan, klasifikasikan per modul sebelum migrasi.** | Tidak boleh ditautkan massal ke `neo_core`; setiap modul harus punya owner, dependensi, dan test. |
| `editor/` | Editor React/Vite dengan Three.js, viewport, Outliner, Properties, Content Browser, dan Aries Console. | **Pertahankan dan adaptasikan.** | Editor adalah client authoring; state browser bukan state runtime authoritative. |
| `backend/` | FastAPI, Aries, game director, bridge WebSocket, API editor. | **Pertahankan dan ganti endpoint placeholder prioritas.** | Tidak mendapat otoritas langsung untuk proses game, ekonomi, ban, deploy, atau kredensial. |
| `android/`, `jni/` | Pembungkus Android/JNI dan bridge platform. | **Pertahankan sebagai adapter platform.** | Hanya dihubungkan setelah kontrak runtime desktop terbukti; tidak membawa binary/generated output. |
| `World/`, `Assets/`, `skills/`, `tools/`, `Agents/` | Data authoring, automation, definisi skill/tool, dan source pendukung. | **Pertahankan setelah sanitasi.** | Hanya manifest/hash/data reviewable; vault, cache, `.env`, database lokal, dan output hasil build tetap dikecualikan. |

## Bukti batas saat ini

`SceneWorld` aktif memiliki entity ber-generasi, transform, parent, update transform, dan serialisasi biner dengan kapasitas tetap 4.096 entitas. `AssetManifestSnapshot` aktif menangani snapshot registry, serialisasi, checksum, dan kecocokan registry. Sebaliknya, actor editor saat ini berada terutama pada Zustand/local storage; API `GET/POST/DELETE /scene/actors` backend masih placeholder. Oleh sebab itu, editor belum boleh dinyatakan terhubung end-to-end ke runtime C++.

> Konsekuensi teknis: **editor yang dapat membuat cube secara visual tidak identik dengan engine runtime yang dapat memuat, memvalidasi, menjalankan, menyimpan, dan merilis scene tersebut.** Keduanya perlu dihubungkan melalui kontrak eksplisit, bukan dengan salin file antar-subtree.

## Kontrak integrasi pertama yang disetujui untuk dibangun

| Lapisan | Input yang diizinkan | Output yang wajib | Validasi/gate |
|---|---|---|---|
| Editor | `SceneDocument v1` berisi actor, transform, parent, komponen render dasar, dan referensi asset. | JSON versi eksplisit dengan ID stabil dan checksum payload. | Validasi schema lokal; tidak ada API key atau tindakan publish di client. |
| Backend authoring | Permintaan create/read/update/delete `SceneDocument v1`. | Record terversi dan receipt perubahan; tidak mengklaim state permainan. | Schema validation, batas ukuran/actor, audit actor ID, dan authority gate. |
| Adapter runtime | `SceneDocument v1` tervalidasi serta `AssetManifestSnapshot v1`. | Candidate `SceneWorld` + daftar resource scene yang tidak mengubah state lama bila gagal. | Parse bounded, cek parent/transform/capacity, checksum asset, commit atomik. |
| `NeoRuntime` | Candidate scene yang telah lolos adapter. | Snapshot `SceneWorld` untuk simulation/presentation dan hasil error terstruktur. | Satu writer transform/lifecycle; kegagalan tidak mengubah scene aktif. |

Kontrak ini sengaja sempit. Ia tidak mencoba langsung memigrasikan seluruh blueprint, AI, economy, multiplayer, atau Android. Sasaran awalnya adalah membuktikan satu scene editor yang dapat disimpan, divalidasi, dimuat ke runtime C++, lalu dipresentasikan melalui backend yang dipilih dengan state dan error yang dapat diaudit.

## Prioritas penggantian stub

| Prioritas | Stub/gap | Perbaikan terukur | Bukti selesai |
|---|---|---|---|
| P0 | API actor backend mengembalikan daftar kosong atau echo payload. | Ganti dengan repository `SceneDocument` terversi dan schema validation. | Test API create/read/update/delete, rejection payload rusak, dan receipt versi. |
| P0 | Editor menyimpan scene terutama di browser. | Tambahkan client adapter terpisah dari Zustand untuk sinkronisasi dokumen versi. | Test unit serialisasi/round-trip dan state error UI. |
| P1 | `SceneWorld` hanya biner runtime, tidak menerima model editor. | Implementasikan adapter `SceneDocument v1 → candidate SceneWorld` yang bounded. | Smoke C++ untuk transform, parent, capacity, asset missing, corruption, dan atomic failure. |
| P1 | Tidak ada bukti editor → runtime → presentasi. | Tambahkan satu integration harness headless dan satu scene Farm kecil yang benar-benar round-trip. | Test end-to-end tanpa jaringan produksi, AI, atau publish. |
| P2 | Editor/Android punya build/publish UX lebih luas dari capability tervalidasi. | Ubah UI menjadi capability-gated sampai pipeline build yang nyata tersedia. | UI dan API menolak action yang tidak tersedia secara eksplisit. |

## Perlindungan baseline

XPBD V5 yang memiliki evidence benchmark dan regression dipertahankan sebagai bagian dari C++ kanonis, bukan dijadikan alasan untuk menimpa editor atau legacy engine. Benchmark acceptance yang tercatat adalah konfigurasi khusus dan tidak menggantikan validasi Android atau seluruh workload gameplay. Integrasi berikutnya harus mempertahankan smoke Release dan AddressSanitizer yang sudah berlaku sebelum dan sesudah perubahan.

## Larangan konsolidasi

Source generated, `node_modules`, Gradle cache, binary JNI, backup, RAR, database/vault Coba/Aries, secret, dan konfigurasi perangkat lokal tidak masuk jalur source-of-truth. Aksi agen juga tetap hanya draft/planning/approval-gated; manifest ini tidak memberi otoritas runtime, ekonomi, ban pemain, kredensial, atau deployment kepada Aries maupun Coba.
