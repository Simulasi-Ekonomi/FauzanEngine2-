# NeoRuntime Farm Progress Checkpoint V1

`NeoRuntime::SaveFarmProgressCheckpoint` menyimpan satu envelope `RuntimeSaveCodec` bertipe `neo-farm-progress`. Payload memiliki tiga blob panjang-terbatas: serialisasi `FarmWorldTool` (yang sudah berisi satu serialisasi Farm), `RuntimeTimeSystem`, dan ledger `FarmAuthoritativeService`. Revisi harus bukan nol; checkpoint hanya dapat diambil pada batas quiescent ketika event bus tidak memiliki event tertunda.

Pemulihan selalu mendekode envelope, semua blob, Farm, FarmWorld, waktu, dan ledger otoritas ke kandidat terlebih dahulu. Kandidat lalu hanya dapat mewarisi binding SceneWorld aktif jika konfigurasi world, jumlah, urutan, dan ID building/NPC sama persis serta semua entity binding aktif masih valid. Session otoritas runtime dibind ulang secara eksplisit karena kredensial sesi tidak pernah terserialisasi.

Jika format, checksum, jenis envelope, blob, state kandidat, ledger, atau topology scene tidak valid, NeoRuntime menolak tanpa mengganti Farm, FarmWorld, waktu, otoritas, SceneWorld, atau receipt aktif. Pada commit berhasil, transform entity topology-preserving disinkronkan dari kandidat dan receipt frame/render lama dibersihkan karena keduanya bukan bagian dari checkpoint progress.

Kontrak ini bukan rollback dunia umum dan tidak merekonsiliasi perubahan topology building atau NPC. Ia tidak menyimpan actor, timer, input, event tertunda, asset, renderer, surface, scene authoring, route motion, atau state clock-frame. Karena itu API hanya merupakan checkpoint progres Farm lokal pada batas quiescent, bukan persistence runtime/scene penuh, snapshot multiplayer, atau bukti kesiapan produksi.
