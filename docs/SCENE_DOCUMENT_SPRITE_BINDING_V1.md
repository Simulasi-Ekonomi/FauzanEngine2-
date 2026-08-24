# SceneDocument v3 — Sprite Binding Contract

## Tujuan dan ruang lingkup

Dokumen ini menjelaskan jalur terbatas untuk membawa satu actor `sprite` dari authoring scene menuju quad bertekstur di runtime kanonis. Jalur ini menambah **representasi 2D bertekstur yang dapat diuji**, bukan player game, tilemap, atlas, animator sprite, renderer GPU, atau bukti game siap rilis.

| Tahap | Komponen | Kontrak yang dibuktikan |
|---|---|---|
| Authoring | `editor/src/services/sceneDocumentClient.ts` | Actor editor `sprite` diserialisasi sebagai `kind: "sprite"`, `asset_id`, ukuran, layer/order, dan warna RGBA. |
| Penyimpanan | `backend/api/scene_documents.py` | SceneDocument v3 memvalidasi sprite secara bounded, memakai optimistic revision, dan membuat receipt checksum. |
| Handoff lokal | `backend/api/local_authoring_handoff.py` dan `LocalAuthoringBridge` | Hanya panggilan lokal yang telah diberi `approved=True` dapat membentuk dan memuat envelope `NAB3`. |
| Asset | `AssetRegistry` dan `TextureStagingStore` | `asset_id` sprite harus menjadi asset `Texture` berstatus `Ready`; binder menerima PPM P6 atau BMP BI_RGB yang berhasil di-stage. |
| Runtime | `EditorSceneSpriteBinder`, `SceneSpriteAdapter`, dan `SpriteBatch` | Texture CPU disalin ke instance adapter, quad diantrikan berdasarkan transform `SceneWorld`, lalu diraster oleh `SoftwareRenderer`. |

## Schema v3 dan kompatibilitas

SceneDocument v3 menambahkan actor `sprite`. Nilai numerik jenis actor lama dipertahankan untuk menjaga interpretasi `NAB1` dan `NAB2`; `Sprite` ditambahkan sebagai nilai baru setelah `Marker`. Dokumen v1/v2 tidak boleh membawa actor atau properti sprite.

| Field sprite | Validasi backend dan runtime | Efek runtime |
|---|---|---|
| `kind` | Harus `sprite` dan hanya legal pada v3. | Dipilih oleh `EditorSceneSpriteBinder`. |
| `asset_id` | Wajib, printable ASCII, menunjuk asset `Texture` yang `Ready`. | Menentukan bytes PPM/BMP yang di-stage. |
| `sprite_width`, `sprite_height` | Wajib, finite, dan positif. | Menentukan ukuran quad sebelum skala transform dunia diterapkan. |
| `sprite_layer`, `sprite_order` | Wajib, `int16` bounded. | Menentukan urutan stable di `SpriteBatch`. |
| `sprite_rgba` | Wajib, `uint32` bounded. | Disimpan sebagai metadata instance; texture renderer software saat ini belum menerapkan tint/alpha blending. |

Envelope `NAB3` memuat layout actor dasar, tiga string binding v2, lalu `width`, `height`, `layer`, `order`, dan `rgba`. Field tambahan juga diserialisasi dengan nilai default bounded untuk actor non-sprite agar layout v3 tetap deterministik. Bridge menolak magic, version, ukuran, string, enum, atau trailing byte yang tidak valid sebelum `EditorSceneDocumentAdapter` membuat kandidat `SceneWorld`.

## Atomisitas dan lifecycle

`EditorSceneSpriteBinder` membuat kandidat `SceneSpriteAdapter` dan hanya mengganti adapter target setelah semua actor sprite telah lolos staging serta memiliki entity dari adapter dokumen. `SceneSpriteAdapter` tidak memiliki entity dan tidak menulis transform. Ia hanya menyimpan snapshot texture CPU miliknya sendiri dan membaca transform dunia dari `SceneWorld` saat frame diantrikan. Karena itu, perpindahan entity yang sah terlihat pada frame berikutnya tanpa duplikasi ownership scene.

> Staging texture dapat memperbarui cache yang telah ada sebelum kandidat adapter diterima. Namun, kegagalan binder tidak mengganti adapter sprite target atau `SceneWorld` yang telah aktif.

## Bukti lokal yang tersedia

Smoke `editor_scene_sprite_binder_smoke` mengimpor satu PPM 1×1, memuat dokumen v3, men-stage texture, meraster sprite, memindahkan transform entity, dan membuktikan penolakan dokumen tanpa sprite tanpa mengganti adapter yang lama. Smoke `local_authoring_bridge_smoke` mempertahankan approval gate serta bukti `NAB1` dan `NAB2`, lalu menambah parsing `NAB3` sprite. Seluruh **90/90** smoke non-Vulkan Release dan **90/90** smoke non-Vulkan ASAN dengan `detect_leaks=1` lulus; backend unittest lulus **11/11**, termasuk persistensi schema v3 dan serialisasi bounded `NAB3`.

## Batas yang tetap terbuka

Editor viewport saat ini masih memakai preview plane berwarna dan belum membaca `asset_id` texture dari contract ini, sehingga tidak ada klaim WYSIWYG texture preview. Pemeriksaan `pnpm exec tsc --noEmit` pada editor juga diblokir **sebelum** TypeScript berjalan karena policy package manager menolak build script `esbuild`; script tidak di-approve atau dipaksa. Karena itu serializer v3 belum memiliki bukti build editor produksi baru pada sandbox ini. Tidak ada tilemap/atlas/flip/animation sprite, clipping quad, alpha blending, batching GPU, PNG/JPEG, audio, skeletal asset, gameplay loop, multiplayer, anti-cheat, Android package, atau proof perangkat. `SpriteBatch` menolak quad yang keluar dari frustum alih-alih melakukan clipping; hal tersebut merupakan batas renderer software saat ini, bukan dukungan camera production.
