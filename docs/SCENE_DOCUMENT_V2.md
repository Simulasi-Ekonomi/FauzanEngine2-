# SceneDocument v2 — Asset Binding Authoring Contract

## Perubahan schema

SceneDocument v2 menambahkan `material_asset_id`, `material_name`, dan `texture_asset_id` optional pada actor. SceneDocument v1 tetap dapat dibaca dan diserialisasi sebagai envelope `NAB1`; v2 memakai `NAB2`. Backend menyimpan kedua versi dengan checksum/revision yang sama dan menolak binding material pada dokumen v1.

| Field | Validasi | Runtime effect |
|---|---|---|
| `asset_id` | Referensi printable ASCII; runtime membutuhkan Mesh/Prefab `Ready` bila dipakai. | Actor mesh dapat dibentuk pada `SceneWorld`. |
| `material_asset_id` + `material_name` | Nama material membutuhkan asset material; keduanya printable ASCII dan bounded. | `EditorSceneMeshBinder` men-stage MTL dan menjaga identity material. |
| `texture_asset_id` | Texture `Ready`, hanya PPM P6 atau BMP BI_RGB pada binder v2 saat ini. | Texture di-stage dan disalin ke instance render dengan identity/hash sumber. |

## End-to-end bounded path

Editor serializer mengirim SceneDocument v2 ke backend versioned. Backend memvalidasi, menerima revision optimistic, membuat receipt checksum, dan serializer local-only hanya menghasilkan `NAB2` ketika approval eksplisit diberikan. C++ bridge memeriksa envelope/version/bounds sebelum SceneDocument adapter memuat kandidat atomik. `EditorSceneMeshBinder::BindDocumentAssets` kemudian membuat candidate mesh adapter dari referensi authoring v2; scene target dan adapter lama dipertahankan bila asset, material, texture, atau staging ditolak.

## Evidence

Backend unittest: 9/9 lulus, termasuk compatibility `NAB1` dan serializer `NAB2`. Smoke C++ membuktikan parsing `NAB2`, material/texture ready checks, texture stage PPM, render texture, duplicate/missing binding rejection, serta preservation kandidat. Regressi broad non-Vulkan setelah perubahan: **88/88 Release** dan **88/88 ASAN** dengan `detect_leaks=1`.

## Batas yang tidak boleh disembunyikan

Editor TypeScript production build belum direvalidasi pada sandbox ini karena policy package manager memblokir build script `esbuild`; ini merupakan gate environment yang terbuka. Document v2 belum berarti model asset lengkap: importer sekarang terbatas pada OBJ, MTL, PPM P6, dan BMP BI_RGB; tidak ada glTF, PNG/JPEG, audio, sprite atlas, skeletal asset import, atau Android player proof pada contract ini.
