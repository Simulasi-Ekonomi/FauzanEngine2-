# Evaluasi Parser OBJ Bounded

## Keputusan

Importer tahap ini akan memakai **TinyObjLoader** sebagai parser data Wavefront OBJ in-memory yang dipin dan divendor dengan lisensi MIT. Kandidat ini dipilih karena menyediakan implementasi C++ header-only tanpa dependensi di luar STL, mendukung posisi, normal, UV, serta triangulasi, dan dokumentasinya menjelaskan tata letak indeks atribut secara eksplisit.[1]

| Kandidat | Kesesuaian | Keputusan |
|---|---|---|
| TinyObjLoader | C++11 header-only, lisensi MIT, API atribut/index matang, dapat memuat dari string | Dipilih untuk adapter bounded. |
| rapidobj | C++17 header-only dan cepat, tetapi memerlukan pengelolaan material/file yang lebih luas untuk API utamanya | Tidak dipilih untuk tahap bounded ini. |

## Batas adapter FauzanEngine

Adapter akan menerima **byte/string OBJ in-memory saja**; tidak ada file I/O, `.mtl`, path resolver, network, atau pemuatan tekstur. Hasil harus membentuk `MeshVertex` dan indeks `uint16_t` yang tidak melebihi batas kanonis renderer CPU, yaitu 2.048 vertex dan 6.144 indeks. Parser atau adapter akan menolak input kosong, error parse, indeks atribut hilang/tidak valid ketika dirujuk, nilai posisi/normal/UV non-finite, hasil kosong, dan kapasitas yang melampaui batas tanpa mengganti output caller.

Triangulasi dilakukan parser lalu adapter mendeduplikasi kombinasi indeks posisi/normal/UV menjadi vertex CPU yang stabil. Atribut normal atau UV yang tidak ada tetap dibentuk secara eksplisit menggunakan default renderer (`normal = (0,0,1)`, `uv = (0,0)`); material, smoothing-group generasi normal, mesh hierarchy, skinning, dan GPU upload tetap di luar tahap ini.

## Referensi

[1] [TinyObjLoader — GitHub](https://github.com/tinyobjloader/tinyobjloader)

[2] [rapidobj — GitHub](https://github.com/guybrush77/rapidobj)
