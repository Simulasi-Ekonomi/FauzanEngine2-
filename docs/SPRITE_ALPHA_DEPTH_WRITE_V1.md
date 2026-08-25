# Sprite Alpha Depth Write V1

`SpriteDraw::depthWrite` secara eksplisit mengendalikan apakah sprite mengubah depth buffer setelah lulus depth test. Nilai default `true` mempertahankan kompatibilitas renderer sebelumnya. Untuk sprite alpha seperti billboard, `false` mempertahankan warna source-over tetapi tidak menghalangi gambar yang lebih jauh pada perintah berikutnya.

Kebijakan diteruskan ke jalur flat dan textured `SoftwareRenderer`. `sprite_batch_smoke` membuktikan sprite biru alpha di depan dengan `depthWrite=false` tidak memblokir sprite merah opaque lebih jauh; pada `true`, sprite merah itu ditolak oleh depth seperti perilaku lama. Target lulus Release dan ASAN `detect_leaks=1`; broad non-Vulkan lulus **97/97 Release** dan **97/97 ASAN**.

Ini bukan general transparency sorting, order-independent transparency, alpha-test/cutout material, GPU blending, gameplay, APK, atau release evidence.
