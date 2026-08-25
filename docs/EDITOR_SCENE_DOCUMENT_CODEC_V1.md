# EditorSceneDocument Codec v1

## Status dan tujuan

`EditorSceneDocumentCodec` adalah envelope biner **in-memory** yang deterministik untuk model canonical `EditorSceneDocument` versi 1–3. Ia menyediakan batas serialisasi yang kecil dan fail-closed untuk alur editor: sebuah pemanggil dapat menyandikan snapshot SceneDocument ke bytes, lalu mendekodekan bytes tersebut kembali ke kandidat document sebelum meneruskannya ke `EditorSceneDocumentAdapter` atau `EditorSceneSession`.

> Codec ini bukan importer asset, tidak memeriksa readiness asset, dan tidak menggantikan validasi hierarchy/asset oleh `EditorSceneDocumentAdapter`.

## Envelope dan batas

Envelope dimulai dengan magic `FZSD`, codec version `1`, versi SceneDocument, dua byte reserved, scene ID length-prefixed, revision, jumlah actor, lalu actor dalam urutan vector saat diberikan caller. Nilai skalar multi-byte disandikan little-endian eksplisit; `float` menggunakan bit IEEE-754 32-bit yang sama dalam urutan byte tersebut. Encode ulang document hasil decode menghasilkan bytes identik untuk payload yang valid.

| Area | Kontrak v1 | Batas fail-closed |
|---|---|---|
| Payload | Maksimum 1 MiB | Bytes melebihi batas ditolak sebelum parsing. |
| Strings | `sceneId`, asset/material/name/texture ID length-prefixed | Setiap string maksimum 4.096 bytes. |
| Actors | Model `EditorSceneActor` v3 lengkap | Maksimum 512 actor, sama dengan adapter canonical. |
| Numeric fields | Transform dan dimensi sprite | Semua nilai harus finite saat encode maupun decode. |
| Actor kind | Enum existing sampai `Sprite` | Nilai enum tidak dikenal ditolak. |
| Framing | Payload harus persis habis | Payload terpotong atau trailing byte ditolak. |

Semua decode dilakukan ke `EditorSceneDocument` kandidat lokal. Output caller ditugaskan **hanya** setelah magic, versi, bounds, setiap field, semua actor, dan batas akhir payload tervalidasi. Karena itu malformed bytes, capacity overflow, atau trailing data mempertahankan document caller yang sebelumnya.

## Evidence executable

`editor_scene_document_codec_smoke` menjalankan round-trip SceneDocument v3 yang mencakup actor mesh dan sprite, fields asset/material/texture, transform, revision, layer/order/RGBA, serta equality bytes encode deterministik. Smoke yang sama menolak payload terpotong, magic dan versi yang salah, string length berlebih, actor kind tidak dikenal, transform NaN, actor count melampaui kapasitas, dan trailing byte sambil memverifikasi output caller tidak berubah. Ia juga memverifikasi penolakan encode untuk capacity dan transform non-finite tanpa mengganti bytes output sebelumnya.

| Gate | Hasil final |
|---|---|
| `editor_scene_document_codec_smoke` Release | Lulus; `bytes=206`, `actors=2`, deterministic dan atomic. |
| `editor_scene_document_codec_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 112/112 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 112/112 smoke lulus dengan `detect_leaks=1`. |

## Batas yang tetap terbuka

Codec v1 tidak menyediakan filesystem project, atomic file write, project browser, migrations lintas schema masa depan, compression, encryption, checksum, undo/redo, collaboration, network replication, asset import, hot reload, GPU upload, atau desktop editor UI. Tidak ada evidence pada increment ini untuk game production, renderer GPU, APK/AAB, multiplayer, atau release readiness.
