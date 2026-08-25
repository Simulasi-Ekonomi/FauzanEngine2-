# Asset Import Surface Demo V1

`RunAssetImportSurfaceDemo` adalah vertical slice finite yang membuktikan alur asset in-memory menuju frame terlihat. Demo mengimpor texture PPM dan mesh OBJ lewat `TextureImportPipeline` serta `MeshImportPipeline`, mengambil resource CPU staged, lalu merendernya dengan `MeshRenderer` ke `SoftwareRenderer`. Frame diunggah melalui `SoftwareSurfacePresenter` dan frame akhir ditulis sebagai artifact PPM eksplisit.

Konfigurasi dibatasi pada framebuffer 32–1024 piksel per sisi, 1–600 frame, dan path artifact non-kosong maksimal 256 karakter. Demo menjalankan jumlah frame yang terbatas, memompa event surface, dan gagal eksplisit bila surface meminta close atau present gagal. Ia tidak memasang host loop persisten, input gameplay, save file, network, atau authority game.

`asset_import_surface_demo_smoke` membuktikan penolakan konfigurasi invalid, empat frame render/present tersembunyi, pixel non-background, hash mesh/texture/frame non-nol, dan artifact PPM ber-header `P6`. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **106/106 Release** dan **106/106 ASAN**.

Ini adalah proof finite asset-to-render, bukan game playable, editor visual, host desktop persisten, renderer GPU, package APK, atau release evidence.
