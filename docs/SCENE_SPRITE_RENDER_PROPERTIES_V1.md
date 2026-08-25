# Scene Sprite Render Properties V1

`SceneSpriteAdapter::AddStaged` kini menerima `rotationRadians`, `faceCamera`, dan `depthWrite` sebagai parameter trailing dengan default kompatibel. Saat queue, rotasi lokal dijumlahkan dengan `Transform3::rz`; billboard dan depth-write diteruskan ke `SpriteDraw` tanpa mengganti kepemilikan texture snapshot atau `SceneWorld`.

`scene_render_adapter_smoke` membuktikan sprite staged pada entity `x=5` dapat memakai billboard terhadap kamera menghadap `+X`, dengan rotasi lokal finite dan `depthWrite=false`. Target lulus Release serta ASAN `detect_leaks=1`; broad non-Vulkan lulus **98/98 Release** dan **98/98 ASAN**.

Ini bukan material scene umum, transform 3D sprite penuh, animation, GPU batching, scene sorting production, gameplay, APK, atau release evidence.
