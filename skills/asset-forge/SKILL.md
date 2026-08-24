---
name: high-fidelity-3d-forge
description: AAA-Grade 3D Pipeline integration using TRELLIS (SLAT) technology and Unreal Engine 5 Nanite/Lumen standards for FauzanEngine.
---

# High-Fidelity 3D Forge (TRELLIS x UE5 Standard)

## 🏗️ 1. TRELLIS Core (Large Structured Latents)
- **Technology**: Menggunakan Structured Latent Adversarial Training (SLAT) untuk hasil mesh yang tajam (sharp edges) dan manifold (siap cetak/render).
- **Material**: Otomatis menghasilkan PBR Maps (BaseColor, Roughness, Metallic, Normal) sekelas TRELLIS-2.
- **Topology**: High-poly generation yang dioptimasi untuk proses Decimation/LOD.

## 🎮 2. Unreal Engine 5 Integration Standards
- **Nanite Support**: Mesh hasil generate diatur agar kompatibel dengan virtualized geometry (High vertex density tanpa lag).
- **Lumen-Ready**: Pengaturan UV Lightmap otomatis untuk sistem Global Illumination.
- **Auto-Rigging**: Skema penulangan (skeleton) mengikuti standar UE5 Mannequin (SK_Mannequin) untuk retargeting animasi instan.

## 🖼️ 3. Advanced Sprite-Gen (Ray-Traced)
- **Multiview Capture**: Mengambil 36 frame (10° increment) untuk rotasi sprite 360° yang smooth.
- **Normal Map Sprites**: Menghasilkan Normal Map untuk sprite 2D agar bisa merespons pencahayaan dinamis (Dynamic Lighting) di FauzanEngine.
- **DPI Quality**: Export 4K texture atlas dengan kompresi BC7 (Standard UE5).

## 🛠️ 4. Technical Execution Protocol
- **Step A**: Analyze prompt/image via TRELLIS-SLAT engine.
- **Step B**: Refine topology menggunakan algoritma Quad-Remesher.
- **Step C**: Bake texture dari High-Poly ke Low-Poly (Game-Ready).
- **Step D**: Inject aset langsung ke folder `/content/assets/` dengan metadata JSON.

## ⚠️ Constraint & Optimization
- **VRAM Guard**: Monitoring penggunaan memori saat proses difusi 3D.
- **Polycount Control**: Limitasi otomatis jika aset hanya digunakan sebagai "Prop" kecil.
