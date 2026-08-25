# Scene Mesh Oriented Culling V1

`SceneMeshAdapter` kini menilai bounding sphere setiap instance dalam **camera space** lewat `RenderCamera::WorldToCamera`, bukan lagi dengan asumsi world `+Z`. Near/far serta bidang sisi memakai basis kamera berorientasi yang sama seperti proyeksi mesh.

`scene_mesh_adapter_smoke` membuktikan kamera menghadap `+X` merender instance berotasi pada `x=5` dan mencull instance di belakang kamera pada `x=-5`, sambil mempertahankan staged mesh/material/texture, transform validation, dan draw atomik. Target lulus Release serta ASAN `detect_leaks=1`; suite non-Vulkan lulus **96/96 Release** dan **96/96 ASAN**.

Ini adalah culling sphere lokal konservatif per instance, bukan scene hierarchy, occlusion culling, LOD, streaming, GPU culling, renderer production, game, APK, atau release evidence.
