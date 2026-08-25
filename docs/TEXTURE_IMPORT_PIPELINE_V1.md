# Texture Import Pipeline V1

`TextureImportPipeline` adalah transaksi import texture in-memory. Ia menerima bytes PPM P6 atau BMP BI_RGB, membuat candidate `AssetRegistry`, memasukkan asset texture, menandainya ready, lalu mendecode melalui candidate `TextureStagingStore`. Hanya setelah registry dan staging berhasil, kedua store serta receipt caller diganti bersama.

Import dengan ID kosong/bytes kosong, registry duplicate/capacity/dependency failure, ready-state failure, atau decoder failure tidak mengganti registry, staging store, maupun receipt caller. Coordinator tidak membaca filesystem, tidak memasang file watcher, tidak mem-refresh live renderer adapter, dan tidak mengunggah resource GPU.

`texture_import_pipeline_smoke` membuktikan import PPM menuju registry-ready dan staged CPU texture, serta rollback pada PPM rusak, duplicate ID, dan request invalid. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **104/104 Release** dan **104/104 ASAN**.

Ini belum pipeline asset produksi: tidak ada filesystem import, format general, MTL/material transaction, prefab/scene transaction, hot reload hidup, cache disk, GPU lifetime, atau release evidence.
