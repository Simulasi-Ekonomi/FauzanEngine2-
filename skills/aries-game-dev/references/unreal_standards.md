# Unreal Engine & C++ Reference

## 1. Actor Lifecycle
- `PostInitProperties()`: Setup data awal.
- `BeginPlay()`: Inisialisasi saat game mulai.
- `Tick(float DeltaTime)`: Update setiap frame (gunakan dengan hati-hati).

## 2. Rendering & Sprites
- Batasi jumlah tekstur unik dalam satu scene.
- Gunakan Mipmaps untuk aset yang terlihat dari jauh.

## 3. C++ Best Practices
- Selalu gunakan `const` untuk fungsi yang tidak mengubah state.
- Gunakan `enum class` untuk tipe game agar *type-safe*.
