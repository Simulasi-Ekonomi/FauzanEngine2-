# SceneDocument Mesh Binding v1

## Batas kemampuan

`EditorSceneMeshBinder` menghubungkan bagian yang sebelumnya terpisah: `EditorSceneDocument` yang telah dimuat atomik ke `SceneWorld`, asset `Mesh`/`Material` yang `Ready` pada `AssetRegistry`, resource CPU dari `MeshStagingStore`/`MaterialStagingStore`, dan instance render `SceneMeshAdapter`.

Binder ini hanya menerima mapping eksplisit `actorId → materialAssetId + materialName` untuk actor `Mesh` pada dokumen yang telah divalidasi. Ia bukan importer filesystem, endpoint jaringan, tool agent, atau otoritas game runtime.

## Kontrak atomik

1. Dokumen harus versi v1 dan seluruh actor mesh wajib memiliki mapping material tepat satu kali.
2. Mesh dan material harus ada, bertipe benar, dan berstatus `Ready` dalam `AssetRegistry`.
3. Resource yang belum staged akan di-stage dari bytes registry; resource stale akan di-refresh secara eksplisit.
4. Kandidat `SceneMeshAdapter` dibangun seluruhnya sebelum mengganti target. Mapping ganda, material hilang, asset salah, atau entity yang tidak terikat mempertahankan adapter target sebelumnya.
5. Binder hanya memakai `EditorSceneDocumentAdapter::EntityForActor`; ia tidak menciptakan entity di luar dokumen atau memodifikasi `SceneWorld`.

## Bukti

`editor_scene_mesh_binder_smoke` membuktikan staging OBJ/MTL dari registry, binding actor mesh ke entity scene, render non-background yang deterministik, penolakan mapping kosong/ganda/material hilang, dan preservation terhadap adapter yang telah valid. Setelah penambahan binder, broad suite non-Vulkan adalah **88/88 Release** serta **88/88 ASAN** dengan `detect_leaks=1`.

## Gap yang tersisa

SceneDocument v1 belum membawa material/texture binding sebagai bagian schema authoring/editor/backend. Mapping material masih diberikan caller secara eksplisit. Versi schema berikutnya harus mendefinisikan binding mesh/material/texture secara versioned dengan migration, backend validation, editor serialization, receipt, dan refresh/rebind semantics sebelum dapat disebut pipeline asset game lengkap.
