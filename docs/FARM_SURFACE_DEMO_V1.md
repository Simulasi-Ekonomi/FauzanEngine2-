# Farm Surface Demo v1 — Finite Graphical Vertical Slice

## Tujuan

`farm_surface_demo` adalah executable native terbatas yang menghubungkan `NeoRuntime`, `FarmWorldTool`, CPU `FarmRenderAdapter`, dan `SoftwareSurfacePresenter`. Demo ini membuat satu Farm 8×8, lima NPC, karakter di tile (2,2), Barn di tile (4,4), serta tile Wheat yang diolah, ditanam, dan disiram sebelum menjalankan sejumlah frame yang dibatasi.

| Tahap | Komponen kanonis | Bukti output |
|---|---|---|
| Inisialisasi | `NeoRuntime` + `TrustSafetySystem` + `FarmWorldTool` | Runtime dan SDL surface optional harus berhasil. |
| Setup scene | Permit Barn dan aksi Farm pemain yang tervalidasi | Snapshot memuat satu building dan lima NPC. |
| Frame | `NeoRuntime::Tick` lalu `RenderFarm` | Frame CPU diraster dan dipresentasikan ke SDL surface. |
| Artefak | `SoftwareRenderer::WritePpm` | PPM `P6` berisi frame akhir untuk inspeksi. |

## Penggunaan

```bash
./farm_surface_demo --frames 8 --output farm_surface_demo.ppm
./farm_surface_demo --visible --frames 120 --output farm_surface_visible.ppm
```

Mode default memakai surface tersembunyi untuk determinisme smoke. Opsi `--visible` meminta SDL membuka window, tetapi demo tetap finite dan tidak menerima input pengguna atau memelihara event loop. Opsi tersebut adalah mekanisme inspeksi host, bukan bukti aplikasi desktop interaktif.

## Bukti

`farm_surface_demo_smoke` menjalankan empat frame hidden surface, memverifikasi receipt dengan empat frame present, satu building, lima NPC, hash frame nonnol, dan artefak PPM valid. Ia juga membuktikan konfigurasi ukuran di bawah batas ditolak. Executable digunakan pada Release dan ASAN dengan `detect_leaks=1` untuk menghasilkan tiga frame beserta PPM.

Suite non-Vulkan penuh mencapai **95/95 Release** dan **95/95 ASAN**. Artefak demonstrasi Release yang terpisah dihasilkan sebagai PPM 256×256 dengan hash frame `6179471946010819966`; artefak tidak disimpan ke GitHub karena merupakan output build, bukan source.

## Batas

Vertical slice ini menggunakan renderer Farm warna CPU yang sudah aktif, bukan texture Farm runtime, GPU material pipeline, lighting baru, 3D camera interaktif, input window, animation, audio, resize, save/load, networking, atau APK. Ia memenuhi bukti jalur grafis finite dan artefak frame, namun belum membuktikan aplikasi game yang dapat dimainkan atau rilis desktop.
