# Render Camera Sphere Frustum V1

`RenderCamera::SphereIntersectsFrustum(center, radius)` menyediakan query sphere konservatif reusable pada camera space tervalidasi. Ia menangani orientasi forward/up, perspective maupun orthographic, enam batas frustum, dan radius non-negatif finite. Instance sepenuhnya di luar mengembalikan `false`; sphere yang menyentuh near plane dipertahankan secara konservatif.

`SceneMeshAdapter` sekarang memakai query ini sehingga tidak lagi menyimpan persamaan frustum duplikat. `render_camera_smoke` dan `scene_mesh_adapter_smoke` lulus Release serta ASAN `detect_leaks=1`; broad non-Vulkan lulus **96/96 Release** dan **96/96 ASAN**.

Ini bukan scene hierarchy, occlusion/LOD/streaming, GPU culling, renderer production, game playable, APK, atau release evidence.
