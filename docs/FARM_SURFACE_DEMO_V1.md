# Farm Surface Demo v1 — Finite Graphical Vertical Slice

## Tujuan

`farm_surface_demo` adalah executable native terbatas yang menghubungkan `NeoRuntime`, `FarmWorldTool`, CPU `FarmRenderAdapter`, dan `SoftwareSurfacePresenter`. Demo ini membuat satu Farm 8×8, lima NPC, karakter di tile (2,2), Barn di tile (4,4), serta tile Wheat yang diolah, ditanam, dan disiram sebelum menjalankan sejumlah frame yang dibatasi.

| Tahap | Komponen kanonis | Bukti output |
|---|---|---|
| Inisialisasi | `NeoRuntime` + `TrustSafetySystem` + `FarmWorldTool` | Runtime dan SDL surface optional harus berhasil. |
| Setup scene | Permit Barn dan aksi Farm pemain yang tervalidasi | Snapshot memuat satu building dan lima NPC. |
| Frame | `NeoRuntime::Tick` lalu `RenderFarm` dengan Farm HUD runtime | Kandidat CPU world+HUD dipresentasikan ke SDL surface sebelum receipt/renderer dikomit. |
| Artefak | `SoftwareRenderer::WritePpm` pada renderer yang telah dikomit | PPM `P6`, `worldFrameHash`, `hudFrameHash`, dan jumlah presentasi berasal dari receipt runtime yang sama. |

## Penggunaan

```bash
./farm_surface_demo --frames 8 --output farm_surface_demo.ppm
./farm_surface_demo --visible --frames 120 --output farm_surface_visible.ppm
```

Mode default memakai surface tersembunyi untuk determinisme smoke. Opsi `--visible` meminta SDL membuka window, tetapi demo tetap finite dan tidak menerima input pengguna atau memelihara event loop. Opsi tersebut adalah mekanisme inspeksi host, bukan bukti aplikasi desktop interaktif.

## Bukti

`farm_surface_demo_smoke` menjalankan empat frame hidden surface, memverifikasi receipt runtime dengan empat frame present, satu building, lima NPC, hash world/HUD yang berbeda, dan artefak PPM valid. Ia juga membuktikan konfigurasi ukuran di bawah batas ditolak. Executable digunakan pada Release dan ASAN dengan `detect_leaks=1` untuk menghasilkan frame beserta PPM.

Suite non-Vulkan penuh dicatat oleh runner regresi canonical; artefak demonstrasi tidak disimpan ke GitHub karena merupakan output build, bukan source.

## Batas

Vertical slice ini menggunakan renderer Farm warna CPU yang sudah aktif, bukan texture Farm runtime, GPU material pipeline, lighting baru, 3D camera interaktif, input window, animation, audio, resize, save/load, networking, atau APK. Ia memenuhi bukti jalur grafis finite dan artefak frame, namun belum membuktikan aplikasi game yang dapat dimainkan atau rilis desktop.
