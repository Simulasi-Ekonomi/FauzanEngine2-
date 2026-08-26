# Prefab Staging V1

`PrefabStagingStore` menerima hanya asset `AssetKind::Prefab` yang ada, `Ready`, dan memiliki byte registry. Byte itu didecode dengan `EditorScenePrefabCodec` ke satu snapshot `CpuPrefabResource` yang menyimpan ID asset, content hash, dan subtree actor. Staging dibatasi hingga 64 prefab dan total 512 actor.

`IsCurrent` membandingkan hash resource dengan asset registry siap saat ini. `InstantiateStagedPrefab` pada `EditorSceneSession` menolak resource yang tidak ada atau stale sebelum membentuk candidate dokumen; instance yang berhasil tetap melewati validasi document dan binder sesi. `Stage` atau `Refresh` yang gagal tidak mengubah resource/staged actor yang sudah ada.

`prefab_staging_smoke` membuktikan ready staging, preservasi setelah asset prefab rusak kedua gagal decode, instansiasi staged ke session, stale-hash rejection dengan dokumen session tetap, dan refresh setelah byte valid dipulihkan. Target diuji pada Release dan ASAN `detect_leaks=1`, kemudian runner broad non-Vulkan canonical.

Ini bukan file watcher, filesystem import, nested prefab, asset streaming, GPU resource, live runtime spawning, UI editor, atau production content pipeline.
