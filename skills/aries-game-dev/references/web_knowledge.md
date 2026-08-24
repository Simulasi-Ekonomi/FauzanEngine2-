# Aries Learned Knowledge (April 2026 Update)

## Unreal Engine Pattern: Actor Component
Aries sekarang paham bahwa logika Game harus dipisah menjadi komponen.
- **InputComponent**: Menangani interaksi.
- **MeshComponent**: Menangani visual.
- **EconomicComponent**: (Spesifik FauzanEngine) Menangani data simulasi.

## High-Performance C++ (Internet Standard)
- Gunakan `std::vector::reserve()` sebelum mengisi data besar untuk mencegah re-alokasi memori di tengah simulasi.
- Gunakan `SIMD` (Single Instruction, Multiple Data) jika memungkinkan untuk kalkulasi ekonomi massal.
