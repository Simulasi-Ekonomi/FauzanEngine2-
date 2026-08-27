# Vulkan Xvfb Present Evidence V1

`vulkan_present_smoke` membuat window SDL tersembunyi, Vulkan surface, device, swapchain, satu submit frame, dan satu present. Dengan `SDL_VIDEODRIVER=dummy`, surface path ditolak sehingga smoke tidak dapat membuktikan present. Runner `tools/vulkan_present_xvfb_smoke.sh` menjalankan executable yang telah dibangun pada Xvfb, yaitu display X11 virtual terisolasi.

Perintah yang dibuktikan untuk tiap konfigurasi adalah sebagai berikut.

```bash
cmake -S Source/NeoEngine -B <build> -G Ninja -DCMAKE_BUILD_TYPE=<Release|Debug> [sanitizer flags]
ninja -C <build> -j2 vulkan_present_smoke
tools/vulkan_present_xvfb_smoke.sh <build>
```

Runner fail-closed bila `xvfb-run` atau executable target tidak tersedia. Keberhasilan hanya menyatakan lifecycle hidden SDL/X11 surface dan Vulkan device/swapchain/submit/present berjalan pada backend virtual yang tersedia. `VulkanRHI` tetap memiliki boundary `NOT_IMPLEMENTED`; tidak ada klaim render loop game, integrasi NeoRuntime, device fisik, GPU performance, device-loss recovery, window interaktif, atau renderer produksi.
