# Gameplay Physics Velocity Guard V1

`GameplayPhysicsBodyBuilder` menyediakan setter velocity planar tunggal dan batch yang hanya menulis komponen velocity ECS pada body dinamis. Sebelum menulis, kedua jalur memvalidasi input velocity, inverse mass, radius, dan velocity ECS yang sedang tersimpan. State velocity yang non-finite atau body malformed ditolak sebagai `InvalidBodyState`; batch memvalidasi seluruh kandidat terlebih dahulu sehingga body lain tidak ikut berubah dan physics revision tidak bertambah pada kegagalan.

`gameplay_physics_body_smoke` dan `gameplay_physics_query_smoke` lulus pada Release serta AddressSanitizer dengan `ASAN_OPTIONS=detect_leaks=1`. Evidence mencakup single-set rejection pada velocity `NaN`, batch rejection ketika kandidat pertama malformed, preservasi velocity body kedua, preservasi physics revision, dan regresi query setelah state dipulihkan.

Increment ini tidak menambahkan force integration, angular dynamics, transform authority, collision response, network prediction, persistence, atau determinism proof lintas thread count. Penulisan langsung ke ECS pada smoke untuk membentuk malformed input adalah adversarial test setup; caller produksi tetap harus menghormati boundary bahwa SceneWorld transform dan movement authority bukan milik API ini.

## Status scope

Guard ini memperkuat validasi state pada API yang sudah ada. Ia bukan rigidbody system lengkap dan tidak menutup parent P1.2, yang masih memerlukan contract trigger/collider yang lebih luas, integrasi solver gameplay, serta evidence runtime yang lebih komprehensif.

