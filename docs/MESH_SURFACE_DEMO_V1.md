# Mesh Surface Demo V1

## Tujuan dan batas

`MeshSurfaceDemo` adalah **vertical slice renderer 3D finite** untuk memeriksa satu jalur aktif secara utuh: `RenderCamera` perspektif, `MeshRenderer`, texture CPU 2×2, material tint, directional light, framebuffer `SoftwareRenderer`, lifecycle `SoftwareSurfacePresenter`, dan artefak PPM. Ia tidak menggunakan `NeoRuntime`, Farm, `SceneWorld`, RouteIntent, `MovementAuthorityGate`, registry asset, agent, maupun authority ekonomi.

> Demo ini membuktikan presentasi 3D software yang terbatas dan terukur. Ia **bukan** host desktop interaktif, gameplay, animation system, renderer GPU, APK, atau bukti kesiapan rilis.

| Kontrak | Ketentuan fail-closed |
|---|---|
| Dimensi surface | Lebar dan tinggi masing-masing wajib `32..1024`. |
| Frame finite | Wajib `1..600`; tidak terdapat loop event atau game loop yang berjalan terus-menerus. |
| Artefak | Path PPM wajib tidak kosong dan paling banyak 256 karakter. PPM hanya ditulis setelah seluruh frame berhasil. |
| Scene | Satu piramida empat sisi bertekstur, normal tervalidasi, UV sah, material tint, ambient, dan directional light. |
| Presentasi | Setiap frame menjalankan `Clear → MeshRenderer::Draw → PumpEvents → CloseRequested check → Present`. Kegagalan pada satu tahap menghentikan demo dengan error spesifik. |
| Receipt | Hanya mencatat jumlah frame render/present, jumlah pixel non-latar terakhir, dan hash framebuffer terakhir. |

Texture checker 2×2 dalam demo adalah `CpuTextureResource` immutable yang valid untuk kontrak `MeshRenderer`. Ia dipilih agar proof ini tidak memperluas scope ke import, `AssetRegistry`, atau reload. Jalur staging asset yang telah ada tetap merupakan seam terpisah.

## Perspektif dan pose scene

Kamera berada di origin dengan mode perspective, FOV vertikal 60°, near clip 0,1, far clip 20, dan aspect ratio yang diturunkan dari konfigurasi surface. Mesh berada pada ruang kamera positif, memakai material `0xFFE6F4FF`, ambient `0,15`, directional `0,85`, serta light menuju `-Z` dengan intensitas 1.

Demo mengubah pose Y piramida secara kecil dan deterministic pada setiap frame finite. Hal ini hanya memeriksa transform mesh bersama clipping frustum aktif; ini bukan playback skeletal, state machine, blend, root motion, atau animation system. Perubahan pose dibatasi agar konfigurasi default tetap menghasilkan pixel non-latar pada frame akhir.

## Antarmuka executable

```bash
build=/home/ubuntu/work/fauzan_engine/build/neoengine
"$build/mesh_surface_demo" --frames 3 \
  --output /home/ubuntu/Downloads/fauzanengine_mesh_surface_phase_a.ppm
```

`--visible` meminta surface SDL terlihat. Tanpa flag tersebut, surface tetap hidden agar smoke dapat dijalankan di environment sandbox. `--frames <n>` dan `--output <path>` menolak argumen tidak valid melalui exit code executable. Tidak ada mode yang menjadikan demo sebagai loop permanen.

## Evidence

| Gate | Hasil aktual |
|---|---|
| Target `mesh_surface_demo_smoke` Release | Lulus: konfigurasi invalid ditolak, empat frame hidden dipresentasikan, pixel non-latar dan hash dicatat, serta artefak diawali magic `P6`. |
| Target `mesh_surface_demo_smoke` ASAN | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Executable `mesh_surface_demo` Release | Lulus untuk tiga frame; menghasilkan PPM eksplisit pada `/home/ubuntu/Downloads/fauzanengine_mesh_surface_phase_a.ppm`. |
| Executable `mesh_surface_demo` ASAN | Lulus untuk tiga frame dengan `detect_leaks=1`; artefak sementara dibersihkan setelah validasi. |
| Suite non-Vulkan | **96/96 Release** dan **96/96 ASAN** dengan `detect_leaks=1`; `vulkan_*` dan `sdl_audio_bridge_smoke` tetap dikecualikan sesuai konvensi. |

## Batas yang tetap terbuka

Demo ini memanggil `PumpEvents` sebelum setiap present dan membatalkan operasi jika `CloseRequested`, tetapi ia tidak menyediakan event loop desktop, resize, DPI, vsync, pacing, input, fullscreen, maupun recovery window. Kontrak close injection dan reset tetap diuji pada `software_surface_presenter_smoke`; demo ini hanya mengonsumsi kontrak tersebut secara fail-closed.

Jalur 3D sudah memiliki clipping frustum lokal pada MeshRenderer, tetapi masih tidak mempunyai scene-wide culling, PBR, shadow, multiple lights, material graph, skeletal playback, physics/collision, profiling frame, GPU render path, atau benchmark device target. Karena itu vertical slice ini tidak membuktikan game 3D playable, host desktop lengkap, atau readiness produksi/rilis.
