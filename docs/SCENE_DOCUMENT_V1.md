# Kontrak `SceneDocument v1`

`SceneDocument v1` adalah kontrak authoring terbatas untuk memindahkan scene editor ke runtime C++. Ia bukan protokol multiplayer, format save game pemain, atau jalur otoritas ekonomi. Kontrak ini memisahkan state editor dari `SceneWorld` sehingga kegagalan payload tidak mengubah scene runtime aktif.

## Envelope

| Field | Tipe | Aturan |
|---|---|---|
| `version` | integer | Harus `1`. |
| `scene_id` | string ASCII | Panjang 1–48; identifier untuk storage authoring, bukan entitas runtime. |
| `revision` | unsigned integer | Minimal `1`; backend menaikkan nilainya pada mutasi berhasil. |
| `actors` | array | Maksimal `512` actor pada v1. |
| `checksum` | string opsional | Dihitung backend atas payload kanonik; transport yang belum mendukung checksum harus tetap tervalidasi schema. |

## Actor

| Field | Tipe | Aturan |
|---|---|---|
| `id` | unsigned integer | Non-zero dan unik di dokumen. |
| `parent_id` | unsigned integer atau `null` | Harus menunjuk actor lain; tidak boleh self-parent atau membentuk siklus. |
| `kind` | string | V1 menerima `empty`, `mesh`, `light`, `camera`, `player_start`, atau `marker`. Jenis ini adalah metadata editor, bukan komponen executable penuh. |
| `transform` | object | Sembilan float finite: posisi, rotasi radian, dan skala. Skala harus positif. |
| `asset_id` | string opsional | Bila diisi, harus cocok dengan asset `Ready` pada registry runtime dan berupa `Mesh` atau `Prefab`. |

## Semantik commit

Editor hanya menghasilkan draft dan tidak menganggap sinkronisasi berhasil sebelum backend mengembalikan receipt `scene_id`, `revision`, dan checksum. Backend memvalidasi schema, ukuran, identifier, serta revision sebelum menyimpan record authoring pada JSON store yang diganti secara atomik; path store dikonfigurasi melalui `NEOENGINE_AUTHORING_STORE_PATH`. Adapter runtime memetakan dokumen tervalidasi ke `SceneWorld` kandidat, memeriksa asset registry, membangun hubungan parent, lalu mengganti scene target **hanya setelah seluruh kandidat berhasil**.

> Dengan semantik ini, payload rusak, asset hilang, parent siklik, transform tak-finite, atau kapasitas berlebih mengembalikan error dan mempertahankan scene aktif sebelumnya.

## Batas v1

V1 tidak membawa scripting, physics body, skeletal binding, material authoring penuh, file bytes, AI action, build/publish, atau kredensial. Domain tersebut harus masuk versi berikutnya hanya setelah v1 memiliki test round-trip editor → backend → adapter runtime → `SceneWorld` dan evidence regresi C++.
