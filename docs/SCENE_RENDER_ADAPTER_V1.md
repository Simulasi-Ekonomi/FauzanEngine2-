# Scene Render Adapter V1

`SceneRenderAdapter` adalah entry point runtime terbatas yang mengomposisikan `SceneMeshAdapter` lalu `SceneSpriteAdapter` ke candidate `SoftwareRenderer`. Hanya setelah mesh draw, sprite queue, dan SpriteBatch flush seluruhnya berhasil, candidate menggantikan framebuffer caller.

Urutan kontrak adalah mesh terlebih dahulu lalu sprite. Adapter ini tidak melakukan scene-wide material sort atau transparency sort lintas tipe. Ia meneruskan validation/culling mesh dan staging snapshot sprite yang sudah ada, tanpa mengambil alih kepemilikan `SceneWorld`.

`scene_render_adapter_smoke` membuktikan scene berisi mesh dan sprite staged diraster pada kamera yang sama. Ketika sprite adapter memuat entity yang tidak ada, queue gagal dan hash framebuffer caller dipertahankan. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **98/98 Release** dan **98/98 ASAN**.

Ini bukan scene graph umum, editor, gameplay, animation system, culling/sorting production, GPU renderer, APK, atau release evidence.
