# Evaluasi Importer MTL Bounded

## Keputusan

Tahap ini akan menggunakan **TinyObjLoader** yang telah dipin dan divendor untuk parsing MTL in-memory. Header yang sama telah memiliki `material_t` dengan properti diffuse (`Kd`), dissolve (`d`), dan nama material, serta API material yang konsisten dengan parser OBJ.[1]

| Ruang lingkup | Keputusan tahap ini |
|---|---|
| Input | String MTL in-memory maksimal 1 MiB. |
| Seleksi | Tepat satu nama material yang diminta caller. |
| Output | `MeshMaterial` CPU dengan `rgba` dari `Kd` dan alpha dari `d`. |
| Validasi | Nama valid, material ada dan unik, semua `Kd`/`d` finite serta pada rentang `[0,1]`; output caller dipertahankan saat penolakan. |
| Tidak didukung | File I/O, `mtllib` path resolution, texture map, normal/bump map, PBR, material multi-pass, shader, GPU binding, dan reload. |

Parser TinyObjLoader hanya boleh didefinisikan satu kali pada translation unit terpisah. Adapter OBJ dan adapter MTL akan memasukkan header tanpa mendefinisikan implementasi ulang.

## Referensi

[1] [TinyObjLoader — GitHub](https://github.com/tinyobjloader/tinyobjloader)
