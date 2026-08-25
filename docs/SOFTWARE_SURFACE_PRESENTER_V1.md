# Software Surface Presenter v1 — Optional SDL Presentation

## Ruang Lingkup

Sebelum milestone ini, runtime gameplay kanonis `NeoRuntime` hanya meraster Farm ke `SoftwareRenderer` offscreen. `VulkanPresentProbe` dapat membuat dan menyajikan clear frame tersembunyi sebagai probe terpisah, tetapi tidak menerima output renderer aktif. `SoftwareSurfacePresenter` menutup seam tersebut secara terbatas: ia menyalin frame ARGB CPU dari `SoftwareRenderer` ke `SDL_Texture`, melakukan `SDL_RenderCopy`, lalu memanggil `SDL_RenderPresent` pada window SDL.

| Komponen | Peran | Mutasi yang diizinkan |
|---|---|---|
| `SoftwareRenderer` | Memiliki frame CPU dan rasterisasi aktif. | Tidak dimutasi presenter. |
| `SoftwareSurfacePresenter` | Memiliki window/renderer/texture SDL dan mengunggah snapshot frame. | Hanya resource presentasi SDL. |
| `NeoRuntime::RenderFarm` | Merender Farm lalu mengirim frame ke presenter bila runtime opt-in. | Menetapkan `PresentationFailed` bila presentasi diminta tetapi gagal. |

## Kontrak

`RuntimeConfig::enableSoftwareSurfacePresentation` default-nya `false`, sehingga baseline headless tidak berubah. Jika `true`, `NeoRuntime::Initialize` membuat presenter dengan ukuran render yang sama. Kegagalan inisialisasi SDL bukan fallback diam-diam; inisialisasi runtime gagal dengan `RuntimeError::PresentationFailed`. `softwareSurfaceHidden` default-nya `true` untuk test dan host tanpa interaksi manual; host dapat menyetelnya `false` untuk membuat window SDL terlihat.

Presenter menolak renderer yang belum diinisialisasi atau ukurannya tidak sama. Penolakan tidak mengubah jumlah frame atau hash frame terakhir yang telah disajikan. `RendererCapabilityProbe` sekarang melaporkan `ReadyPresent` hanya dalam arti **backend presenter tersedia dan runtime dapat meminta opt-in**, bukan bahwa window sudah dibuka atau GPU renderer sudah tersedia.

> Jalur ini adalah presentasi **software CPU ke SDL surface**, bukan swapchain GPU, renderer Vulkan runtime, compositing window manager, atau bukti game desktop siap dipublikasikan.

## Bukti

`software_surface_presenter_smoke` membuktikan inisialisasi ukuran invalid ditolak, dua frame CPU dengan hash berbeda diunggah ke surface tersembunyi, renderer salah ukuran ditolak dengan frame sebelumnya tetap tercatat, dan `NeoRuntime` Farm dapat melakukan `Tick` serta `RenderFarm` ketika opt-in surface tersembunyi aktif. `renderer_capability_smoke` membuktikan capability opt-in yang baru.

Kedua smoke lulus pada Release dan ASAN dengan `detect_leaks=1`. Suite non-Vulkan penuh lulus **94/94 Release** dan **94/94 ASAN** setelah perubahan capability.

## Batas

Presenter belum memiliki event loop platform yang terus-menerus, resize, DPI scaling, fullscreen, vsync policy, present pacing, input window, screenshot UI, GPU upload, shader, alpha composition, 3D camera window interaction, atau recovery device/window lost. Tidak ada bukti manual window yang terlihat oleh end-user pada milestone ini; evidence automated memakai surface tersembunyi agar deterministik di sandbox. Fase A.2 tetap terbuka untuk graphical vertical slice yang benar-benar dapat diinspeksi.
