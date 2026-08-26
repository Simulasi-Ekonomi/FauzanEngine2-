# Editor Scene Prefab Codec V1

`EditorScenePrefabCodec` memberikan framing in-memory untuk satu `EditorScenePrefab`. Header `FZPF` V1 menyimpan `rootSourceId`, lalu memuat payload `EditorSceneDocumentCodec` yang memakai scene identity internal `editor-prefab`, revision `1`, dan tepat seluruh actor prefab. Framing tambahan itu mengikat root ke payload sehingga actor yang bukan keturunan root, root dengan parent eksternal, actor hierarchy invalid, trailing data, atau payload codec invalid ditolak.

Encode dan decode membuat kandidat lebih dahulu; `bytes` atau prefab output caller hanya diganti setelah seluruh framing, codec, dan validasi subtree berhasil. Codec tidak meng-embed byte asset dan tidak melakukan I/O filesystem. Asset readiness dan binding tetap divalidasi ketika prefab diinstansiasikan oleh `EditorSceneSession`.

`editor_scene_prefab_codec_smoke` membuktikan round trip root/parent actor, preservasi output pada framing invalid dan trailing byte, penolakan prefab malformed saat encode, serta instansiasi hasil decode dalam session dengan remap ID. Target diuji di Release dan ASAN `detect_leaks=1`, kemudian runner broad non-Vulkan canonical.

Ini bukan format file `.prefab` untuk proyek, nested prefab, asset packaging/streaming, UI editor, live runtime spawning, multiplayer replication, atau production content pipeline.
