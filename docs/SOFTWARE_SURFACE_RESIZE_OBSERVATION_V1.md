# Software Surface Resize Observation V1

`SoftwareSurfacePresenter` kini mencatat ukuran terakhir dari event SDL `RESIZED` atau `SIZE_CHANGED` melalui `WindowWidth()` dan `WindowHeight()`. Observasi ini tidak mengubah ukuran texture streaming, `SoftwareRenderer`, atau kontrak dimensi `Present`; mismatch source tetap gagal `DimensionMismatch`.

> Ini adalah observasi lifecycle fail-closed, bukan implementasi resize framebuffer atau host desktop lengkap.

`software_surface_presenter_smoke` menyuntikkan event ukuran `24×20`, membuktikan ukuran tercatat, presentasi source `16×16` tetap dapat berjalan melalui texture lama, close request tetap memblokir present, dan `Reset` mengosongkan ukuran. Target lulus Release dan ASAN `detect_leaks=1`; suite non-Vulkan lulus **96/96 Release** dan **96/96 ASAN**.

Tidak ada resize renderer/framebuffer, DPI, fullscreen, vsync/pacing, input host, event loop persisten, desktop application, game playable, APK, atau evidence rilis.
