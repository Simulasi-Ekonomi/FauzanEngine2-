# Hybrid Surface Demo V1

`HybridSurfaceDemo` adalah vertical slice finite yang merender satu mesh 3D bertekstur dan satu sprite billboard opt-in ke framebuffer software yang sama, lalu menjalankan `PumpEvents → CloseRequested check → Present` pada `SoftwareSurfacePresenter` dan menulis PPM eksplisit.

Konfigurasi dibatasi pada `32..1024` pixel dan `1..600` frame. Receipt mencatat frame render/present, pixel non-latar, serta hash akhir. Smoke menolak frame nol, memvalidasi tiga frame hidden, receipt, dan magic PPM `P6`. Executable menghasilkan artefak Release pada `/home/ubuntu/Downloads/fauzanengine_hybrid_surface_phase_a.ppm`.

Target hybrid lulus Release serta ASAN `detect_leaks=1`. Dengan smoke baru ini, suite non-Vulkan lulus **97/97 Release** dan **97/97 ASAN**. `vulkan_*` dan `sdl_audio_bridge_smoke` tetap dikecualikan sesuai konvensi evidence.

Ini bukan gameplay, input/persistent desktop host, editor, scene system umum, animation, GPU renderer, APK, atau release evidence.
