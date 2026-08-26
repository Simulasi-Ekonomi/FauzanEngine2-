# Editor Scene Session V1

`EditorSceneSession` adalah fondasi editor in-engine yang bounded. `Open` memakai validasi `EditorSceneDocumentAdapter`, lalu binders mesh/sprite kanonis untuk membangun candidate world/adapters sebelum sesi lama diganti. `Save` mengembalikan salinan dokumen yang terbuka; ia bukan writer filesystem.

`HierarchySnapshot` mengurutkan actor berdasarkan ID secara deterministik. `InspectActor` mengembalikan actor copy menurut ID. `RenderViewport` memakai `SceneRenderAdapter` sehingga mesh-first dan sprite staged diraster pada seam runtime yang sama. Open yang gagal mempertahankan sesi sebelumnya.

`UpdateTransform` mencari actor pada candidate salinan dokumen, mengganti `Transform3`, lalu menaikkan revision satu kali. Candidate itu dibuka ulang melalui jalur `Open` yang sama; karena itu invalid transform, missing asset, actor yang tidak ada, atau revision yang tidak dapat dinaikkan tidak mengubah sesi yang telah terbuka. Tidak ada write parsial ke `SceneWorld` atau adapters.

`ReparentActor` memakai pola candidate yang sama untuk mengganti `parentId`. Parent yang hilang atau cyclic hierarchy ditolak oleh loader kanonis dan sesi yang aktif, revision, hierarchy snapshot, serta viewport tidak diganti. Snapshot tetap diurutkan secara numerik; API ini tidak mendefinisikan visual tree ordering atau operasi undo/redo.

`AddActor` dan `DeleteActor` menyelesaikan mutation hierarchy minimal melalui candidate document yang sama. Tambah actor duplikat/invalid ditolak oleh loader; delete ditolak lebih awal jika actor masih memiliki anak. Commit yang sukses menaikkan revision tepat satu kali. Actor tidak memiliki name/display layer dan tidak ada mutation UI; caller tetap bertanggung jawab menyediakan actor ID dan asset references yang valid.

`editor_scene_session_smoke` membuktikan hierarchy, inspector, save snapshot, viewport render, edit transform/revision, reparent actor, add/delete actor, serta preservasi sesi setelah update/open dokumen gagal. Ia juga membuka satu dokumen terpadu dengan mesh OBJ/MTL staged dan sprite PPM staged, merender keduanya melalui satu `RenderViewport`, lalu membuktikan kegagalan sprite kedua mempertahankan hash viewport session yang telah dikomit. Target lulus Release dan ASAN `detect_leaks=1`; runner broad non-Vulkan canonical juga lulus.

Ini bukan desktop editor/UI, filesystem project manager, undo/redo, gizmo transform, collaboration, gameplay, APK, atau release evidence.
