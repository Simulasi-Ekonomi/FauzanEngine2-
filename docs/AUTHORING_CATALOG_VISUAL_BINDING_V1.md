# AuthoringCatalog Visual Binding v1

`AuthoringCatalogVisualBinder` menghubungkan game-authoring yang telah ada—building dan actor Character/NPC/Monster—ke asset visual runtime tanpa mengubah ownership `SceneWorld`. `AuthoringCatalog` tetap membuat, memelihara, men-tick, dan memiliki entity placement; binder hanya mencari entity yang telah di-bind berdasarkan pasangan `(AuthoringSceneObjectKind, definitionId)` dan membangun `SceneMeshAdapter` kandidat.

## Kontrak

Setiap visual binding menyatakan kind placement, definition ID, mesh asset, material asset, material name, dan texture optional. Mesh/material/texture harus `Ready` pada AssetRegistry; resource di-stage atau di-refresh sebelum candidate render adapter dipublikasikan. PPM P6 dan BMP BI_RGB adalah texture format yang didukung pada contract ini. Binding ganda, entity tidak ada, resource tidak valid, atau decode/staging gagal tidak mengganti adapter target yang sudah valid.

## Bukti

Smoke mencakup skeleton, character, building, NPC patrol, scene binding, mesh/material/texture staging, render texture, duplikasi yang ditolak, dan perubahan transform actor setelah tick. Broad non-Vulkan evidence setelah penambahan: **89/89 Release** dan **89/89 ASAN** dengan `detect_leaks=1`.

## Batas

Contract ini belum membuat player controller, combat, inventory runtime, skeletal mesh GPU rendering, sprite/tile world, networking, save-game player, atau presentation window. Ia membuktikan satu seam: data building/NPC yang telah di-author dapat menjadi entity scene yang memiliki asset visual staged dan mengikuti lifecycle transform canonical.
