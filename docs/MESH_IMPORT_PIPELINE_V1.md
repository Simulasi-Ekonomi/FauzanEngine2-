# Mesh Import Pipeline V1

`MeshImportPipeline` adalah transaksi import OBJ in-memory di atas importer kanonis. Ia membuat candidate `AssetRegistry`, mengimpor asset sebagai `Mesh`, menandainya ready, lalu memanggil candidate `MeshStagingStore::StageObj`. Registry, staging store, dan receipt caller hanya berubah apabila seluruh jalur berhasil.

Import menolak ID/bytes kosong. Duplicate/dependency/capacity registry failure, ready-state failure, maupun kegagalan importer OBJ membatalkan transaksi tanpa mengubah registry, mesh staging, atau receipt. `MeshStagingOptions` tetap caller-selected; default-nya tidak mengenerate normal datar.

`mesh_import_pipeline_smoke` membuktikan OBJ quad ber-normal menjadi mesh registry-ready/staged dengan empat vertex dan enam index, serta rollback pada OBJ rusak, duplicate ID, dan request invalid. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **105/105 Release** dan **105/105 ASAN**.

Ini belum filesystem import, MTL/material import, scene/prefab transaction, generic mesh format, hot reload hidup, cache disk, GPU upload, atau release evidence.
