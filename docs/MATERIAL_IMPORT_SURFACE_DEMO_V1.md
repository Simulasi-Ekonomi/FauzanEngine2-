# Material Import Surface Demo v1

## Tujuan

`MaterialImportSurfaceDemo` adalah proof finite yang menyambungkan tiga pipeline kandidat in-memory: PPM texture, OBJ mesh, dan named MTL material. Ketiganya memasuki `AssetRegistry` dan CPU staging canonical, lalu resource staged diregistrasikan sekali pada `SceneMeshAdapter` melalui overload mesh-material-texture existing. Adapter membaca `SceneWorld` dan menghasilkan frame software yang diunggah ke SDL hidden surface serta artefak PPM.

> Demo ini membuktikan satu lintasan bounded CPU/software. Ia tidak menyatakan bahwa importer filesystem, hot reload, renderer GPU, material runtime production, atau desktop game host telah tersedia.

## Lintasan yang dibuktikan

| Tahap | Resource | Kontrak |
|---|---|---|
| Import texture | PPM 2×2 | `TextureImportPipeline` → ready registry texture → `TextureStagingStore`. |
| Import mesh | OBJ quad ber-UV/normal | `MeshImportPipeline` → ready registry mesh → `MeshStagingStore`. |
| Import material | MTL `farm`, diffuse `0.4/0.8/0.6` | `MaterialImportPipeline` → ready registry material → `MaterialStagingStore`. |
| Bind scene | Mesh + material + texture staged | `SceneMeshAdapter::AddStaged` copy-on-register yang existing; tidak ada live asset reference. |
| Render finite | SceneWorld → adapter → CPU mesh renderer | Empat frame melalui software renderer, SDL surface hidden, lalu PPM artifact. |

SceneWorld di demo hanya menyediakan satu transform identity untuk entity mesh. Tidak ada animation, gameplay movement, physics, authority, live material refresh, atau transform writer tambahan.

## Evidence executable

`material_import_surface_demo_smoke` menolak konfigurasi frame nol, lalu memverifikasi empat frame rendered/presented, 992 pixel non-background, hash frame deterministic `1642313260225753203`, hash texture/mesh/material nonzero, RGBA MTL `FF66CC99`, dan header PPM `P6`. Target berjalan pada Release dan AddressSanitizer dengan deteksi kebocoran aktif.

| Gate | Hasil final |
|---|---|
| `material_import_surface_demo_smoke` Release | Lulus; 4 frame, 4 present, 992 pixel, PPM artifact. |
| `material_import_surface_demo_smoke` AddressSanitizer | Lulus dengan `ASAN_OPTIONS=detect_leaks=1`. |
| Broad non-Vulkan Release | 119/119 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 119/119 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Demo tidak menyediakan import file, `mtllib`, material graph/PBR, texture maps, cache disk, hot reload, GPU upload/lifetime, shader binding, interactive host, scene editor UI, animation/skeletal rendering, multiplayer, APK/AAB, ataupun release readiness.
