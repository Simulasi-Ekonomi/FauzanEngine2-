# Software Surface Lifecycle v1 — Event Pump and Close Request

## Ruang Lingkup

`SoftwareSurfacePresenter` kini memiliki lifecycle surface minimal di atas jalur CPU → SDL yang telah ada. `PumpEvents` menguras event SDL, mencatat `SDL_QUIT` atau `SDL_WINDOWEVENT_CLOSE` sebagai close request, dan `Present` menolak frame berikutnya dengan `CloseRequested`. `NeoRuntime::RenderFarm` memanggil `PumpEvents` sebelum `Present`, sehingga runtime yang meminta presentation tidak mengabaikan close surface secara diam-diam.

| Operasi | Kontrak |
|---|---|
| `Initialize` | Membuat SDL resources dan mereset close request. |
| `PumpEvents` | Wajib surface siap; mencatat quit/window-close tanpa memutasi renderer atau gameplay. |
| `Present` | Menolak dengan `CloseRequested` jika surface sudah meminta close; counter/hash tidak berubah. |
| `Reset` | Menghancurkan resource SDL dan membersihkan close request. |

## Failure behavior

`PumpEvents` ketika surface belum siap gagal `NotInitialized`. Setelah close request, `Present` gagal eksplisit; `NeoRuntime::RenderFarm` menerjemahkan penolakan itu menjadi `RuntimeError::PresentationFailed`. Tidak ada fallback headless setelah presentation telah diminta, dan tidak ada event yang diterjemahkan langsung menjadi input/gameplay command.

> Ini adalah lifecycle surface terbatas. Ia tidak menambah window input mapping, continuous game loop, resize, DPI scaling, fullscreen, vsync/present pacing, device recovery, ataupun authority gameplay.

## Bukti

`software_surface_presenter_smoke` membuat hidden surface, mempresentasikan dua frame, menyuntikkan `SDL_QUIT`, membuktikan `PumpEvents` melihat close request, dan memastikan present berikutnya ditolak tanpa mengubah count/hash. Smoke juga membuktikan reset kembali fail-closed dan hook NeoRuntime hidden surface masih dapat tick/render/shutdown. Smoke lulus pada Release dan ASAN `detect_leaks=1`; suite non-Vulkan penuh mencapai **95/95 Release** dan **95/95 ASAN**.
