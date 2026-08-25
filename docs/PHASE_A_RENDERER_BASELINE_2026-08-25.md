# Fase A Renderer Baseline — 25 Agustus 2026

## Status ringkas

Fase A telah menghasilkan **baseline renderer software yang lebih terintegrasi dan teruji**, bukan renderer setara Unreal atau renderer produksi. Jalur aktif kini mencakup proyeksi camera berorientasi, SpriteBatch berlayer dengan alpha/tint, mesh bertekstur/material/light directional, presentasi SDL opt-in, vertical slice Farm finite, dan vertical slice mesh 3D static finite yang menghasilkan artefak PPM. Semua perubahan pada baseline ini dibuktikan melalui smoke target Release dan ASAN dengan `detect_leaks=1`.

> `ReadyPresent` pada capability renderer hanya berarti backend CPU → SDL surface tersedia dan dapat diminta oleh runtime. Ia **bukan** bukti bahwa window end-user sedang aktif, GPU renderer tersedia, atau game desktop siap rilis.

| Area | Evidence kanonis | Status yang tepat | Batas utama |
|---|---|---|---|
| Presentasi surface | `software_surface_presenter_smoke`, `renderer_capability_smoke`, commit `6c90696` | CPU frame dapat diunggah ke SDL texture dan dipresentasikan secara opt-in. | Bukan swapchain GPU, event loop host lengkap, resize, fullscreen, atau vsync policy. |
| Vertical slice Farm | `farm_surface_demo_smoke`, commit `3bb4a93` | Farm finite merender dan menghasilkan PPM 256×256 melalui NeoRuntime. | Bukan game interaktif, input window, UI, audio, networking, atau APK. |
| Vertical slice mesh 3D | `mesh_surface_demo_smoke` | Mesh perspektif berpose finite dengan CPU texture, material tint, directional light, lifecycle SDL, dan PPM berjalan melalui jalur renderer kanonis. | Bukan animation system, game 3D playable, atau host desktop persisten. |
| Kamera | `render_camera_smoke`, commits `db5f21f` dan `534e633` | Orthographic/perspective memakai orientasi forward/up tervalidasi; mesh memakai camera-space. | Tanpa controller, follow target, frustum culling penuh, stereo, atau matrices publik. |
| Sprite | `sprite_batch_smoke`, commits `c30173b` dan `03cbc2d` | Sorting stabil layer/order/sequence, alpha/tint, dan flush framebuffer atomik. | Tanpa atlas, flipbook, clipping quad, GPU batching, atau transparency sort kompleks. |
| Mesh/material/texture | `mesh_renderer_smoke`, commits `534e633`, `21b4466`, dan `94660cb` | Six-plane camera-space clipping/depth, staged PPM/BMP texture, material tint, bounded Lambert intensity, dan draw atomik. | Tanpa scene-wide culling, PBR, shadow, normal map, multiple lights, atau GPU mesh path. |
| Authoring-to-runtime visual seams | `editor_scene_mesh_binder_smoke`, `authoring_catalog_visual_binder_smoke` | Jalur staged material/texture tetap tervalidasi setelah tint material diterapkan. | Tidak menggantikan editor visual atau tool authoring lengkap. |

## Evidence regresi

Validasi terakhir baseline menjalankan semua executable `*_smoke` non-Vulkan langsung dari build canonical, dengan pengecualian `sdl_audio_bridge_smoke` sesuai konvensi suite yang telah dipakai sebelumnya. Hasilnya adalah **96/96 Release** dan **96/96 ASAN** dengan `detect_leaks=1`. Vulkan probe dan audio bridge tidak termasuk angka ini; keduanya memerlukan gate environment/runtime sendiri dan tidak boleh disamakan dengan bukti renderer aktif.

| Gate evidence | Hasil | Arti |
|---|---|---|
| Smoke target per milestone | Lulus Release + ASAN | Kontrak lokal setiap increment diuji. |
| Regression suite non-Vulkan | 96/96 Release | Tidak ada kegagalan smoke canonical dalam cakupan tersebut. |
| Regression suite non-Vulkan | 96/96 ASAN `detect_leaks=1` | Tidak ada temuan sanitizer pada cakupan tersebut. |
| Source archive | GitHub `main` hingga `b5afd21` sebelum milestone demo mesh ini | Milestone source yang telah dibuktikan dapat direview. |

## Correctness contract yang sekarang aktif

Sprite dan mesh tidak lagi meninggalkan frame parsial ketika sebuah item berikutnya gagal proyeksi atau raster: masing-masing memakai salinan `SoftwareRenderer` kandidat dan melakukan commit hanya setelah draw lengkap berhasil. Kontrak tersebut **hanya** mencakup pixel/depth framebuffer untuk satu panggilan render. Ia tidak menyatakan bahwa Farm state, `SceneWorld`, registry asset, staging, agent, RouteIntent, atau MovementAuthorityGate ikut transactional.

Hal yang sama berlaku untuk renderer surface. `NeoRuntime` tetap headless secara default. Ketika `enableSoftwareSurfacePresentation=true`, kegagalan inisialisasi atau presentasi menghasilkan `PresentationFailed`; runtime tidak berpura-pura berhasil dalam mode headless.

## Gate yang masih belum terpenuhi

| Gate renderer lanjut | Mengapa masih terbuka |
|---|---|
| Host window interaktif | Demo finite kini memompa event dan membatalkan present setelah close request, tetapi belum ada pump event window berkelanjutan, resize, DPI, pacing, atau vsync policy. |
| GPU runtime renderer | Vulkan yang ada masih berupa probe/offscreen terpisah; belum menerima output NeoRuntime/Farm/MeshRenderer aktif. |
| 2D production path | Atlas, animation/flipbook, clipping, sampler modes, render target, dan performance batching belum ada. |
| 3D production path | Culling scene-wide, PBR, shadows, multiple lights, material graph, skeletal GPU skinning, animation system yang tervalidasi, dan profiling frame belum ada. |
| Hardware evidence | Belum ada hasil build/run surface visible pada device target, compatibility matrix driver, atau benchmark render workload. |
| Release evidence | Belum ada installer/package, APK/AAB, UX testing, telemetry release, multiplayer, anti-cheat release, atau Play gate. |

## Kesimpulan operasional

Baseline ini cukup untuk melanjutkan pekerjaan renderer secara disiplin pada seam yang jelas, tetapi **belum cukup** untuk mengklaim renderer usable untuk game komersial, Unreal-like parity, atau production readiness. Prioritas Fase A berikutnya sebaiknya dipilih hanya setelah menentukan gate mana yang diperlukan: culling scene-wide 3D, host window interaktif yang terukur, perluasan jalur 2D, atau pipeline GPU yang benar-benar terhubung ke runtime. Tidak ada alasan evidence-based untuk melewati gate tersebut menuju APK, website, atau klaim rilis.
