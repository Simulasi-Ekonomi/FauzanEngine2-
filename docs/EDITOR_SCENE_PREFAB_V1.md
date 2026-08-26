# Editor Scene Prefab V1

`EditorScenePrefab` adalah salinan subtree actor **in-memory** dari `EditorSceneDocument` yang sudah terbuka. `CapturePrefab` memilih satu actor root beserta seluruh keturunan yang mencapai root itu dalam hierarchy dokumen. Salinan mempertahankan kind, transform lokal, dan referensi asset; root prefab dinormalisasi menjadi `parentId = 0` di dalam prefab.

`InstantiatePrefab` memerlukan satu ID instance baru untuk setiap actor prefab. Semua ID harus tak-nol, unik, dan belum terdapat dalam dokumen target. Root instance diparentkan hanya kepada `parentActorId` caller; parent seluruh actor lain diremap hanya melalui hierarchy internal prefab. Candidate document dinaikkan satu revision lalu dibuka melalui jalur atomik `EditorSceneSession::CommitMutation`; kegagalan mapping, kapasitas, parent, asset, atau binder tidak mengubah dokumen, world, adapters, history, atau viewport sesi yang telah dikomit.

`editor_scene_prefab_smoke` membuktikan capture satu root dengan child, remap dua ID pada parent target, revision tunggal, preservasi penuh setelah duplicate mapping dan prefab malformed, serta undo/redo instansiasi. Target diuji pada Release dan ASAN `detect_leaks=1`, kemudian runner broad non-Vulkan canonical.

Ini bukan format `.prefab` serialisasi, nested prefab, asset registry lifetime, UI hierarchy/drag-drop, live runtime spawning, filesystem project asset, networking, atau production content pipeline.
