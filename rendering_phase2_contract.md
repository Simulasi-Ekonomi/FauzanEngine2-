# FauzanEngine Rendering Phase 2 Contract

## Status dan batas

Fase ini memperkuat **bukti presentation engine**, bukan membuat aplikasi, website, APK, AAB, atau game siap rilis. Software renderer saat ini sudah dapat raster triangle berwarna serta Farm tile/marker headless; Vulkan telah membuktikan instance/device, offscreen draw, texture sampling, dan hidden-surface present. Tidak ada mesh importer, skeletal skinning, gameplay camera, UI, terrain renderer, atau mobile presentation yang siap dimainkan.

Implementasi kanonis sekarang memiliki `RenderCamera` ortografik/perspektif, `SpriteBatch` berkapasitas 2.048 dengan sort stabil, `MeshRenderer` CPU berkapasitas 2.048 vertex/6.144 indeks dengan depth, ambient/directional light, serta nearest sampling hanya dari texture staging ber-hash. Staging menerima P6 PPM serta BMP BI_RGB 24/32-bit yang dibatasi sebelum menghasilkan RGBA CPU. `BitmapTextRenderer` memiliki glyph 5×7 yang dibatasi dengan clip-depth tervalidasi. `UiCanvasRenderer` dapat mengkomposisikan panel widget router dan maksimum satu label bitmap per widget pada depth panel sehingga z-order parent/child tetap benar pada raster CPU. `FarmRenderAdapter::RenderWorld` kini menyalurkan seluruh tile terrain serta building/NPC/character ke jalur camera/sprite yang sama. Semua primitive ini memiliki smoke Release dan AddressSanitizer, tetapi tetap hanya proof CPU deterministik: tidak ada mesh importer, skeletal skinning, scene-to-GPU bridge, player UI lengkap, terrain streaming, atau presentasi mobile siap dimainkan.

`SceneMeshAdapter` menambahkan bridge terbatas dari transform world `SceneWorld` ke `MeshRenderer` CPU. Bridge ini dibatasi menjadi 64 instance, menolak entity hilang serta skala non-uniform, meneruskan rotasi `rx/ry/rz` sebagai Euler radians ke vertex dan normal mesh, dan memakai local-origin bounding sphere untuk culling frustum konservatif pada kamera axis-aligned. `MeshMaterial` juga dapat meminta front-winding back-face culling di clip space sebelum raster. Untuk kamera perspektif, `MeshRenderer` sekarang melakukan Sutherland–Hodgman clipping hanya pada near plane dengan epsilon positif, menginterpolasi posisi/normal/UV, dan mentriangulasi hasil maksimal quad. Side/far-plane clipping, clipping orthografik, dan clipping GPU belum ada. `SceneWorld` sekarang menerapkan uniform scale dan rotasi Euler XYZ parent ke offset posisi child sebelum translasi, tetapi channel rotasi world sendiri masih penjumlahan Euler—bukan komposisi quaternion/matrix. Ini membuktikan satu vertical slice 3D berbasis scene transform, bukan scene graph GPU, asset-instance renderer, animator, importer, atau client 3D interaktif.

## Urutan implementasi yang dapat diuji

| Blok | Kontrak bounded | Evidence minimum |
|---|---|---|
| Camera | Ortografik 2D dan perspektif 3D memproyeksikan koordinat world ke clip space secara deterministic dan menolak konfigurasi invalid. | Release + AddressSanitizer smoke memeriksa center, edge, near/far, invalid FOV/extent. |
| Sprite 2D | Sprite quad memiliki layer/order, transform, warna/textured region, dan sort stable sebelum raster. | Smoke memeriksa ordering layer serta pixel non-background/hash. |
| Mesh 3D | Mesh bounded dengan vertex/index, model/view/projection, dan material warna dasar. | Offscreen smoke memeriksa draw triangle/quad perspektif serta invalid mesh rejection. |
| Material/texture/light | Texture content-hash/staging digunakan dengan material paling kecil; ambient dan directional light memodulasi warna. | Hash/pixel test untuk material dan dua lighting condition. |
| Text | Glyph atlas/bitmap bounded dengan string limit dan penolakan character tidak didukung. | Smoke memeriksa glyph draw non-background dan overflow. |
| Farm graphical slice | Farm world dirender melalui camera/sprite/scene contract, bukan marker NDC hardcoded saja. | Frame capture/hash dalam executable engine; tetap bukan player UI atau mobile client. |

## Arsitektur backend-first

Fase awal memakai CPU/software path agar algoritma camera, sorting, primitive, dan material dapat diuji deterministik. API yang sama kemudian dapat dipetakan ke Vulkan. Bukti software tidak boleh dipakai untuk mengklaim GPU performance, Android compatibility, atau renderer shipping. Vulkan proof yang ada juga tidak boleh dihitung sebagai material/mesh/camera pipeline lengkap sampai bukti spesifiknya dibangun.

## Batas keamanan dan produk

Tidak ada asset dari prompt/dokumen yang langsung dimuat ke GPU atau dieksekusi. Importer/decoder baru harus memiliki content limit, hash, dan fail-closed validation. AI prompt runner dapat meminta audit/build/test evidence, tetapi tidak bisa memperkenalkan shader, source renderer, asset, atau deploy melalui operasi otomatis.
