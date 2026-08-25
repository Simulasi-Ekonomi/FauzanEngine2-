# Editor Scene Session V1

`EditorSceneSession` adalah fondasi editor in-engine yang bounded. `Open` memakai validasi `EditorSceneDocumentAdapter`, lalu binders mesh/sprite kanonis untuk membangun candidate world/adapters sebelum sesi lama diganti. `Save` mengembalikan salinan dokumen yang terbuka; ia bukan writer filesystem.

`HierarchySnapshot` mengurutkan actor berdasarkan ID secara deterministik. `InspectActor` mengembalikan actor copy menurut ID. `RenderViewport` memakai `SceneRenderAdapter` sehingga mesh-first dan sprite staged diraster pada seam runtime yang sama. Open yang gagal mempertahankan sesi sebelumnya.

`UpdateTransform` mencari actor pada candidate salinan dokumen, mengganti `Transform3`, lalu menaikkan revision satu kali. Candidate itu dibuka ulang melalui jalur `Open` yang sama; karena itu invalid transform, missing asset, actor yang tidak ada, atau revision yang tidak dapat dinaikkan tidak mengubah sesi yang telah terbuka. Tidak ada write parsial ke `SceneWorld` atau adapters.

`editor_scene_session_smoke` membuktikan hierarchy, inspector, save snapshot, viewport render, edit transform dan revision yang sukses, serta preservasi sesi setelah update/open dokumen gagal. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **99/99 Release** dan **99/99 ASAN**.

Ini bukan desktop editor/UI, filesystem project manager, undo/redo, gizmo transform, collaboration, gameplay, APK, atau release evidence.
